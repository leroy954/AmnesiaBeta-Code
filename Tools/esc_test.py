"""Presses the real Escape key (a window message to the editor, like a key press) in a running game and reports whether
the pause menu opened, closed again on a second press, and whether PIE survived."""
import ctypes
import os
import subprocess
import time
from ctypes import wintypes

import ue
from pie_lib import pie, start_play, HERE

user32 = ctypes.windll.user32
VK_ESCAPE = 0x1B
SCAN_ESCAPE = 0x01


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


def press(vk, scan, hold=0.08):
    hwnd = editor_window()
    down = 1 | (scan << 16)
    up = 1 | (scan << 16) | (1 << 30) | (1 << 31)
    user32.PostMessageW(hwnd, 0x0100, vk, down)
    time.sleep(hold)
    user32.PostMessageW(hwnd, 0x0101, vk, ctypes.c_long(up - (1 << 32)).value)


def state(label):
    on = ue.pie_on()
    cursor = None
    paused = None

    if on:
        pcs = pie("PlayerController")

        if pcs:
            try:
                cursor = ue.get_props(pcs[0], ["bShowMouseCursor"])
            except Exception as e:
                cursor = "err %s" % e

    print("%-28s PIE=%s  PC=%s" % (label, on, cursor))
    return on, cursor


if __name__ == "__main__":
    if not start_play():
        raise SystemExit("PIE did not start as the player")

    time.sleep(3)
    state("main menu")

    # the main menu has nothing to go back to: Esc must neither stop the game nor close the menu
    press(VK_ESCAPE, SCAN_ESCAPE)
    time.sleep(1.5)
    on, cursor = state("main menu + Esc (stays?)")

    if on:
        hud = pie("HUD")[0]
        ue.props(hud, bDebugCloseMenu=True)          # past the main menu, like New Game
        time.sleep(4)
        state("in game")

        press(VK_ESCAPE, SCAN_ESCAPE)
        time.sleep(1.5)
        on, _ = state("after Esc #1 (pause menu?)")
        ue.shot(os.path.join(HERE, "shots", "esc_pause.png"))

    if on:
        press(VK_ESCAPE, SCAN_ESCAPE)
        time.sleep(1.5)
        on, _ = state("after Esc #2 (resumed?)")

    # the admin panel (K) closes on Esc as well
    if on:
        press(0x4B, 0x25)
        time.sleep(1.5)
        state("after K (admin panel?)")
        press(VK_ESCAPE, SCAN_ESCAPE)
        time.sleep(1.5)
        on, _ = state("after Esc (admin closed?)")

    if on:
        ue.call(ue.E, "StopPIE")
