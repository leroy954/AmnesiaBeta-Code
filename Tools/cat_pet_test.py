"""Pets the cats with the real F key in a running game (window messages to the editor, like a real key press) and takes pictures:
the sleeping cat with the prompt, the moment after the first pet (banner, popups, happy hop), the awake cat, the second pet. Then it goes to the other cats
and pets them all, to see the banner of the last one (all found). The save file is read for the ids of the found cats.
Usage: python cat_pet_test.py <out folder> [how many cats, default all]"""
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes

from PIL import Image

import cats as C
import ue
import pie_lib as P

user32 = ctypes.windll.user32
OUT = sys.argv[1]
COUNT = int(sys.argv[2]) if len(sys.argv) > 2 else len(C.CATS)
SAVE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "SaveGames", "AmnesiaProgress.sav")
os.makedirs(OUT, exist_ok=True)

KEYS = {"F": (0x46, 0x21)}


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
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    im.save(path)
    return im


def saved_ids():
    try:
        data = open(SAVE, "rb").read()
    except OSError:
        return "no save file"
    return [c[0] for c in C.CATS if c[0].encode() in data]


def go_to(x, dx=-120.0):
    ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(x + dx, 0, P.ground_h(x + dx) + 130, 0, 0, 0, 1, 1, 1))
    time.sleep(2.2)


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=True)
time.sleep(3)
print("saved before:", saved_ids())

for i, (cat_id, zone, coat, x_first, y, yaw) in enumerate(C.CATS[:COUNT]):
    go_to(x_first)
    shot("%d_%s_near" % (i, cat_id))
    press("F")
    time.sleep(0.5)
    shot("%d_%s_petted" % (i, cat_id))
    time.sleep(2.2)
    shot("%d_%s_awake" % (i, cat_id))
    if i == 0:
        press("F")
        time.sleep(0.4)
        shot("0_%s_again" % cat_id)
    print(cat_id, "saved:", saved_ids())

time.sleep(1.5)
shot("last")
ue.call(ue.E, "StopPIE")
print("saved at the end:", saved_ids())
