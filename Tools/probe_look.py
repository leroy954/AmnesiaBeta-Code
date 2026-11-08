"""One look at probe cats: a magenta test cat at the given places, the picture of each saved whole, and the most magenta pixels printed (to see how the zone's light changes the colour).
Usage: python probe_look.py <out folder> <x> [<x> ...]"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
XS = [float(v) for v in sys.argv[2:]]
FOLDER = "Temp/CatTest"
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT, exist_ok=True)
ops = [{"op": "delete", "folder": FOLDER}]
for x in XS:
    ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaCat", "loc": [x, -25.0, P.ground_h(x)], "rot": [0.0, 0.0, 0.0], "label": "TestCat_%d" % x,
                "folder": FOLDER, "props": {"CatId": "t_%d" % x, "Coat": 1, "CatScale": 1.8, "CoatFolder": "/Game/Gen/CatProbe/"}})
ue.job(ops, timeout=300)
subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
P.start_pie(park_enemies=True)
time.sleep(3)
for x in XS:
    ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(x - 230.0, 0, P.ground_h(x) + 130, 0, 0, 0, 1, 1, 1))
    time.sleep(1.8)
    path = os.path.join(OUT, "probe_%d.png" % x)
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    im.save(path)
    # the most magenta pixels of the picture: min(r-g, b-g)
    r, g, b = im.split()
    best = []
    px = im.load()
    for yy in range(0, im.height, 2):
        for xx in range(0, im.width, 2):
            rr, gg, bb = px[xx, yy]
            best.append((min(rr - gg, bb - gg), (rr, gg, bb), (xx, yy)))
    best.sort(reverse=True)
    print(int(x), best[:3])
ue.call(ue.E, "StopPIE")
time.sleep(2)
ue.job([{"op": "delete", "folder": FOLDER}], timeout=300)
