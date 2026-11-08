"""Shows how fire systems look in the game: each one is set as the frost nova flames of the hero (a test effect with no recharge, flames rolling
out to both sides) and photographed in slow motion. Usage: python vfx_gallery.py <out folder> <first index> <last index> [scale]"""
import base64
import os
import sys
import time

from PIL import Image, ImageDraw

import cmdlib
import ue
import pie_lib as P

OUT = sys.argv[1]
FIRST, LAST = int(sys.argv[2]), int(sys.argv[3])
SCALE = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0
BASE = "/Game/Vefects/Stylized_Fire/VFX/"
NAMES = [
    "Flames/Particles/NS_Stylized_Fire_01", "Flames/Particles/NS_Stylized_Fire_01_Infinite", "Flames/Particles/NS_Stylized_Fire_01_Once",
    "Flames/Particles/NS_Stylized_Fire_02", "Flames/Particles/NS_Stylized_Fire_02_Infinite", "Flames/Particles/NS_Stylized_Fire_02_Once",
    "Flames/Particles/NS_Stylized_Fire_Large_01", "Flames/Particles/NS_Stylized_Fire_Radial_01", "Flames/Particles/NS_Stylized_Fire_Radial_02",
    "Pure/Particles/NS_Stylized_Fire_01_Pure", "Pure/Particles/NS_Stylized_Fire_02_Pure", "Pure/Particles/NS_Stylized_Fire_Radial_01_Pure",
    "Pure/Particles/NS_Stylized_Fire_Radial_02_Pure", "Dark/Particles/NS_Stylized_Fire_01_Dark", "Dark/Particles/NS_Stylized_Fire_02_Dark",
]
os.makedirs(OUT, exist_ok=True)

P.start_pie(park_enemies=False)
time.sleep(3)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, NovaFlameScale=SCALE, NovaRingScale=0.0)
cmdlib.cmd(["slomo 0.4"], world="pie")
tiles = []

for index in range(FIRST, min(LAST, len(NAMES) - 1) + 1):
    path = BASE + NAMES[index] + "." + NAMES[index].split("/")[-1]
    ue.props(skills, ChillEffect=path)
    ue.props(skills, bDebugNovaEffect=True)
    time.sleep(0.5)

    for take in range(2):
        shot = os.path.join(OUT, "g%02d_%d.png" % (index, take))
        ue.shot(shot)
        im = Image.open(shot).convert("RGB")
        w, h = im.size
        im = im.crop((int(w * 0.17), int(h * 0.36), int(w * 0.62), int(h * 0.86)))
        draw = ImageDraw.Draw(im)
        draw.text((6, 6), "%d %s take %d" % (index, NAMES[index].split("/")[-1].replace("NS_Stylized_Fire_", ""), take), fill=(255, 255, 255))
        im.save(shot)
        tiles.append(im)

    time.sleep(7.0)

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")

w, h = tiles[0].size
cols = 4
rows = (len(tiles) + cols - 1) // cols
sheet = Image.new("RGB", (w * cols, h * rows))

for i, im in enumerate(tiles):
    sheet.paste(im, ((i % cols) * w, (i // cols) * h))

sheet.save(os.path.join(OUT, "gallery_%d_%d.png" % (FIRST, LAST)))
print("sheet", sheet.size)
