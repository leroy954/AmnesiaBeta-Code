"""Tries several hand rotations of one weapon in a running game and makes a contact sheet of the hero's hand for each, to see which way the model has to turn to sit
right in the hand. The numbers go through Saved/WeaponTuning.json (read again whenever the weapon is put in the hand), so nothing is built.
Usage: python weapon_rot_sweep.py <out png> <weapon id> "p,y,r;p,y,r;..." [x,y,z location of the model in the hand] [scale] [json of more tuning fields, e.g. '{"AuraScale": 0.001}']"""
import json
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
WEAPON = sys.argv[2]
ROTS = [tuple(float(v) for v in item.split(",")) for item in sys.argv[3].split(";")]
LOC = [float(v) for v in sys.argv[4].split(",")] if len(sys.argv) > 4 else [0.0, 0.0, 0.0]
SCALE = float(sys.argv[5]) if len(sys.argv) > 5 else 1.0
EXTRA = json.loads(sys.argv[6]) if len(sys.argv) > 6 else {}
TUNING = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\WeaponTuning.json"
CROP = (280, 120, 560, 470)
TILE = (420, 525)

subprocess.run([sys.executable, "close_msglog.py"])
P.start_pie(park_enemies=True)
time.sleep(4)
hero = P.live_hero()
comps = ue.call(ue.A, "get_components", actor=hero)
skills = [c for c in comps if c["refPath"].endswith(".Skills")][0]
boom = [c for c in comps if c["refPath"].endswith(".CameraBoom")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(-1500, 0, P.ground_h(-1500) + 120, 0, 0, 0, 1, 1, 1))
time.sleep(2)
ue.props(boom, TargetArmLength=380.0, TargetOffset={"x": 0.0, "y": 0.0, "z": -100.0})
time.sleep(1.5)

tiles = []
for rot in ROTS:
    with open(TUNING, "w", encoding="utf-8") as f:
        json.dump({WEAPON: dict({"HandRotation": list(rot), "HandLocation": LOC, "HandScale": SCALE}, **EXTRA)}, f)
    time.sleep(1.1)
    ue.props(hero, DebugWeapon="sword")
    time.sleep(0.6)
    ue.props(hero, DebugWeapon=WEAPON)
    time.sleep(1.6)
    path = ue.shot(os.path.join(os.path.dirname(OUT), "_tmp_sweep.png"))
    im = Image.open(path).convert("RGB")
    w, h = im.size
    full = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    tile = full.crop(CROP).resize(TILE, Image.LANCZOS)
    ImageDraw.Draw(tile).text((6, 4), "pitch %g yaw %g roll %g" % rot, fill=(255, 255, 0))
    tiles.append(tile)
    print("done", rot)

cols = min(4, len(tiles))
rows = (len(tiles) + cols - 1) // cols
sheet = Image.new("RGB", (TILE[0] * cols, TILE[1] * rows))
for i, t in enumerate(tiles):
    sheet.paste(t, ((i % cols) * TILE[0], (i // cols) * TILE[1]))
sheet.save(OUT)
os.remove(os.path.join(os.path.dirname(OUT), "_tmp_sweep.png"))
ue.call(ue.E, "StopPIE")
