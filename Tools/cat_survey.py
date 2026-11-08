"""Looks for good places for the cats: puts a test cat at several candidate places in each zone, walks the hero to each in the running game and takes
a picture around it. The test cats are removed afterwards. Usage: python cat_survey.py <out folder> <first|long> [scale] [json of candidates by zone, replaces the built-in list]"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
WORLD = sys.argv[2]
SCALE = float(sys.argv[3]) if len(sys.argv) > 3 else 1.15
import json
CUSTOM = json.loads(sys.argv[4]) if len(sys.argv) > 4 else None
# light coats show best; the dark ones are lost in the grass
COAT = {"forest": 7, "village": 4, "snow": 2, "ruins": 1, "cave": 9, "storm": 5}
FOLDER = "Temp/CatTest"
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT, exist_ok=True)

FIRST = {
    "forest": [600, 1700, 7600], "village": [11300, 15900, 19800], "snow": [22000, 26200, 31000],
    "ruins": [33000, 38300, 40700], "cave": [47000, 49500, 51500], "storm": [55300, 59000, 62900],
}
LONG = {
    "forest": [200000, 211600, 240000], "village": [290000, 328000, 365000], "snow": [410000, 443600, 480000],
    "ruins": [520000, 549000, 580000], "cave": [620000, 660000, 700000], "storm": [740000, 772300, 805000],
}
CANDIDATES = CUSTOM if CUSTOM else (FIRST if WORLD == "first" else LONG)

ops = [{"op": "delete", "folder": FOLDER}]
items = []
for zone, xs in CANDIDATES.items():
    for x in xs:
        items.append((zone, float(x)))
        ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaCat", "loc": [float(x), -40.0, P.ground_h(float(x))], "rot": [0.0, 0.0, 0.0],
                    "label": "TestCat_%s_%d" % (zone, x), "folder": FOLDER, "props": {"CatId": "t_%s_%d" % (zone, x), "Coat": COAT[zone], "CatScale": SCALE}})
print(ue.job(ops, timeout=300)["spawned"], "test cats")

subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
P.start_pie(park_enemies=True)
time.sleep(3)
tiles = []
for zone, x in items:
    ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(x - 230.0, 0, P.ground_h(x) + 130, 0, 0, 0, 1, 1, 1))
    time.sleep(1.8)
    subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
    path = os.path.join(OUT, "%s_%d.png" % (zone, x))
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    im = im.crop((200, 250, 700, 500))
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, 500, 16), fill=(0, 0, 0))
    d.text((4, 2), "%s %d" % (zone, x), fill=(255, 255, 255))
    im.save(path)
    tiles.append(im)
    print("shot", zone, x)

ue.call(ue.E, "StopPIE")
time.sleep(2)
print(ue.job([{"op": "delete", "folder": FOLDER}], timeout=300)["deleted"], "test cats removed")

# one sheet with every candidate, three to a row
cols = 3
rows = (len(tiles) + cols - 1) // cols
big = Image.new("RGB", (500 * cols, 250 * rows))
for i, im in enumerate(tiles):
    big.paste(im, ((i % cols) * 500, (i // cols) * 250))
big.save(os.path.join(OUT, "all.png"))
print("sheet", big.size)
