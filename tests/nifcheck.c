/* prints icon.library's global frameless setting (for NoIconFrames' emulator test)
 * SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/icon.h>
struct Library *IconBase;
int main(void)
{
	LONG v = -1;
	struct TagItem t[2];
	if (!(IconBase = OpenLibrary((STRPTR)"icon.library", 44))) { printf("frameless: no icon.library V44\n"); return 5; }
	t[0].ti_Tag = ICONCTRLA_GetGlobalFrameless; t[0].ti_Data = (ULONG)&v; t[1].ti_Tag = TAG_DONE;
	IconControlA(NULL, t);
	printf("frameless: %ld\n", (long)v);
	CloseLibrary(IconBase);
	return 0;
}
