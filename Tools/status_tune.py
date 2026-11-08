"""Tries values of the status effect properties of an enemy live, and takes pictures in slow motion.
Usage: python status_tune.py <out folder> <burn|chill|freeze> '<json of enemy properties>' [shots]
Example: python status_tune.py out burn '{"BurnTint": {"R": 0.5, "G": 0.18, "B": 0.03, "A": 1}, "BurnTintOpacity": 0.3}'"""
import json
import os
import sys
import time

from PIL import Image

import ue
import cmdlib
import pie_lib as P

OUT = sys.argv[1]
MODE = sys.argv[2]
PROPS = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
SHOTS = int(sys.argv[4]) if len(sys.argv) > 4 else 3
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)


P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=1 if MODE == "burn" else 2)

enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        enemy = e
        break
time.sleep(3)

# nothing else may hurt or tint it while the picture is taken: no dragon fire, no attacks
ue.props(enemy, **PROPS) if PROPS else None
cmdlib.cmd(["slomo 0.12"], world="pie")
flag = {"burn": "bDebugIgnite", "chill": "bDebugChill", "freeze": "bDebugFreeze"}[MODE]
ue.props(skills, **{flag: True})
for i in range(SHOTS):
    shot("%s_%d" % (MODE, i))

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")
