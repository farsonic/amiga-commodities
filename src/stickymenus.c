/*
 * StickyMenus - menus that stay open after a click, like on a Mac.
 *
 * Click the right mouse button (press and release quickly) and the menu bar
 * stays open: move over the menus without holding a button, then click an
 * item with either button to choose it, or click anywhere else to close the
 * menus. Holding the button and releasing over an item still works the
 * Amiga way.
 *
 * How: an input handler ahead of Intuition hides the right-button release
 * from Intuition while the menus are open, so Intuition keeps tracking the
 * menus as if the button were held. The next click is turned into that
 * release, which picks the item under the pointer (or nothing). It only does
 * this when the menus really are open (WFLG_MENUSTATE on the active window;
 * on systems that never set it, such as AROS, when the active window has a
 * menu strip and the pointer is on its screen), so programs that use the
 * right button themselves (RMBTRAP windows, games) are not affected.
 *
 * Tool types:
 *   CX_PRIORITY=0    commodity priority
 *   CLICKTIME=350    a press shorter than this (milliseconds) is a click
 *                    that leaves the menus open; releasing over the screen's
 *                    title bar always leaves them open
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>
#include "cxcommon.h"

static const char version[] = "$VER: StickyMenus 1.0 (5.11.2026)";

struct IntuitionBase *IntuitionBase = NULL;

enum {
	S_IDLE,       /* nothing going on */
	S_HELD,       /* right button pressed, menus may be opening */
	S_STICKY,     /* release hidden: menus open with no button held */
	S_EAT_LUP,    /* left click became the menu release: hide the left release */
	S_EAT_RUP     /* right click became the menu release: hide its release */
};

static volatile int state = S_IDLE;
static ULONG down_secs, down_micro;
static ULONG click_ms = 350;

#define RDOWN IECODE_RBUTTON
#define RUP   (IECODE_RBUTTON | IECODE_UP_PREFIX)
#define LDOWN IECODE_LBUTTON
#define LUP   (IECODE_LBUTTON | IECODE_UP_PREFIX)

/* set once WFLG_MENUSTATE has been seen: AmigaOS sets it while menus are
 * open, AROS does not, so until then we go by the window having a menu
 * strip and the pointer being on its screen */
static BOOL menustate_seen;

/* the active window, if its menus can be open (read in input.device
 * context: Intuition changes these in the same task, after us) */
static struct Window *menu_window(void)
{
	struct Window *w = IntuitionBase->ActiveWindow;
	struct Screen *s;
	if (!w || (w->Flags & WFLG_RMBTRAP) || !w->MenuStrip)
		return NULL;
	if (w->Flags & WFLG_MENUSTATE) {
		menustate_seen = TRUE;
		return w;
	}
	if (menustate_seen)
		return NULL;
	for (s = IntuitionBase->FirstScreen; s && s->MouseY < 0; s = s->NextScreen)
		;
	return s == w->WScreen ? w : NULL;
}

static ULONG elapsed_ms(const struct InputEvent *e)
{
	LONG s = (LONG)e->ie_TimeStamp.tv_secs - (LONG)down_secs;
	LONG us = (LONG)e->ie_TimeStamp.tv_micro - (LONG)down_micro;
	if (s < 0 || s > 100)
		return 0xFFFF;
	return (ULONG)(s * 1000 + us / 1000);
}

static struct InputEvent *handler(__reg("a0") struct InputEvent *ev, __reg("a1") APTR data)
{
	CxApp *app = (CxApp *)data;
	struct InputEvent *e, *prev = NULL, *next;
	if (!app->enabled) {
		state = S_IDLE;
		return ev;
	}
	for (e = ev; e; e = next) {
		BOOL drop = FALSE;
		next = e->ie_NextEvent;
		if (e->ie_Class == IECLASS_RAWMOUSE) {
			UWORD code = e->ie_Code;
			struct Window *w;
			switch (state) {
			case S_IDLE:
				if (code == RDOWN) {
					state = S_HELD;
					down_secs = e->ie_TimeStamp.tv_secs;
					down_micro = e->ie_TimeStamp.tv_micro;
				}
				break;
			case S_HELD:
				if (code == RUP) {
					state = S_IDLE;
					w = menu_window();
					if (w && (elapsed_ms(e) < click_ms ||
					          w->WScreen->MouseY <= w->WScreen->BarHeight)) {
						drop = TRUE;           /* keep the menus open */
						state = S_STICKY;
					}
				}
				break;
			case S_STICKY:
				if (!menu_window()) {          /* menus went away by themselves */
					state = S_IDLE;
					break;
				}
				if (code == LDOWN) {
					/* this click chooses: make it the menu button release */
					e->ie_Code = RUP;
					e->ie_Qualifier &= ~(IEQUALIFIER_LEFTBUTTON | IEQUALIFIER_RBUTTON);
					state = S_EAT_LUP;
				} else if (code == RDOWN) {
					/* same with the right button: the press chooses, its release is hidden */
					e->ie_Code = RUP;
					e->ie_Qualifier &= ~IEQUALIFIER_RBUTTON;
					state = S_EAT_RUP;
				} else {
					/* moves: Intuition still believes the button is down */
					e->ie_Qualifier |= IEQUALIFIER_RBUTTON;
				}
				break;
			case S_EAT_LUP:
				if (code == LUP) {
					drop = TRUE;
					state = S_IDLE;
				} else if (code == LDOWN || code == RDOWN)
					state = S_IDLE;            /* lost a release somewhere: start over */
				break;
			case S_EAT_RUP:
				if (code == RUP) {
					drop = TRUE;
					state = S_IDLE;
				} else if (code == LDOWN || code == RDOWN)
					state = S_IDLE;
				break;
			}
		}
		if (drop) {
			if (prev)
				prev->ie_NextEvent = next;
			else
				ev = next;
		} else
			prev = e;
	}
	return ev;
}

int main(int argc, char **argv)
{
	CxApp app;
	int rc = 20;
	(void)version;
	IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
	if (!IntuitionBase)
		return 20;
	memset(&app, 0, sizeof(app));
	app.name = "StickyMenus";
	app.title = "StickyMenus 1.0 (5.11.2026)";
	app.descr = "Menus stay open after a click, like on a Mac";
	app.handler = handler;
	app.handler_pri = 52;          /* ahead of Intuition (50) and commodities (51) */
	if (cx_start(&app, argc, argv)) {
		LONG t = cx_tooltype_int("CLICKTIME", 350);
		click_ms = t > 0 && t < 5000 ? (ULONG)t : 350;
		cx_run(&app, NULL);
		rc = 0;
	}
	cx_stop(&app);
	CloseLibrary((struct Library *)IntuitionBase);
	return rc;
}
