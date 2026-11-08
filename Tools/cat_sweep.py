"""Finds the places where a sleeping cat can be seen from the game camera. A test cat (a flat magenta shape, see make_cat_probe.py) lies at every candidate
place of the zones; the hero stands next to each in the running game, the game picture is taken and the magenta pixels are counted: that is the area of the cat
that is not hidden by grass, rocks or the foreground. The best places of every zone are looked at again from the other sides of the cat (the hero left and right
of it, near and far), the worst of those counts ranks them. The test cats are removed afterwards.
A hazy zone (the snow) washes the magenta out: give a threshold below 100 and a pixel then only needs a red and a blue that are about equal and above the green.
Usage: python cat_sweep.py <out folder> <first|long> <zones: all, or forest,village,... each with an own range as zone:x0:x1> [step] [scale] [y] [threshold, default 110] [views per zone, default 5]"""
import json
import os
import subprocess
import sys
import time

from PIL import Image, ImageChops, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
WORLD = sys.argv[2]
ZONES = sys.argv[3].split(",") if len(sys.argv) > 3 else ["all"]
STEP = float(sys.argv[4]) if len(sys.argv) > 4 else 250.0
SCALE = float(sys.argv[5]) if len(sys.argv) > 5 else 1.8
Y = float(sys.argv[6]) if len(sys.argv) > 6 else -25.0
THRESHOLD = int(sys.argv[7]) if len(sys.argv) > 7 else 110
TOP = int(sys.argv[8]) if len(sys.argv) > 8 else 5
FOLDER = "Temp/CatTest"
PROBE = "/Game/Gen/CatProbe/"
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT, exist_ok=True)

RANGES = {
    "first": {"forest": (800, 8500), "village": (9800, 20500), "snow": (22500, 31500), "ruins": (33000, 41300), "cave": (43500, 53000), "storm": (55000, 63500)},
    "long": {"forest": (170000, 190000), "village": (282000, 302000), "snow": (402000, 422000), "ruins": (514000, 534000), "cave": (614000, 634000),
             "storm": (734000, 754000)},
}[WORLD]
if ZONES != ["all"]:
    chosen = {}

    for token in ZONES:
        name, *span = token.split(":")
        chosen[name] = (float(span[0]), float(span[1])) if span else RANGES[name]

    RANGES = chosen

items = []
for zone, (a, b) in RANGES.items():
    x = float(a)
    while x <= b:
        items.append((zone, x))
        x += STEP
print(len(items), "candidates")


def label(zone, x):
    return "TestCat_%s_%d" % (zone, x)


ue.job([{"op": "delete", "folder": FOLDER}], timeout=300)
for start in range(0, len(items), 80):
    ops = []
    for zone, x in items[start:start + 80]:
        ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaCat", "loc": [x, Y, P.ground_h(x)], "rot": [0.0, 0.0, 0.0], "label": label(zone, x),
                    "folder": FOLDER, "props": {"CatId": "t_%s_%d" % (zone, x), "Coat": 1, "CatScale": SCALE, "CoatFolder": PROBE}})
    ue.job(ops, timeout=300)


def shot(path):
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    return im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))


def magenta(im):
    """The number of magenta pixels of the picture and their box: red and blue are both far above green."""
    r, g, b = im.split()
    m = ImageChops.darker(ImageChops.subtract(r, g), ImageChops.subtract(b, g)).point(lambda v: 255 if v > THRESHOLD else 0)

    if THRESHOLD < 100:
        m = ImageChops.darker(m, ImageChops.difference(r, b).point(lambda v: 255 if v < 60 else 0))

    return m.histogram()[255], m.getbbox()


def put_hero(x, dx):
    ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(x + dx, 0, P.ground_h(x + dx) + 130, 0, 0, 0, 1, 1, 1))
    time.sleep(1.4)


subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
P.start_pie(park_enemies=True)
time.sleep(3)
tmp = os.path.join(OUT, "tmp.png")
cats = {}
for a in P.pie("TestCat_"):
    cats[a["refPath"]] = a
print(len(cats), "test cats in the game")
meshes = {}
for zone, x in items:
    found = P.pie(label(zone, x))
    if found:
        meshes[(zone, x)] = {"refPath": found[0]["refPath"] + ".Mesh"}
for ref in meshes.values():
    ue.props(ref, bHiddenInGame=True)
print("all hidden")


def look(zone, x, dx):
    """The magenta pixel count, its box and the picture of the cat at (zone, x) seen with the hero dx next to it."""
    ref = meshes[(zone, x)]
    ue.props(ref, bHiddenInGame=False)
    put_hero(x, dx)
    im = shot(tmp)
    ue.props(ref, bHiddenInGame=True)
    n, box = magenta(im)
    return n, box, im


rows = []
for zone, x in items:
    if (zone, x) not in meshes:
        continue
    try:
        n, box, im = look(zone, x, -230.0)
    except RuntimeError as e:
        print("failed", zone, x, str(e)[:100])
        continue
    rows.append({"zone": zone, "x": x, "n": n})
    print("%-8s %8.0f  %5d" % (zone, x, n), flush=True)

ref_n = max(r["n"] for r in rows) if rows else 1
print("largest area", ref_n)
json.dump(rows, open(os.path.join(OUT, "pass1_%s.json" % WORLD), "w"))

# the best of every zone from the other sides too
final = []
for zone in RANGES:
    best = sorted([r for r in rows if r["zone"] == zone], key=lambda r: -r["n"])[:TOP]
    for r in best:
        counts = [r["n"]]
        for dx in (230.0, -700.0, 700.0):
            n, box, im = look(zone, r["x"], dx)
            counts.append(n)
        r["counts"] = counts
        r["worst"] = min(counts)
        final.append(r)
        print("%-8s %8.0f  %s" % (zone, r["x"], counts), flush=True)

ue.call(ue.E, "StopPIE")
time.sleep(2)
ue.job([{"op": "delete", "folder": FOLDER}], timeout=300)
print("--- ranking (worst count of four views, share of the largest area)")
tiles = []
for zone in RANGES:
    lst = sorted([r for r in final if r["zone"] == zone], key=lambda r: -r["worst"])
    print(zone, [(int(r["x"]), r["worst"], "%d%%" % (100 * r["worst"] / ref_n)) for r in lst])
json.dump(final, open(os.path.join(OUT, "final_%s.json" % WORLD), "w"))
