"""Looks at the guard from the side in a running game: the hero guards next to the first orc with the orc on its right, then on its left, then walks
away while guarding (D held). The other enemies are parked. The guard state is read from the movement (the guard stops turning with the walk).
Usage: python guard_look.py <out folder>"""
import ctypes
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P
from guard_move_test import editor_window

user32 = ctypes.windll.user32
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def crop(path):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    w, h = im.size
    return im.crop((int(w * 0.25), int(h * 0.35), int(w * 0.80), int(h * 0.90))).resize((550, 360))


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
orc = None
n = 0
for e in P.pie("Ent_"):
    if "Orc_01" in ue.call(ue.A, "get_label", actor=e):
        orc = e
    else:
        ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        n += 1
hero = P.live_hero()
cmc = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith("CharMoveComp")][0]


def guarding():
    return not ue.get_props(cmc, "bOrientRotationToMovement")["bOrientRotationToMovement"]


tiles = []
ue.props(hero, bDebugGuard=True)  # the right button goes down and stays down
for side, dx in (("orc on the right", -330), ("orc on the left", 330)):
    o = ue.call(ue.A, "get_actor_transform", actor=orc)["location"]
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(o["x"] + dx, 0, o["z"] + 20, 0, 0, 0, 1, 1, 1))
    time.sleep(1.2)
    p = os.path.join(OUT, side.replace(" ", "_") + ".png")
    ue.shot(p)
    tiles.append(("%s, guard=%s" % (side, guarding()), crop(p)))

# walk away from the orc (it is on the left now) with D held, still guarding
hwnd = editor_window()
t0 = time.time()
first = True
shot_taken = False
while time.time() - t0 < 1.0:
    user32.PostMessageW(hwnd, 0x0100, 0x44, 1 | (0x20 << 16) | (0 if first else (1 << 30)))
    first = False
    time.sleep(0.1)
    if time.time() - t0 > 0.6 and not shot_taken:
        p = os.path.join(OUT, "walking.png")
        ue.shot(p)
        tiles.append(("walking away, guard=%s" % guarding(), crop(p)))
        shot_taken = True
user32.PostMessageW(hwnd, 0x0101, 0x44, ctypes.c_long((1 | (0x20 << 16) | (1 << 30) | (1 << 31)) - (1 << 32)).value)
ue.call(ue.E, "StopPIE")

sheet = Image.new("RGB", (3 * 550, 360))
d = ImageDraw.Draw(sheet)
for i, (label, im) in enumerate(tiles):
    sheet.paste(im, (i * 550, 0))
    d.text((i * 550 + 6, 4), label, fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
