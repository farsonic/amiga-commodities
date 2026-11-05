/*
 * twowin - test program for the commodities: opens two overlapping windows
 * ("Window A" behind, "Window B" in front), both with a menu "Test"
 * (One, Two, Three), and logs to stdout:
 *   front A|B            whenever the frontmost of the two changes
 *   active A|B           when a window becomes active
 *   pick A <item>        menu item chosen (or "pick A none")
 *   menus A open|closed  when the window's menu state changes
 *   left / geometry      positions, so the test can aim
 *   twowin SECONDS
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/layers.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

static struct IntuiText it[3] = {
	{ 0, 1, JAM1, 2, 1, NULL, (UBYTE *)"One", NULL },
	{ 0, 1, JAM1, 2, 1, NULL, (UBYTE *)"Two", NULL },
	{ 0, 1, JAM1, 2, 1, NULL, (UBYTE *)"Three", NULL },
};
static struct MenuItem items[2][3];
static struct Menu menus[2];

static void make_menu(int k)
{
	int i;
	for (i = 0; i < 3; i++) {
		struct MenuItem *m = &items[k][i];
		m->NextItem = i < 2 ? &items[k][i + 1] : NULL;
		m->LeftEdge = 0;
		m->TopEdge = (WORD)(i * 10);
		m->Width = 80;
		m->Height = 10;
		m->Flags = ITEMTEXT | ITEMENABLED | HIGHCOMP;
		m->ItemFill = &it[i];
	}
	menus[k].NextMenu = NULL;
	menus[k].LeftEdge = 4;
	menus[k].TopEdge = 0;
	menus[k].Width = 60;
	menus[k].Height = 10;
	menus[k].Flags = MENUENABLED;
	menus[k].MenuName = (BYTE *)"Test";
	menus[k].FirstItem = &items[k][0];
}

/* is layer a in front of layer b? */
static int in_front(struct Layer *a, struct Layer *b)
{
	struct Layer *l;
	for (l = b->front; l; l = l->front)
		if (l == a)
			return 1;
	return 0;
}

int main(int argc, char **argv)
{
	struct Window *w[2];
	struct NewWindow nw;
	const char *names[2] = { "A", "B" };
	int secs = argc > 1 ? atoi(argv[1]) : 60, ticks, k, front = -1, mstate[2] = { 0, 0 };
	ULONG sigs;

	IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 37);
	GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 37);
	if (!IntuitionBase || !GfxBase)
		return 20;
	for (k = 0; k < 2; k++) {
		memset(&nw, 0, sizeof(nw));
		nw.LeftEdge = (WORD)(k ? 260 : 60);
		nw.TopEdge = (WORD)(k ? 70 : 30);
		nw.Width = 300;
		nw.Height = 130;
		nw.DetailPen = (UBYTE)-1;
		nw.BlockPen = (UBYTE)-1;
		nw.IDCMPFlags = IDCMP_MENUPICK | IDCMP_ACTIVEWINDOW | IDCMP_CLOSEWINDOW;
		nw.Flags = WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH;
		nw.Title = (UBYTE *)(k ? "Window B" : "Window A");
		nw.Type = WBENCHSCREEN;
		w[k] = OpenWindow(&nw);
		if (!w[k])
			return 20;
		make_menu(k);
		SetMenuStrip(w[k], &menus[k]);
	}
	for (k = 0; k < 2; k++)
		printf("geometry %s %d %d %d %d\n", names[k], w[k]->LeftEdge, w[k]->TopEdge, w[k]->Width, w[k]->Height);
	printf("screen %d %d bar %d\n", w[0]->WScreen->Width, w[0]->WScreen->Height, w[0]->WScreen->BarHeight);
	fflush(stdout);
	{
		BPTR f = Open((STRPTR)"DH0:out/twowin.ready", MODE_NEWFILE);
		if (f) { Write(f, "ready\n", 6); Close(f); }
	}
	sigs = (1UL << w[0]->UserPort->mp_SigBit) | (1UL << w[1]->UserPort->mp_SigBit);
	for (ticks = 0; ticks < secs * 10; ticks++) {
		struct IntuiMessage *m;
		int f, quit = 0;
		Delay(5);
		f = in_front(w[0]->WLayer, w[1]->WLayer) ? 0 : 1;
		if (f != front) {
			front = f;
			printf("front %s\n", names[f]);
		}
		for (k = 0; k < 2; k++) {
			int ms = (w[k]->Flags & WFLG_MENUSTATE) ? 1 : 0;
			if (ms != mstate[k]) {
				mstate[k] = ms;
				printf("menus %s %s\n", names[k], ms ? "open" : "closed");
			}
			while ((m = (struct IntuiMessage *)GetMsg(w[k]->UserPort)) != NULL) {
				ULONG cl = m->Class;
				UWORD code = m->Code;
				ReplyMsg((struct Message *)m);
				if (cl == IDCMP_ACTIVEWINDOW)
					printf("active %s\n", names[k]);
				else if (cl == IDCMP_CLOSEWINDOW)
					quit = 1;
				else if (cl == IDCMP_MENUPICK) {
					if (code == MENUNULL)
						printf("pick %s none\n", names[k]);
					else
						printf("pick %s %d\n", names[k], ITEMNUM(code) + 1);
				}
			}
		}
		fflush(stdout);
		if (quit)
			break;
	}
	(void)sigs;
	for (k = 1; k >= 0; k--) {
		ClearMenuStrip(w[k]);
		CloseWindow(w[k]);
	}
	printf("end\n");
	return 0;
}
