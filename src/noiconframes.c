/*
 * NoIconFrames - Workbench icons without the box (frame) around them.
 *
 * Switches icon.library's global "frameless" setting on (OS 3.5+, icon.library
 * V44) and makes the Workbench redraw, so every icon is drawn without its
 * frame. Disable in Exchange (or quitting) puts the setting back as it was.
 * Starting it a second time stops the first copy. No input handler, no CPU
 * while idle: it only waits for Exchange and Ctrl-C.
 *
 * Built without the C library (-nostdlib): main() must stay the first function.
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <libraries/commodities.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <graphics/layers.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/commodities.h>
#include <proto/icon.h>

struct ExecBase *SysBase;
struct IntuitionBase *IntuitionBase;
struct Library *CxBase, *IconBase;

static void set_frameless(LONG on);
static void redraw_workbench(void);

int main(void)
{
	struct Process *me;
	struct WBStartup *wbmsg = NULL;
	struct MsgPort *port;
	CxObj *broker = NULL;
	struct NewBroker nb;
	struct Message *m;
	LONG old = 0, on, err = 0;
	ULONG sigs;
	BOOL run = TRUE;
	struct TagItem get[2];

	SysBase = *(struct ExecBase **)4;
	me = (struct Process *)FindTask(NULL);
	if (!me->pr_CLI) {                       /* started from Workbench: take its message */
		WaitPort(&me->pr_MsgPort);
		wbmsg = (struct WBStartup *)GetMsg(&me->pr_MsgPort);
	}
	IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 39);
	CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
	IconBase = OpenLibrary((STRPTR)"icon.library", 44);  /* frameless icons: OS 3.5 and newer */
	if (IntuitionBase && CxBase && IconBase && (port = CreateMsgPort())) {
		nb.nb_Version = NB_VERSION;
		nb.nb_Name = (STRPTR)"NoIconFrames";
		nb.nb_Title = (STRPTR)"NoIconFrames 1.0";
		nb.nb_Descr = (STRPTR)"Workbench icons without frames";
		nb.nb_Unique = NBU_UNIQUE | NBU_NOTIFY;  /* a second start tells us (CXCMD_UNIQUE) */
		nb.nb_Flags = 0;
		nb.nb_Pri = 0;
		nb.nb_Port = port;
		nb.nb_ReservedChannel = 0;
		if ((broker = CxBroker(&nb, &err))) {
			get[0].ti_Tag = ICONCTRLA_GetGlobalFrameless; get[0].ti_Data = (ULONG)&old;
			get[1].ti_Tag = TAG_DONE;
			IconControlA(NULL, get);
			ActivateCxObj(broker, 1);
			set_frameless(on = TRUE);
			while (run) {
				sigs = Wait((1UL << port->mp_SigBit) | SIGBREAKF_CTRL_C);
				if (sigs & SIGBREAKF_CTRL_C) run = FALSE;
				while ((m = GetMsg(port))) {
					ULONG type = CxMsgType((CxMsg *)m), id = CxMsgID((CxMsg *)m);
					ReplyMsg(m);
					if (type != CXM_COMMAND) continue;
					if (id == CXCMD_DISABLE && on) { ActivateCxObj(broker, 0); set_frameless(on = old); }
					else if (id == CXCMD_ENABLE && !on) { ActivateCxObj(broker, 1); set_frameless(on = TRUE); }
					else if (id == CXCMD_KILL || id == CXCMD_UNIQUE) run = FALSE;
				}
			}
			if (on != old) set_frameless(old);
			DeleteCxObjAll(broker);
			while ((m = GetMsg(port))) ReplyMsg(m);
		}
		DeleteMsgPort(port);
	}
	CloseLibrary(IconBase);                  /* (CloseLibrary(NULL) is harmless from V36 on) */
	CloseLibrary(CxBase);
	CloseLibrary((struct Library *)IntuitionBase);
	if (wbmsg) { Forbid(); ReplyMsg(&wbmsg->sm_Message); }
	return broker || err == CBERR_DUP ? 0 : 20;   /* (a second start that stopped the first: fine) */
}

/* the global frameless setting, then a redraw so the Workbench draws its icons again */
static void set_frameless(LONG on)
{
	struct TagItem t[2];
	t[0].ti_Tag = ICONCTRLA_SetGlobalFrameless; t[0].ti_Data = (ULONG)on;
	t[1].ti_Tag = TAG_DONE;
	IconControlA(NULL, t);
	redraw_workbench();
}

/* open a borderless window over the whole Workbench screen and close it again: every window
   under it is damaged and redraws (the Workbench draws its icons afresh) */
static void redraw_workbench(void)
{
	struct Screen *s = LockPubScreen((STRPTR)"Workbench");
	struct Window *w;
	struct TagItem t[8];
	if (!s) return;
	t[0].ti_Tag = WA_PubScreen;    t[0].ti_Data = (ULONG)s;
	t[1].ti_Tag = WA_Left;         t[1].ti_Data = 0;
	t[2].ti_Tag = WA_Top;          t[2].ti_Data = 0;
	t[3].ti_Tag = WA_Width;        t[3].ti_Data = s->Width;
	t[4].ti_Tag = WA_Height;       t[4].ti_Data = s->Height;
	t[5].ti_Tag = WA_Borderless;   t[5].ti_Data = TRUE;
	t[6].ti_Tag = WA_BackFill;     t[6].ti_Data = (ULONG)LAYERS_NOBACKFILL;  /* no drawing: no flash */
	t[7].ti_Tag = TAG_DONE;
	if ((w = OpenWindowTagList(NULL, t))) CloseWindow(w);
	UnlockPubScreen(NULL, s);
}

const char version[] = "$VER: NoIconFrames 1.0 (10.11.2026)";
