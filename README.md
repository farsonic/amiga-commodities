# ClickToFront, StickyMenus and NoIconFrames

Small commodities for AmigaOS 3.x (any 68000 or better; ClickToFront and
StickyMenus need OS 2.04+, NoIconFrames OS 3.5+):

- **ClickToFront** - click anywhere in a window (its border, title bar or
  contents) and the window comes to the front. Backdrop windows (like the
  Workbench backdrop) are left alone, and so is a click on a window's own
  depth gadget, so that gadget still sends windows to the back.
- **StickyMenus** - menus that stay open after a click, like on a Mac. Click
  the right mouse button and the menu bar stays open: move over the menus
  without holding a button, then click an item (left or right button) to
  choose it, or click anywhere else to close the menus. Holding the button
  and releasing over an item still works the classic Amiga way.
- **NoIconFrames** - Workbench icons without the box around them. It
  switches icon.library's global "frameless" setting on (OS 3.5, 3.9, 3.1.4,
  3.2: icon.library V44 or newer) and makes the Workbench redraw. Disable
  in Exchange, or quitting it, puts the frames back. About 1.3 KB, no input
  handler and no C library: it only waits for Exchange and Ctrl-C.

ClickToFront and StickyMenus are about 5 KB. All use no CPU while idle and appear in Commodities Exchange
(Enable, Disable, Remove). Starting one again stops it.

## Install

Double-click **Install-Commodities**: it asks which commodities to install
(ClickToFront, StickyMenus, NoIconFrames, any combination), where to put them
(SYS:WBStartup to start them at every boot, SYS:Tools/Commodities, or a drawer
of your choice) and whether to start them now. It needs Installer (on the
Workbench 3.1 Install disk, in SYS:Utilities on 3.5 and later); if Workbench
can't find it, set the icon's Default Tool to where Installer is.

Or by hand: Copy `ClickToFront`, `StickyMenus` and `NoIconFrames` (with their icons) to
`SYS:WBStartup` to start them at boot (their icons carry the `DONOTWAIT`
tool type, so Workbench does not wait for them to "return" - commodities
keep running), or to `SYS:Tools/Commodities` to start
them by double-click. From a Shell: `Run >NIL: ClickToFront`.

## Tool types

ClickToFront:

| Tool type | Default | Meaning |
|---|---|---|
| `CX_PRIORITY` | 0 | commodity priority |
| `QUALIFIER` | NONE | a key that must be held while clicking: NONE, SHIFT, ALT, CTRL, AMIGA |
| `ONRELEASE` | NO | YES: act when the button is released, so you can drag an icon out of a window behind another without raising it first |
| `DEBUG` | NO | YES: print what each click finds (start it from a Shell) |

StickyMenus:

| Tool type | Default | Meaning |
|---|---|---|
| `CX_PRIORITY` | 0 | commodity priority |
| `CLICKTIME` | 350 | a right-button press shorter than this (milliseconds) is a click that leaves the menus open; releasing over the screen title bar always leaves them open |

## How they work

- **ClickToFront** installs an input handler that notices left-button
  presses and wakes the commodity. It then finds the window under the
  pointer from the screen's layers and calls `WindowToFront()`.
- **StickyMenus** installs an input handler ahead of Intuition.
  - When the menus are open and the right button is released quickly, it
    hides that release, so Intuition keeps the menus open as if the button
    were still held.
  - The next click is turned into the release: Intuition chooses the item
    under the pointer, or nothing if the pointer is elsewhere.
  - It only does this while the menus really are open (`WFLG_MENUSTATE` on
    the active window). On systems that never set that flag, such as AROS,
    it checks that the active window has a menu strip and the pointer is on
    its screen. Windows that use the right button themselves (`RMBTRAP`)
    and games are left alone.
- Without commodities.library (some ROM-only setups) both still run; only
  the Exchange controls are missing.

## Building and testing

```
make                      # vbcc + NDK 3.2: build/ClickToFront, build/StickyMenus
make package              # build/Commodities.lha
sudo tests/run_emu_test.py  # end-to-end on an emulated A1200 (FS-UAE, AROS ROM)
```

The emulator test opens two overlapping windows with a menu (tests/twowin.c),
drives the mouse and checks:
- clicks bring the window behind to the front;
- a quick right click leaves the menus open;
- left and right clicks choose items;
- a click elsewhere closes the menus;
- holding the button and releasing still chooses.

## Licence

GPL-2.0-only.
