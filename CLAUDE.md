# CLAUDE.md - working on Amiga Commodities

Small commodities for AmigaOS 3.x, built and shipped together as `Commodities.lha` with an Installer
script that lets the user choose which to install. GPL-2.0-only (see `LICENSE`). This file is for anyone,
person or agent, picking the work up. Read it first, then `README.md` (what each one does, tool types,
how they work).

## What is here

| Commodity | Source | What it does | Needs |
|---|---|---|---|
| **ClickToFront** 1.0 | `src/clicktofront.c` | A click anywhere in a window brings it to the front (not backdrop windows, not on the depth gadget). Tool types `QUALIFIER`, `ONRELEASE`, `DEBUG`. | OS 2.04+, 68000 |
| **StickyMenus** 1.0 | `src/stickymenus.c` | A short right-button click leaves the menus open, Mac style; the next click chooses. Tool type `CLICKTIME`. | OS 2.04+, 68000 |
| **NoIconFrames** 1.0 | `src/noiconframes.c` | Workbench icons without frames (icon.library's global frameless setting, then a Workbench redraw); restored on Disable or quit. About 1.3 KB, no C library, no input handler. | OS 3.5+ (icon.library 44), 68000 |
| shared | `src/cxcommon.c/.h` | Broker, tool types (icon or CLI), input handler and main loop used by ClickToFront and StickyMenus. | |

All of them appear in Commodities Exchange (Enable, Disable, Remove), use no CPU while idle and stop
when started a second time.

## Build and test

Amiga binaries: `make` builds `build/ClickToFront`, `build/StickyMenus`, `build/NoIconFrames` with vbcc
(0.9h with the m68k-amigaos target, `/opt/amiga/vbcc`, http://www.compilers.de/vbcc.html) and the
NDK 3.2 headers (`/opt/amiga/ndk/Include_H`, Aminet `dev/misc/NDK3.2.lha`); set `VBCC`/`NDK` otherwise.
NoIconFrames is built with `-nostdlib`: `main()` must stay the first function in its file.

Package: `make package` builds `build/Commodities.lha` (the commodities with icons from
`tools/mkicon.py`, `Install-Commodities`, ReadMe, LICENSE and Source/) and refuses to finish if any file
holds certificate or key material.

Tests:

- `python3 tools/checkinst.py pkg/Install-Commodities` checks the Installer script statically (CI runs
  this, plus the icon generator). The script must run on Commodore's Installer 43, InstallerLG and AROS's
  Installer: no `procedure`, no `select` for values, no `(and ...)` in conditions (nest `if`s), and
  every relative source through `(tackon #src "...")` (the script finds its drawer from `@icon`).
- `sudo tests/run_emu_test.py` runs the end-to-end test on an emulated Amiga (FS-UAE with the AROS ROM;
  it needs a harness that boots the emulator and runs a command list, see the script's top). It opens two
  windows with a menu (`tests/twowin.c`) and drives the mouse through `tests/fsuae_pointer.py`
  (FS-UAE turns X motion into relative Amiga motion, so the pointer is found in screenshots and steered
  in a closed loop). `tests/nifcheck.c` prints icon.library's frameless setting for NoIconFrames' test.
- There are no host unit tests: these programs are thin layers over Intuition, commodities.library and
  input.device. Test on an emulator or a real Amiga, and say what you could and couldn't verify.

## Rules of the project

- vbcc C99, 68000-compatible, small: no C library where it isn't needed, no CPU use while idle (wait on
  signals, never poll), and keep input handlers short (signal the main task; do the work there).
- Leave windows that use the right button themselves (`RMBTRAP`), backdrop windows and games alone.
- Every commodity appears in Exchange, honours Enable/Disable/Remove and quits when started twice.
- A new commodity gets its own `src/<name>.c`, a Makefile rule, an icon image in `tools/mkicon.py`, a
  line in `tools/mkpackage.sh`, an option in `pkg/Install-Commodities` and a README section.
- Don't use `rm -rf` in scripts or by hand on shared machines; move things aside.
- Never put a personal email address in commits, code or docs.

## Where work stands

All three are done. ClickToFront and StickyMenus passed the emulator test; NoIconFrames was checked in the
emulator (setting on, second start stops the first and restores it) but its visible effect needs a real
Workbench 3.5+ (AROS's Wanderer draws icons its own way). The Installer script was run with AROS's
Installer; Commodore's Installer 43 on a real 3.1/3.2 is the remaining check.

## Issues

Bugs and requests come in as GitHub issues (templates in `.github/ISSUE_TEMPLATE/`). To work on them:

1. `gh issue list --repo farsonic/amiga-commodities` and `gh issue view N --repo farsonic/amiga-commodities --comments`.
2. Reproduce first, on an emulator where possible; most problems depend on the OS version, the
   screen, other commodities or patches running, and the window concerned, so ask for those.
3. Fix on a branch named `issue-N-short-name` and open a pull request whose description says `Fixes #N`,
   what changed and how it was tested (emulator, which OS). Don't close issues by hand unless asked;
   merging the PR closes them.

`.github/workflows/claude.yml` lets people mention `@claude` in an issue or pull request to have Claude
Code look at it (it needs the `ANTHROPIC_API_KEY` repository secret or the Claude GitHub App).
