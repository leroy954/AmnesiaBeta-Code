"""Closes the floating Message Log window of the editor (it pops up after a restart and covers the viewport)."""
import ctypes, subprocess
from ctypes import wintypes
user32 = ctypes.windll.user32
out = subprocess.run(["powershell", "-NoProfile", "-Command", "(Get-Process UnrealEditor).Id"], capture_output=True, text=True).stdout
pids = {int(p) for p in out.split() if p.strip().isdigit()}
@ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
def each(hwnd, _):
    pid = wintypes.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    if pid.value in pids and user32.IsWindowVisible(hwnd):
        buf = ctypes.create_unicode_buffer(256)
        user32.GetWindowTextW(hwnd, buf, 256)
        if buf.value == "Message Log":
            print("closing", hwnd)
            user32.PostMessageW(hwnd, 0x0010, 0, 0)
    return True
user32.EnumWindows(each, 0)
