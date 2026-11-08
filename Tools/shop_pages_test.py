"""The pages of the shop with real key presses: the list, the page of the first thing (Enter on its details button), the way back (Esc), the page of the
second thing (Down, Enter), buying it there (Enter), the shop closed, the weapon hint under the life bar, and the guard pose (HUD/hero test flags).
Starts a new game at the end, so the gold and the weapons of the test do not stay in the saved progress.
Usage: python shop_pages_test.py <out folder>"""
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

KEYS = {"T": (0x54, 0x14), "B": (0x42, 0x30), "X": (0x58, 0x2D), "ENTER": (0x0D, 0x1C), "ESC": (0x1B, 0x01), "DOWN": (0x28, 0x50)}


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


def new_game():
    """Starts the game as the player (not as a spectator) and picks New Game."""
    if not P.start_play():
        raise SystemExit("the play session came up as a spectator")

    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
new_game()
time.sleep(2)
subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
time.sleep(1)
hud = P.pie("HUD")[0]
hero = P.live_hero()
ue.props(hud, DebugAddGold=1300)
time.sleep(1.5)

press("B")
time.sleep(2.5)
shot("01_list")
press("ENTER")
time.sleep(1.5)
shot("02_page_first")
press("ESC")
time.sleep(1.2)
shot("03_back_on_list")
press("DOWN")
time.sleep(0.6)
press("ENTER")
time.sleep(1.5)
shot("04_page_second")
press("ENTER")
time.sleep(1.5)
shot("05_bought")
press("ESC")
time.sleep(1.0)
press("ESC")
time.sleep(2.5)
shot("06_hint_under_hp")

# the guard pose
ue.props(hero, bDebugGuard=True)
time.sleep(1.2)
shot("07_guard")
ue.props(hero, bDebugGuard=True)
time.sleep(0.8)

ue.call(ue.E, "StopPIE")

# a new game, so the progress of the test does not stay
time.sleep(3)
new_game()
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
