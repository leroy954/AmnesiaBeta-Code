"""Puts every aura system of some series of the MeleeWeaponAura_VFX pack on a weapon in a running game, one after the other, and makes one contact sheet of the
hero's hand for all of them, to pick the look of each weapon. The aura goes through Saved/WeaponTuning.json (AuraPath), so nothing is built.
Usage: python aura_survey.py <out png> <weapon id> "pitch,yaw,roll" "x,y,z" <series, e.g. V2,V3> [columns, default 6]"""
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
ROT = [float(v) for v in sys.argv[3].split(",")]
LOC = [float(v) for v in sys.argv[4].split(",")]
SERIES = sys.argv[5].split(",")
COLUMNS = int(sys.argv[6]) if len(sys.argv) > 6 else 6
TUNING = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\WeaponTuning.json"
CROP = (130, 170, 600, 440)
TILE = (352, 202)

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
for series in SERIES:
    for number in range(1, 13):
        name = "NS_StylizedWeapon_%s_%02d" % (series, number)
        path = "/Game/MeleeWeaponAura_VFX/VFX/%s/%s.%s" % (series, name, name)
        with open(TUNING, "w", encoding="utf-8") as f:
            json.dump({WEAPON: {"HandRotation": ROT, "HandLocation": LOC, "AuraPath": path}}, f)
        time.sleep(1.1)
        ue.props(hero, DebugWeapon="sword")
        time.sleep(0.5)
        ue.props(hero, DebugWeapon=WEAPON)
        time.sleep(1.4)
        shot = ue.shot(os.path.join(os.path.dirname(OUT), "_tmp_aura.png"))
        im = Image.open(shot).convert("RGB")
        w, h = im.size
        full = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
        tile = full.crop(CROP).resize(TILE, Image.LANCZOS)
        ImageDraw.Draw(tile).text((4, 3), "%s_%02d" % (series, number), fill=(255, 255, 0))
        tiles.append(tile)
        print("done", name)

rows = (len(tiles) + COLUMNS - 1) // COLUMNS
sheet = Image.new("RGB", (TILE[0] * COLUMNS, TILE[1] * rows))
for i, t in enumerate(tiles):
    sheet.paste(t, ((i % COLUMNS) * TILE[0], (i // COLUMNS) * TILE[1]))
sheet.save(OUT)
os.remove(os.path.join(os.path.dirname(OUT), "_tmp_aura.png"))
ue.call(ue.E, "StopPIE")
