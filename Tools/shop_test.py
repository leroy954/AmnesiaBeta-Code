"""Walks through the shop with real key presses and gold from the test helper of the HUD: the shop with gold, buying the dragon with the Enter key, the dragon
arriving, the opened dragon branch, endless skill points, the shop opened from the pause menu (and back), and the gold of a kill.
Starts a new game at the end, so the gold and the dragon of the test do not stay in the saved progress.
Usage: python shop_test.py <out folder>"""
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes

from PIL import Image

import cmdlib
import ue
import pie_lib as P

user32 = ctypes.windll.user32
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)

KEYS = {"T": (0x54, 0x14), "B": (0x42, 0x30), "ENTER": (0x0D, 0x1C), "ESC": (0x1B, 0x01)}


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


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)


def components():
    hero = P.live_hero()
    comps = ue.call(ue.A, "get_components", actor=hero)
    return [c for c in comps if c["refPath"].endswith(".Skills")][0]


def new_game():
    """Starts the game as the player (not as a spectator) and picks New Game."""
    if not P.start_play():
        raise SystemExit("the play session came up as a spectator")

    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)


subprocess.run([sys.executable, os.path.join(os.path.dirname(__file__), "close_msglog.py")])
new_game()
time.sleep(2)
hud = P.pie("HUD")[0]
skills = components()

# gold, and the shop with the dragon on sale
ue.props(hud, DebugAddGold=300)
time.sleep(1.5)
press("B")
time.sleep(2.5)
shot("01_shop_rich")

# the buy button has the focus: Enter buys
press("ENTER")
time.sleep(2.0)
shot("02_shop_bought")
press("ESC")
time.sleep(1.2)
shot("03_dragon_arrives")
time.sleep(3.5)
shot("04_dragon_flying")

# the opened branch, then endless points
press("T")
time.sleep(2.5)
shot("05_skills_open")
press("T")
time.sleep(1.0)
ue.props(hud, DebugInfinitePoints=1)
time.sleep(1.0)
press("T")
time.sleep(2.5)
shot("06_skills_infinite")
press("T")
time.sleep(1.5)
shot("07_hud_infinite")

# the shop from the pause menu, and back
ue.props(hud, bDebugTogglePauseMenu=True)
time.sleep(2.0)
ue.props(hud, bDebugToggleShop=True)
time.sleep(2.5)
shot("08_shop_from_pause")
press("ESC")
time.sleep(2.0)
shot("09_pause_again")
press("ESC")
time.sleep(1.5)

# the gold of a kill: a calm enemy far down the level
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=1)
enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        enemy = e
        break
time.sleep(3)
print("enemy:", enemy["refPath"].split(":")[-1] if enemy else None)
shot("10_before_kill")

# a hard hit: the flame burst kills the enemy at once, and its XP and gold show up
try:
    ue.props(P.live_hero(), MeleeDamage=800.0)
except Exception as error:
    print("could not raise the damage:", str(error)[:150])
ue.props(skills, bDebugFlameBurst=True)
for i in range(4):
    time.sleep(0.45)
    shot("11_kill_%d" % i)

ue.call(ue.E, "StopPIE")

# a new game, so the progress of the test does not stay
time.sleep(3)
new_game()
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
