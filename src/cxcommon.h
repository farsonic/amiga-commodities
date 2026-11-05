/*
 * Shared plumbing for the commodities: broker (so Exchange can enable,
 * disable and quit them), tool types (Workbench icon or CLI arguments), an
 * input.device handler, and the main loop.
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CXCOMMON_H
#define CXCOMMON_H

#include <exec/types.h>
#include <exec/interrupts.h>
#include <devices/inputevent.h>

/* the input handler: called by input.device with A0 = event list and
 * A1 = is_Data; returns the (possibly shortened) event list */
typedef struct InputEvent *(*cx_handler_fn)(__reg("a0") struct InputEvent *ev, __reg("a1") APTR data);

typedef struct CxApp {
	const char    *name;          /* broker name, e.g. "ClickToFront" */
	const char    *title;         /* "ClickToFront 1.0 (5.11.2026)" */
	const char    *descr;         /* one line for Exchange */
	cx_handler_fn  handler;
	BYTE           handler_pri;   /* above Intuition (50) to see events first */
	volatile BOOL  enabled;       /* the handler checks this */
	struct Task   *task;          /* our task, for Signal() from the handler */
	BYTE           sig;           /* a signal the handler may raise (-1 = none) */
} CxApp;

/* tool types (from the icon when started from Workbench, else from the
 * command line as NAME=VALUE words); valid between cx_start and cx_stop */
const char *cx_tooltype(const char *name, const char *def);
LONG        cx_tooltype_int(const char *name, LONG def);
BOOL        cx_tooltype_yes(const char *name, BOOL def);

/* opens the libraries, the broker (CX_PRIORITY tool type) and installs the
 * handler; returns FALSE (after printing why, if possible) on failure.
 * If another copy is running it is told to quit and FALSE is returned. */
BOOL cx_start(CxApp *app, int argc, char **argv);
/* waits for Exchange commands and Ctrl-C; on_signal (may be NULL) runs
 * whenever app->sig is raised by the handler */
void cx_run(CxApp *app, void (*on_signal)(CxApp *app));
void cx_stop(CxApp *app);

#endif
