"""Close-up pictures of the hero's sword with the steel, fire and ice aura: the camera of the hero is brought close.
Usage: python aura_look.py <out folder> [arm length] [json of aura variables to set on the Niagara component after each switch]"""
import json
import os
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
ARM = float(sys.argv[2]) if len(sys.argv) > 2 else 260.0
os.makedirs(OUT, exist_ok=True)

P.start_pie(park_enemies=False)
time.sleep(3)
hero = P.live_hero()
comps = ue.call(ue.A, "get_components", actor=hero)
skills = [c for c in comps if c["refPath"].endswith(".Skills")][0]
boom = [c for c in comps if c["refPath"].endswith(".CameraBoom")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(boom, TargetArmLength=ARM, TargetOffset={"X": 0.0, "Y": 0.0, "Z": 20.0})
time.sleep(1.0)
tiles = []

for label, element in (("steel", 0), ("fire", 1), ("ice", 2)):
    ue.props(skills, DebugElement=element)
    time.sleep(1.5)
    path = os.path.join(OUT, label + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im = im.crop((int(w * 0.30), int(h * 0.25), int(w * 0.70), int(h * 0.95)))
    im.save(path)
    tiles.append(im)
    print("shot", label)

ue.call(ue.E, "StopPIE")
w, h = tiles[0].size
sheet = Image.new("RGB", (w * len(tiles), h))
for i, im in enumerate(tiles):
    sheet.paste(im.convert("RGB"), (i * w, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet", sheet.size)
