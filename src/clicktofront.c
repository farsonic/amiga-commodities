/*
 * ClickToFront - click anywhere in a window and it comes to the front.
 *
 * An input handler (ahead of Intuition) notes left-button presses and wakes
 * the commodity, which then - after Intuition has handled the click and made
 * that window active - brings the active window to the front if the pointer
 * is inside it. Backdrop windows and clicks on a window's depth gadget (which
 * would undo it) are left alone.
 *
 * Tool types:
 *   CX_PRIORITY=0      commodity priority
 *   QUALIFIER=NONE     key that must be held: NONE, SHIFT, ALT, CTRL, AMIGA
 *   DEBUG=NO           YES: print what each click finds (run from a Shell)
 *   ONRELEASE=NO       YES: act when the button is released instead of pressed
 *                      (lets you drag icons out of a window behind another)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/dos.h>
#include <string.h>
#include "cxcommon.h"

static const char version[] = "$VER: ClickToFront 1.0 (5.11.2026)";

struct IntuitionBase *IntuitionBase = NULL;
struct Library *LayersBase = NULL;

static UWORD want_qual;           /* IEQUALIFIER_* mask that must be held (0 = none) */
static UWORD trigger_code;        /* IECODE_LBUTTON or its release */

/* input.device context: keep it short, just wake the main task */
static volatile ULONG nevents;

static struct InputEvent *handler(__reg("a0") struct InputEvent *ev, __reg("a1") APTR data)
{
	CxApp *app = (CxApp *)data;
	struct InputEvent *e;
	nevents++;
	if (!app->enabled)
		return ev;
	for (e = ev; e; e = e->ie_NextEvent)
		if (e->ie_Class == IECLASS_RAWMOUSE && e->ie_Code == trigger_code &&
		    (!want_qual || (e->ie_Qualifier & want_qual)))
			Signal(app->task, 1UL << app->sig);
	return ev;
}

/* is (x, y), relative to the window, on the window's depth gadget? */
static BOOL on_depth_gadget(struct Window *w, WORD x, WORD y)
{
	struct Gadget *g;
	for (g = w->FirstGadget; g; g = g->NextGadget) {
		WORD gx = g->LeftEdge, gy = g->TopEdge, gw = g->Width, gh = g->Height;
		if (!(g->GadgetType & GTYP_SYSGADGET) || (g->GadgetType & GTYP_SYSTYPEMASK) != GTYP_WDEPTH)
			continue;
		if (g->Flags & GFLG_RELRIGHT) gx += w->Width - 1;
		if (g->Flags & GFLG_RELBOTTOM) gy += w->Height - 1;
		if (g->Flags & GFLG_RELWIDTH) gw += w->Width;
		if (g->Flags & GFLG_RELHEIGHT) gh += w->Height;
		if (x >= gx && x < gx + gw && y >= gy && y < gy + gh)
			return TRUE;
	}
	return FALSE;
}

static BOOL debug;

static void bring_to_front(CxApp *app)
{
	struct Screen *s;
	struct Window *w = NULL;
	struct Layer *l;
	WORD x, y;
	ULONG lock;
	(void)app;
	/* the screen under the pointer: the frontmost one the pointer is not
	 * above (screens can be dragged down) */
	lock = LockIBase(0);
	for (s = IntuitionBase->FirstScreen; s && s->MouseY < 0; s = s->NextScreen)
		;
	UnlockIBase(lock);
	if (!s)
		return;
	x = s->MouseX;
	y = s->MouseY;
	/* the window under the pointer, from the screen's layers (independent
	 * of whether Intuition has activated it yet) */
	LockLayerInfo(&s->LayerInfo);
	l = WhichLayer(&s->LayerInfo, x, y);
	if (l && l->Window && l->front) {
		struct Window *cand = (struct Window *)l->Window;
		if (!(cand->Flags & WFLG_BACKDROP) &&
		    !on_depth_gadget(cand, (WORD)(x - cand->LeftEdge), (WORD)(y - cand->TopEdge)))
			w = cand;
	}
	UnlockLayerInfo(&s->LayerInfo);
	if (debug)
		Printf((STRPTR)"[%lu events] click %ld,%ld layer %lx window %lx -> %s\n", (LONG)x, (LONG)y, (ULONG)l,
		       (ULONG)(l ? l->Window : NULL), (STRPTR)(w ? "front" : "skip"));
	if (debug)
		Flush(Output());
	if (w)
		WindowToFront(w);
}

static UWORD parse_qualifier(const char *q)
{
	char c = q[0];
	if (c >= 'a' && c <= 'z')
		c -= 32;
	switch (c) {
	case 'S': return IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT;
	case 'A': return q[1] == 'L' || q[1] == 'l' ? (IEQUALIFIER_LALT | IEQUALIFIER_RALT)
	                                            : (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
	case 'C': return IEQUALIFIER_CONTROL;
	}
	return 0;                         /* NONE or anything else */
}

int main(int argc, char **argv)
{
	CxApp app;
	int rc = 20;
	(void)version;
	IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
	LayersBase = OpenLibrary((STRPTR)"layers.library", 37);
	if (!IntuitionBase || !LayersBase)
		return 20;
	memset(&app, 0, sizeof(app));
	app.name = "ClickToFront";
	app.title = "ClickToFront 1.0 (5.11.2026)";
	app.descr = "Click anywhere in a window to bring it to the front";
	app.handler = handler;
	app.handler_pri = 51;
	if (cx_start(&app, argc, argv)) {
		want_qual = parse_qualifier(cx_tooltype("QUALIFIER", "NONE"));
		trigger_code = cx_tooltype_yes("ONRELEASE", FALSE) ? (IECODE_LBUTTON | IECODE_UP_PREFIX) : IECODE_LBUTTON;
		debug = cx_tooltype_yes("DEBUG", FALSE);
		if (debug)
		{
			Printf((STRPTR)"ClickToFront running, handler events so far %lu\n", nevents);
			Flush(Output());
		}
		cx_run(&app, bring_to_front);
		rc = 0;
	}
	cx_stop(&app);
	CloseLibrary(LayersBase);
	CloseLibrary((struct Library *)IntuitionBase);
	return rc;
}
