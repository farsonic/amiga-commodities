/* Lagoon - tassign NAME PATH: AssignLock() for emulator tests (the AROS
 * ROM's Assign fails in the harness's Startup-Sequence). After Riptide's
 * tests/amiga/run_fsuae_prefs.sh helper (same author).
 * SPDX-License-Identifier: GPL-2.0-only */
#include <proto/dos.h>
int main(int argc, char **argv)
{
	BPTR l;
	if (argc < 3 || !(l = Lock((STRPTR)argv[2], ACCESS_READ)))
		return 20;
	if (!AssignLock((STRPTR)argv[1], l)) {
		UnLock(l);
		return 20;
	}
	return 0;
}
