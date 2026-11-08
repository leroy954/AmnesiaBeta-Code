"""Holds D through the editor's window messages and films each of the three ground swings while walking.
Usage: python walk_slash_test.py [out.png]"""
import sys, time, base64, io, threading, ctypes, subprocess
from ctypes import wintypes
import ue
from PIL import Image, ImageDraw
from pie_lib import pie, start_play, live_hero

user32 = ctypes.windll.user32
OUT = sys.argv[1] if len(sys.argv) > 1 else "shots/walk_slash.png"
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}


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


MAIN = editor_window()
user32.ShowWindow(MAIN, 4)
holding = {"key": 0}


def hold_loop():
    while holding["key"] >= 0:
        if holding["key"]:
            user32.PostMessageW(MAIN, 0x0100, holding["key"], 1 | (0x20 << 16) | (1 << 30))
        time.sleep(0.05)


def release(vk):
    user32.PostMessageW(MAIN, 0x0101, vk, ctypes.c_long((1 | (0x20 << 16) | (1 << 30) | (1 << 31)) - (1 << 32)).value)


def grab():
    r = ue.call(ue.E, "CaptureEditorImage")
    img = r["image"] if "image" in r else r
    im = Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")
    w, h = im.size
    return im.crop((int(w * 0.30), int(h * 0.38), int(w * 0.64), int(h * 0.98))).resize((260, 300))


ue.props(PERF, bThrottleCPUWhenNotForeground=False)
start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
hero = live_hero()
ue.props({"refPath": hero["refPath"] + ".CameraBoom"}, TargetArmLength=420.0)

slashes = ue.get_props(hero, ["GroundSlashes"])["GroundSlashes"]
for n, e in enumerate(pie("Ent_") + pie("Skeleton") + [g for g in pie("Golem") if "Giant" not in g["refPath"]]):
    try:
        ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
    except RuntimeError:
        pass
threading.Thread(target=hold_loop, daemon=True).start()
rows = []
for i, s in enumerate(slashes):
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(15000 + i * 2500, 0, 700, 0, 0, 0, 1, 1, 1))
    time.sleep(1.0)
    holding["key"] = 0x44
    time.sleep(1.0)
    ue.props(hero, DebugSlashStartDurationRate={"x": s["startTime"], "y": s["duration"], "z": s["playRate"]}, bDebugSlash=True)
    t0 = time.time()
    row = []
    while time.time() - t0 < s["duration"] / s["playRate"] + 0.1 and len(row) < 7:
        x = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
        row.append(("swing %d  %.2fs  x %.0f" % (i + 1, time.time() - t0, x), grab()))
    holding["key"] = 0
    release(0x44)
    rows.append(row)
holding["key"] = -1

cols = max(len(r) for r in rows)
sheet = Image.new("RGB", (260 * cols, 300 * len(rows)))
d = ImageDraw.Draw(sheet)
for y, row in enumerate(rows):
    for x, (label, im) in enumerate(row):
        sheet.paste(im, (x * 260, y * 300))
        d.text((x * 260 + 4, y * 300 + 4), label, fill=(255, 255, 0))
sheet.save(OUT)
ue.call(ue.E, "StopPIE")
ue.props(PERF, bThrottleCPUWhenNotForeground=True)
print("saved", OUT)
