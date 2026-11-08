"""Puts the 11 coats of the cat pack as real AmnesiaCat actors side by side in the first forest zone (two groups) and takes pictures in the running game.
The test cats are removed afterwards. Usage: python cat_coats.py <out folder> [x of the first cat] [yaw]"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
X0 = float(sys.argv[2]) if len(sys.argv) > 2 else 3000.0
YAW = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
FOLDER = "Temp/CatTest"
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT, exist_ok=True)

ops = [{"op": "delete", "folder": FOLDER}]
for i in range(11):
    x = X0 + i * 115.0
    ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaCat", "loc": [x, -40.0, P.ground_h(x)], "rot": [0.0, YAW, 0.0],
                "label": "TestCat_%d" % (i + 1), "folder": FOLDER, "props": {"CatId": "test%d" % (i + 1), "Coat": i + 1}})
print(ue.job(ops, timeout=300)["spawned"], "test cats")

subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(3)
hero = P.live_hero()
paths = []
for name, hx in (("a", X0 + 2 * 115.0), ("b", X0 + 8 * 115.0)):
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(hx, 0, P.ground_h(hx) + 130, 0, 0, 0, 1, 1, 1))
    time.sleep(2.5)
    subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
    path = os.path.join(OUT, "coats_%s.png" % name)
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)

ue.call(ue.E, "StopPIE")
time.sleep(2)
print(ue.job([{"op": "delete", "folder": FOLDER}], timeout=300))
