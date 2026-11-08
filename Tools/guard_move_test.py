"""Checks that the hero walks while guarding (right mouse button): holds D for 1.5 s without the guard and with it (the guard is switched on through the
bDebugGuard test flag), and prints how far the hero got each time, with the guard flag and a picture. Usage: python guard_move_test.py <out folder>"""
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
OUT = sys.argv[1] if len(sys.argv) > 1 else "."


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


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)


def hold_d(hwnd, seconds, snap=None):
    down = 1 | (0x20 << 16)
    up = 1 | (0x20 << 16) | (1 << 30) | (1 << 31)
    t0 = time.time()
    first = True
    while time.time() - t0 < seconds:
        user32.PostMessageW(hwnd, 0x0100, 0x44, down | (0 if first else (1 << 30)))
        first = False
        time.sleep(0.1)
        if snap and time.time() - t0 > seconds * 0.6:
            shot(snap)
            snap = None
    user32.PostMessageW(hwnd, 0x0101, 0x44, ctypes.c_long(up - (1 << 32)).value)


def x_of(hero):
    return ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
    P.start_pie(park_enemies=True)
    time.sleep(3)
    hero = P.live_hero()
    hwnd = editor_window()

    x0 = x_of(hero)
    hold_d(hwnd, 1.5, "walk")
    time.sleep(0.6)
    x1 = x_of(hero)
    print("no guard: x %.0f -> %.0f (moved %.0f)" % (x0, x1, x1 - x0))

    ue.props(hero, bDebugGuard=True)
    time.sleep(1.0)
    x0 = x_of(hero)
    hold_d(hwnd, 1.5, "guard_walk")
    time.sleep(0.6)
    x1 = x_of(hero)
    print("guard: x %.0f -> %.0f (moved %.0f)" % (x0, x1, x1 - x0))

    ue.call(ue.E, "StopPIE")
