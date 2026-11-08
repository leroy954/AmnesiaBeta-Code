"""Does the graphics quality setting of the menu change anything? Picks each level through the HUD (DebugGraphicsQuality, the same
path as the menu buttons), reads the engine's quality cvars and the frame rate, and takes a picture.
Usage: python graphics_test.py [out_prefix]      (pictures and a sheet go to Tools/shots by default)"""
import base64
import ctypes
import io
import json
import os
import shutil
import sys
import time

import ue
from PIL import Image, ImageDraw
from esc_test import editor_window
import cmdlib
from pie_lib import pie, start_play, live_hero, warm_up, HERE

PREFIX = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "shots", "graphics")
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
NAMES = ["Alacsony (0)", "Közepes (1)", "Magas (2)", "Max (3)"]
CVARS = ["sg.ResolutionQuality", "sg.ShadowQuality", "sg.GlobalIlluminationQuality", "sg.ReflectionQuality", "sg.PostProcessQuality",
         "sg.EffectsQuality", "sg.TextureQuality", "sg.ViewDistanceQuality", "r.ScreenPercentage", "r.Shadow.Virtual.MaxPhysicalPages"]


def cvar(name):
    r = ue.call(ue.E, "SearchCVars", name=name)
    d = json.loads(r) if isinstance(r, str) else r
    return d.get(name, {}).get("value")


def grab():
    r = ue.call(ue.E, "CaptureEditorImage")
    img = r["image"] if "image" in r else r
    return Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")


SAVE = os.path.join(os.path.dirname(HERE), "Saved", "SaveGames", "AmnesiaProgress.sav")
BACKUP = SAVE + ".graphics_test_backup"
shutil.copy2(SAVE, BACKUP)                                   # picking a level saves it, the player's own setting comes back at the end
ctypes.windll.user32.ShowWindow(editor_window(), 4)          # the editor runs slowly while it is minimized
ue.props(PERF, bThrottleCPUWhenNotForeground=False)
rows = []
shots = []
try:
    print("before the game: ", {n: cvar(n) for n in CVARS[:2]})
    start_play()
    hud = pie("HUD")[0]
    ue.props(hud, bDebugCloseMenu=True)
    time.sleep(3)
    hero = live_hero()
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(15000, 0, 700, 0, 0, 0, 1, 1, 1))
    time.sleep(3)

    for q in (3, 2, 1, 0):
        ue.props(pie("HUD")[0], DebugGraphicsQuality=q)
        time.sleep(6)                                        # new shaders and the new settings need a moment
        warm_up(60)
        values = {n: cvar(n) for n in CVARS}
        timing = cmdlib.cmd(sample=5.0)                       # frame, game thread, render thread and GPU milliseconds
        rows.append((q, values, timing))
        path = "%s_%d.png" % (PREFIX, q)
        grab().save(path)
        shots.append((NAMES[q], path))
        print("quality %d: %s" % (q, cmdlib.line(timing)))
        print("           resolution %s%%, shadows %s, lighting %s, shadow pages %s" % (values["sg.ResolutionQuality"], values["sg.ShadowQuality"], values["sg.GlobalIlluminationQuality"], values["r.Shadow.Virtual.MaxPhysicalPages"]))
        sys.stdout.flush()
finally:
    try:
        ue.call(ue.E, "StopPIE")
    except Exception:
        pass
    time.sleep(2)
    shutil.copy2(BACKUP, SAVE)
    os.remove(BACKUP)
    ue.props(PERF, bThrottleCPUWhenNotForeground=True)
    print("after the game:  ", {n: cvar(n) for n in CVARS[:2]})

# a sheet with the four pictures, cropped to the game view
ims = [Image.open(p) for _, p in shots]
w, h = ims[0].size
crop = (int(w * 0.14), int(h * 0.13), int(w * 0.78), int(h * 0.96))
cw, ch = crop[2] - crop[0], crop[3] - crop[1]
sheet = Image.new("RGB", (cw * 2 // 2, ch * 2 // 2))
tile = (cw // 2, ch // 2)
sheet = Image.new("RGB", (tile[0] * 2, tile[1] * 2))
d = ImageDraw.Draw(sheet)
for k, ((name, _), im) in enumerate(zip(shots, ims)):
    x, y = (k % 2) * tile[0], (k // 2) * tile[1]
    sheet.paste(im.crop(crop).resize(tile), (x, y))
    d.rectangle((x, y, x + 140, y + 14), fill=(0, 0, 0))
    d.text((x + 4, y + 2), name, fill=(255, 255, 0))
sheet.save(PREFIX + "_sheet.png")
print("saved", PREFIX + "_sheet.png")
