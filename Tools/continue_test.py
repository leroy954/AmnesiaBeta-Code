"""Presses Continue in the main menu of a running game and checks that the reloaded game runs and is not stuck in the menu."""
import time
import ue
from pie_lib import pie, start_play, live_hero

start_play()
time.sleep(3)
hud = pie("HUD")[0]
print("hud:", hud["refPath"])
ue.props(hud, bDebugContinue=True)
time.sleep(8)

hud2 = pie("HUD")
print("hud after:", [h["refPath"] for h in hud2])
pc = pie("PlayerController")
for p in pc:
    try:
        print("pc", p["refPath"], ue.get_props(p, ["bShowMouseCursor"]))
    except Exception as e:
        print("pc err", e)
hero = live_hero()
t = ue.call(ue.A, "get_actor_transform", actor=hero)
print("hero at", t)
loc = t["location"] if "location" in t else t
x, y, z = loc["x"], loc["y"], loc["z"]
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(x, y, z + 400, 0, 0, 0, 1, 1, 1))
time.sleep(2)
t2 = ue.call(ue.A, "get_actor_transform", actor=hero)
loc2 = t2["location"] if "location" in t2 else t2
print("lifted to %.0f, after 2 s at %.0f -> %s" % (z + 400, loc2["z"], "game runs" if loc2["z"] < z + 300 else "PAUSED/FROZEN"))

# keyboard: hold D through the editor's own window messages, the way a real key press reaches the game
import ctypes
from ctypes import wintypes
user32 = ctypes.windll.user32
import subprocess
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
main = max(wins, key=lambda h: (lambda r: (r.right - r.left) * (r.bottom - r.top))((lambda r: (user32.GetWindowRect(h, ctypes.byref(r)), r)[1])(wintypes.RECT())))
x0 = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
for _ in range(15):
    user32.PostMessageW(main, 0x0100, 0x44, 1 | (0x20 << 16) | ((1 << 30) if _ else 0))
    time.sleep(0.1)
user32.PostMessageW(main, 0x0101, 0x44, ctypes.c_long((1 | (0x20 << 16) | (1 << 30) | (1 << 31)) - (1 << 32)).value)
time.sleep(0.5)
x1 = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
print("D held 1.5 s: x %.0f -> %.0f -> %s" % (x0, x1, "input works" if abs(x1 - x0) > 50 else "NO INPUT"))
ue.call(ue.E, "StopPIE")
