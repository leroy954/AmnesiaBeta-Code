"""Opens the pause menu in a running game, goes to the Főmenü with real key presses (down four times, enter) and checks that
the level is not loaded again: the main menu must open over the same level. Pictures of the pause menu and the main menu.
Starts a new game at the end, so the progress of the test does not stay in the saved progress.
Usage: python main_menu_test.py <out folder>"""
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes

from PIL import Image

import ue
import pie_lib as P

user32 = ctypes.windll.user32
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)
LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Saved", "Logs", "AmnesiaBeta.log")

KEYS = {"DOWN": (0x28, 0x50), "ENTER": (0x0D, 0x1C)}


def editor_window():
    out = subprocess.run(["powershell", "-NoProfile", "-Command", "(Get-Process UnrealEditor).Id"], capture_output=True, text=True).stdout
    pids = {int(p) for p in out.split() if p.strip().isdigit()}
    wins = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def each(hwnd, _):
        pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value in pids and user32.IsWindowVisible(hwnd):
            wins.append(hwnd)
        return True

    user32.EnumWindows(each, 0)

    def area(h):
        r = wintypes.RECT()
        user32.GetWindowRect(h, ctypes.byref(r))
        return (r.right - r.left) * (r.bottom - r.top)

    return max(wins, key=area)


def press(name, hold=0.1):
    vk, scan = KEYS[name]
    hwnd = editor_window()
    down = 1 | (scan << 16)
    up = 1 | (scan << 16) | (1 << 30) | (1 << 31)
    user32.PostMessageW(hwnd, 0x0100, vk, down)
    time.sleep(hold)
    user32.PostMessageW(hwnd, 0x0101, vk, ctypes.c_long(up - (1 << 32)).value)
    print("pressed", name)


def shot(name, box=(0.139, 0.133, 0.781, 0.963)):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * box[0]), int(h * box[1]), int(w * box[2]), int(h * box[3]))).save(path)
    print("shot", name)


def load_maps():
    with open(LOG, encoding="utf-8", errors="replace") as f:
        return sum(1 for line in f if "LogLoad: LoadMap:" in line)


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
if not P.start_play():
    raise SystemExit("the play session came up as a spectator")
hud = P.pie("HUD")[0]
ue.props(hud, bDebugCloseMenu=True)
time.sleep(4)

maps_before = load_maps()
ue.props(hud, bDebugTogglePauseMenu=True)
time.sleep(1.5)
shot("01_pause_menu")

# the Főmenü button of the pause menu, through the test flag of the HUD (the arrow keys do not reach the menu buttons)
ue.props(hud, bDebugMainMenu=True)
time.sleep(2.5)
shot("02_main_menu")

maps_after = load_maps()
print("level loads during the test:", maps_after - maps_before, "(must be 0)")

ue.call(ue.E, "StopPIE")

# a new game, so the progress of the test does not stay
time.sleep(3)
if not P.start_play():
    raise SystemExit("the play session came up as a spectator")
ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(4)
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
