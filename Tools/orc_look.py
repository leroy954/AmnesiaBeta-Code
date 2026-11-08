"""Looks at the Orc Warrior in a running game: the hero is put a few metres from it, the orc notices, walks or runs up and swings. A picture every 0.4 s goes into
one contact sheet, with the distance to the orc and its position printed. The other enemies are parked far away. A new game at the end keeps the saved progress.
Usage: python orc_look.py <out folder> [distance, default 650] [orc label part, default Orc]"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
DISTANCE = float(sys.argv[2]) if len(sys.argv) > 2 else 650.0
WHO = sys.argv[3] if len(sys.argv) > 3 else "Orc"
os.makedirs(OUT, exist_ok=True)


def loc(actor):
    t = ue.call(ue.A, "get_actor_transform", actor=actor)
    return t["location"] if "location" in t else t


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    im.save(path)
    return im


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()

orc = None
n = 0
for e in P.pie("Ent_"):
    label = ue.call(ue.A, "get_label", actor=e)
    if orc is None and (WHO.lower() in e["refPath"].lower() or WHO.lower() in label.lower()):
        orc = e
        print("watching", label)
    else:
        ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        n += 1
if not orc:
    raise SystemExit("no enemy with %r in the label" % WHO)

o = loc(orc)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(o["x"] - DISTANCE, 0, o["z"] + 120, 0, 0, 0, 1, 1, 1))
time.sleep(2.5)

frames = []
for i in range(10):
    im = shot("f%02d" % i)
    ho, oo = loc(P.live_hero()), loc(orc)
    print("t=%.1f hero x %.0f, orc x %.0f z %.0f (gap %.0f)" % (i * 0.4, ho["x"], oo["x"], oo["z"], abs(ho["x"] - oo["x"])))
    w, h = im.size
    frames.append(im.crop((int(w * 0.30), int(h * 0.40), int(w * 0.80), int(h * 0.82))).resize((420, 354)))
    time.sleep(0.05)

cols = 5
sheet = Image.new("RGB", (cols * 420, ((len(frames) + cols - 1) // cols) * 354), (20, 20, 24))
d = ImageDraw.Draw(sheet)
for i, f in enumerate(frames):
    sheet.paste(f, ((i % cols) * 420, (i // cols) * 354))
    d.text(((i % cols) * 420 + 6, (i // cols) * 354 + 4), str(i), fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
ue.call(ue.E, "StopPIE")
print("sheet:", os.path.join(OUT, "sheet.png"))
