"""Presses the real keys of the skills in a running game (window messages to the editor, like a real key press): T opens the skill screen,
Escape closes it, R changes the element, Q casts the flame burst, E the frost nova. Takes a picture after each.
Usage: python skills_keys_test.py <out folder>"""
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

KEYS = {"T": (0x54, 0x14), "Q": (0x51, 0x10), "E": (0x45, 0x12), "R": (0x52, 0x13), "ESC": (0x1B, 0x01)}


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


P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=1)
time.sleep(1)

press("T")
time.sleep(2.5)
shot("key_T_open")
press("ESC")
time.sleep(2.5)
shot("key_ESC_closed")

press("R")
time.sleep(1.5)
shot("key_R_element")

for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        break
time.sleep(3)
press("Q")
time.sleep(1.2)
shot("key_Q_burst")
press("E")
time.sleep(1.2)
shot("key_E_nova")

ue.call(ue.E, "StopPIE")
