"""Looks at the frost nova in a running game at normal speed: the hero learns everything, takes the ice element, stands in front of an Ent and presses the real E key;
pictures are taken every half second and the enemy's position and animation rate are printed, so a frozen enemy shows as a held pose and no movement.
Usage: python freeze_check.py <out folder> [distance, default 300]"""
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
DISTANCE = float(sys.argv[2]) if len(sys.argv) > 2 else 300.0
os.makedirs(OUT, exist_ok=True)
KEYS = {"E": (0x45, 0x12)}


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


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    im.save(path)
    return im


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=2)
enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        enemy = e
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - DISTANCE, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        break
print("enemy:", enemy["refPath"].split(":")[-1])
mesh = [c for c in ue.call(ue.A, "get_components", actor=enemy) if c["refPath"].endswith(".CharacterMesh0")][0]
time.sleep(3)
shot("0_before")
press("E")
t0 = time.time()
for i in range(7):
    time.sleep(0.45)
    x = ue.call(ue.A, "get_actor_transform", actor=enemy)["location"]["x"]
    rate = ue.get_props(mesh, "GlobalAnimRateScale")
    print("t=%.1f x=%.0f anim rate %s" % (time.time() - t0, x, rate))
    shot("%d_after" % (i + 1))
ue.call(ue.E, "StopPIE")
