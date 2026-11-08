"""Walks through the weapons of the shop with real key presses and the test helpers of the HUD: the shop with the dragon and three weapons on sale, buying the
weapons (the buy button of the HUD helper), the banner of a new weapon, the skill screen with its seven columns, and X changing the weapon in the hand.
Starts a new game at the end, so the gold and the weapons of the test do not stay in the saved progress.
Usage: python weapons_shop_test.py <out folder>"""
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

KEYS = {"T": (0x54, 0x14), "B": (0x42, 0x30), "X": (0x58, 0x2D), "ENTER": (0x0D, 0x1C), "ESC": (0x1B, 0x01)}


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
hud = P.pie("HUD")[0]
hero = P.live_hero()
print("weapon of the new game:", ue.get_props(hero, "DebugWeapon"))

# gold, and the shop with the dragon and the three weapons on sale
ue.props(hud, DebugAddGold=1300)
time.sleep(1.5)
press("B")
time.sleep(2.5)
shot("01_shop_rich")

ue.props(hud, DebugBuyItem="spear")
time.sleep(2.0)
shot("02_bought_spear")
ue.props(hud, DebugBuyItem="axe")
time.sleep(1.5)
ue.props(hud, DebugBuyItem="wand")
time.sleep(2.0)
shot("03_all_bought")

# the shop closes: the banner of the new weapon
press("ESC")
time.sleep(1.2)
shot("04_banner")
time.sleep(3.0)
shot("05_wand_in_hand")

# the skill screen with seven columns
press("T")
time.sleep(2.5)
shot("06_skills_open")
press("T")
time.sleep(1.5)

# X changes the weapon in the hand
for i in range(4):
    press("X")
    time.sleep(1.4)
    shot("07_x_%d" % i)

ue.call(ue.E, "StopPIE")

# a new game, so the progress of the test does not stay
time.sleep(3)
new_game()
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
