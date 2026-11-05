/*
 * Shared plumbing for the commodities (see cxcommon.h).
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/input.h>
#include <libraries/commodities.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/icon.h>
#include <clib/alib_protos.h>
#include <string.h>
#include "cxcommon.h"

struct Library *CxBase = NULL;
struct Library *IconBase = NULL;

static struct MsgPort *cxport;
static CxObj *broker;
static struct MsgPort *inport;
static struct IOStdReq *inreq;
static struct Interrupt handler_int;
static BOOL handler_added;
static UBYTE **ttypes;
static BOOL public_port;

const char *cx_tooltype(const char *name, const char *def)
{
	const char *v = ttypes ? (const char *)ArgString(ttypes, (STRPTR)name, (STRPTR)def) : def;
	return v ? v : def;
}

LONG cx_tooltype_int(const char *name, LONG def)
{
	return ttypes ? ArgInt(ttypes, (STRPTR)name, def) : def;
}

static BOOL same(const char *a, const char *b)
{
	for (; *a && *b; a++, b++)
		if ((*a | 32) != (*b | 32))
			return FALSE;
	return *a == *b;
}

BOOL cx_tooltype_yes(const char *name, BOOL def)
{
	const char *v = cx_tooltype(name, def ? "YES" : "NO");
	return same(v, "YES") || same(v, "ON") || same(v, "TRUE");
}

static void say(const char *s)
{
	BPTR out = Output();
	if (out)
		Write(out, (APTR)s, strlen(s));
}

BOOL cx_start(CxApp *app, int argc, char **argv)
{
	struct NewBroker nb;
	LONG err = 0;

	app->task = FindTask(NULL);
	app->sig = -1;
	app->enabled = TRUE;
	CxBase = OpenLibrary((STRPTR)"commodities.library", 37);
	IconBase = OpenLibrary((STRPTR)"icon.library", 36);
	if (IconBase)
		ttypes = ArgArrayInit(argc, (CONST_STRPTR *)argv);
	cxport = CreateMsgPort();
	if (!cxport)
		return FALSE;
	app->sig = AllocSignal(-1);

	if (!CxBase) {
		/* no commodities.library (some ROM-only setups): run without the
		 * Exchange controls; a public port named after us stands in for
		 * the broker's uniqueness: starting again stops the running copy */
		struct MsgPort *other;
		Forbid();
		other = FindPort((STRPTR)app->name);
		if (other)
			Signal(other->mp_SigTask, SIGBREAKF_CTRL_C);
		else {
			cxport->mp_Node.ln_Name = (char *)app->name;
			cxport->mp_Node.ln_Pri = 0;
			AddPort(cxport);
			public_port = TRUE;
		}
		Permit();
		if (other)
			return FALSE;
	} else {
	memset(&nb, 0, sizeof(nb));
	nb.nb_Version = NB_VERSION;
	nb.nb_Name = (STRPTR)app->name;
	nb.nb_Title = (STRPTR)app->title;
	nb.nb_Descr = (STRPTR)app->descr;
	nb.nb_Unique = NBU_UNIQUE | NBU_NOTIFY;
	nb.nb_Pri = (BYTE)cx_tooltype_int("CX_PRIORITY", 0);
	nb.nb_Port = cxport;
	broker = CxBroker(&nb, &err);
	if (!broker) {
		if (err != CBERR_DUP)        /* a second start tells the first to quit */
			say("cannot create the commodity broker\n");
		return FALSE;
	}
	}

	/* our input handler, ahead of Intuition */
	inport = CreateMsgPort();
	inreq = inport ? (struct IOStdReq *)CreateIORequest(inport, sizeof(struct IOStdReq)) : NULL;
	if (!inreq || OpenDevice((STRPTR)"input.device", 0, (struct IORequest *)inreq, 0)) {
		if (inreq)
			DeleteIORequest((struct IORequest *)inreq);
		inreq = NULL;
		say("cannot open input.device\n");
		return FALSE;
	}
	handler_int.is_Node.ln_Type = NT_INTERRUPT;
	handler_int.is_Node.ln_Pri = app->handler_pri;
	handler_int.is_Node.ln_Name = (char *)app->name;
	handler_int.is_Data = app;
	handler_int.is_Code = (void (*)())app->handler;
	inreq->io_Command = IND_ADDHANDLER;
	inreq->io_Data = &handler_int;
	DoIO((struct IORequest *)inreq);
	handler_added = TRUE;

	if (broker)
		ActivateCxObj(broker, TRUE);
	return TRUE;
}

void cx_run(CxApp *app, void (*on_signal)(CxApp *app))
{
	ULONG cxsig = 1UL << cxport->mp_SigBit;
	ULONG hsig = app->sig >= 0 ? 1UL << app->sig : 0;
	BOOL done = FALSE;
	while (!done) {
		ULONG got = Wait(cxsig | hsig | SIGBREAKF_CTRL_C);
		CxMsg *m;
		if (got & SIGBREAKF_CTRL_C)
			done = TRUE;
		if ((got & hsig) && on_signal && app->enabled)
			on_signal(app);
		while (CxBase && (m = (CxMsg *)GetMsg(cxport)) != NULL) {
			ULONG type = CxMsgType(m), id = CxMsgID(m);
			ReplyMsg((struct Message *)m);
			if (type != CXM_COMMAND)
				continue;
			switch (id) {
			case CXCMD_DISABLE:
				ActivateCxObj(broker, FALSE);
				app->enabled = FALSE;
				break;
			case CXCMD_ENABLE:
				ActivateCxObj(broker, TRUE);
				app->enabled = TRUE;
				break;
			case CXCMD_KILL:
			case CXCMD_UNIQUE:      /* started again: quit, like the system commodities */
				done = TRUE;
				break;
			}
		}
	}
}

void cx_stop(CxApp *app)
{
	CxMsg *m;
	app->enabled = FALSE;
	if (handler_added) {
		inreq->io_Command = IND_REMHANDLER;
		inreq->io_Data = &handler_int;
		DoIO((struct IORequest *)inreq);
		handler_added = FALSE;
	}
	if (inreq) {
		CloseDevice((struct IORequest *)inreq);
		DeleteIORequest((struct IORequest *)inreq);
	}
	if (inport)
		DeleteMsgPort(inport);
	if (broker)
		DeleteCxObjAll(broker);
	if (cxport) {
		if (public_port)
			RemPort(cxport);
		while ((m = (CxMsg *)GetMsg(cxport)) != NULL)
			ReplyMsg((struct Message *)m);
		DeleteMsgPort(cxport);
	}
	if (app->sig >= 0)
		FreeSignal(app->sig);
	if (ttypes)
		ArgArrayDone();
	if (IconBase)
		CloseLibrary(IconBase);
	if (CxBase)
		CloseLibrary(CxBase);
}
