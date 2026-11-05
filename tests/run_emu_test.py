#!/usr/bin/env python3
# Commodities - end-to-end test on an emulated A1200 (FS-UAE, AROS ROM):
# starts ClickToFront and StickyMenus, opens tests/twowin.c's two windows,
# drives the mouse through xdotool and checks twowin's log.
#
#   sudo tests/run_emu_test.py [RUN_EMU]
# RUN_EMU is Lagoon's tests/amiga/run_emu.sh (boots the emulator and runs a
# command list; any harness that does the same will do).
# SPDX-License-Identifier: GPL-2.0-only
import glob
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
sys.path.insert(0, HERE)
os.environ["DISPLAY"] = ":97"
import fsuae_pointer as fp  # noqa: E402

RUN_EMU = sys.argv[1] if len(sys.argv) > 1 else "/home/user/farsonic/lagoon/tests/amiga/run_emu.sh"
SHOTS = os.environ.get("SHOTS", os.path.join(TOP, "build", "shots"))
os.makedirs(SHOTS, exist_ok=True)

# screen pixels (1024x768 Xvfb, FS-UAE scaling) of the things we aim at
A_BODY = (330, 340)      # visible part of Window A
B_BODY = (650, 450)      # Window B
TITLE = (300, 129)       # screen title bar, away from the menu titles
MENU_TEST = (220, 131)   # the "Test" menu title
ITEM = {1: (215, 164), 2: (215, 184), 3: (222, 204)}
EMPTY = (700, 600)       # bare screen

cmds = os.path.join(TOP, "build", "emu_cmds")
with open(cmds, "w") as f:
    f.write("ctf|Run DH0:C/ClickToFront DEBUG=YES\n")
    f.write("sm|Run DH0:C/StickyMenus\n")
    f.write("tw|DH0:C/twowin 170\n")
before = set(glob.glob("/tmp/lagoon-emu.*"))
emu = subprocess.Popen([RUN_EMU, cmds, "260"] + [os.path.join(TOP, "build", n) for n in ("ClickToFront", "StickyMenus", "twowin")],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
work = None
for _ in range(600):
    new = [d for d in glob.glob("/tmp/lagoon-emu.*") if d not in before and os.path.isdir(d)]
    if new and os.path.exists(os.path.join(new[0], "dh0/out/twowin.ready")):
        work = new[0]
        break
    time.sleep(1)
if not work:
    print("FAIL: twowin did not start")
    emu.kill()
    sys.exit(1)
out = os.path.join(work, "dh0/out")
time.sleep(2)

fails = 0


def log():
    with open(os.path.join(out, "tw"), errors="replace") as f:
        return f.read().replace("\r", "")


def expect(what, cond_line, since, timeout=4.0):
    global fails
    t = time.time()
    while time.time() - t < timeout:
        if cond_line in log()[since:]:
            print("ok   %s" % what)
            return
        time.sleep(0.3)
    print("FAIL %s: no '%s' in log" % (what, cond_line))
    fails += 1


def expect_not(what, line, since, wait=1.5):
    global fails
    time.sleep(wait)
    if line in log()[since:]:
        print("FAIL %s: unexpected '%s'" % (what, line))
        fails += 1
    else:
        print("ok   %s" % what)


def mark():
    return len(log())


def move(p):
    fp.click(p[0], p[1], 0)


def quick(button):
    subprocess.run(["xdotool", "mousedown", str(button)], check=True)
    time.sleep(0.1)
    subprocess.run(["xdotool", "mouseup", str(button)], check=True)
    time.sleep(0.5)


def shot(name):
    fp.shot(os.path.join(SHOTS, name + ".png"))


# 1. ClickToFront: click the body of the window behind
m = mark()
fp.click(*A_BODY)
expect("click in Window A brings it to the front", "front A", m)
shot("1-a-front")
# 2. and back again
m = mark()
fp.click(*B_BODY)
expect("click in Window B brings it to the front", "front B", m)
# 3. StickyMenus: a quick right click leaves the menus open ...
move(TITLE)
m = mark()
quick(3)
expect_not("quick right click: no pick yet (menus stay open)", "pick", m)
move(MENU_TEST)
time.sleep(0.5)
shot("3-sticky-open")
# ... and a left click on an item chooses it
move(ITEM[2])
time.sleep(0.4)
shot("3-sticky-hover-two")
m2 = mark()
fp.click(*ITEM[2])
expect("left click on 'Two' picks it", "pick B 2", m2)
# 4. sticky, then click elsewhere closes the menus without a choice
move(TITLE)
m = mark()
quick(3)
move(EMPTY)
fp.click(*EMPTY)
expect("click elsewhere closes the menus", "pick B none", m)
# 5. sticky, then right click on an item chooses it
move(TITLE)
quick(3)
shot("5a-after-quick")
move(MENU_TEST)
time.sleep(0.4)
move(ITEM[1])
time.sleep(0.4)
shot("5b-on-one")
m = mark()
quick(3)
expect("right click on 'One' picks it", "pick B 1", m)
# 6. classic Amiga drag-select still works
move(MENU_TEST)
m = mark()
subprocess.run(["xdotool", "mousedown", "3"], check=True)
time.sleep(0.6)
move(ITEM[3])
time.sleep(0.4)
subprocess.run(["xdotool", "mouseup", "3"], check=True)
expect("hold and release on 'Three' picks it", "pick B 3", m)
# 7. after menus, ClickToFront still works
m = mark()
fp.click(*A_BODY)
expect("ClickToFront after menu use", "front A", m)
shot("7-final")

# close Window A (its close gadget): twowin ends and the harness stops the emulator
fp.click(259, 194)
try:
    emu.wait(timeout=240)
except subprocess.TimeoutExpired:
    print("note: harness still running")
for n in ("ctf", "sm"):
    p = os.path.join(out, n)
    if os.path.exists(p):
        txt = open(p, errors="replace").read().strip()
        if txt:
            print("--- %s:\n%s" % (n, txt[-800:]))
print("--- twowin log:\n" + log())
print("run_emu_test: %s (%d failed) - work dir %s, screenshots in %s" % ("PASSED" if not fails else "FAILED", fails, work, SHOTS))
sys.exit(1 if fails else 0)
