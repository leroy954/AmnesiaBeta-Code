"""Looks at the fire effects in slow motion and tries values of their properties live: the flame burst (Q) and a burning enemy.
Usage: python fire_tune.py <out folder> <burst|burn|chill|freeze|nova|pop> [skills json] [enemy json] [shots] [slomo]
The two JSON objects are properties set on the hero's skill component and on the enemy before the effect starts.
Example: python fire_tune.py out burst '{"FlameBurstScale": 1.0}' '{}' 6 0.1"""
import json
import os
import subprocess
import sys
import time

from PIL import Image

import cmdlib
import ue
import pie_lib as P

OUT = sys.argv[1]
MODE = sys.argv[2]
SKILL_PROPS = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
ENEMY_PROPS = json.loads(sys.argv[4]) if len(sys.argv) > 4 else {}
SHOTS = int(sys.argv[5]) if len(sys.argv) > 5 else 5
SLOMO = sys.argv[6] if len(sys.argv) > 6 else "0.1"
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.20), int(h * 0.30), int(w * 0.75), int(h * 0.92))).save(path)
    return path


P.start_pie(park_enemies=False)
time.sleep(3)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=2 if MODE in ("chill", "freeze", "nova") else 1)

enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        enemy = e
        break
time.sleep(3)

if SKILL_PROPS:
    ue.props(skills, **SKILL_PROPS)

if ENEMY_PROPS:
    ue.props(enemy, **ENEMY_PROPS)

# the Message Log window pops up over the viewport after the load errors of a play session
subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
cmdlib.cmd(["slomo " + SLOMO], world="pie")
flag = {"burst": "bDebugFlameBurst", "burn": "bDebugIgnite", "chill": "bDebugChill", "freeze": "bDebugFreeze", "nova": "bDebugNovaEffect", "pop": "bDebugGuardPop"}[MODE]
ue.props(skills, **{flag: True})
paths = []

for i in range(SHOTS):
    paths.append(shot("%s_%d" % (MODE, i)))

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")

# a contact sheet of all the pictures
ims = [Image.open(p).convert("RGB") for p in paths]
w, h = ims[0].size
cols = 3
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (w * cols, h * rows))

for i, im in enumerate(ims):
    sheet.paste(im, ((i % cols) * w, (i // cols) * h))

sheet.save(os.path.join(OUT, MODE + "_sheet.png"))
print("sheet", sheet.size)
