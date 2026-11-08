"""Checks the level row of the admin panel in a running game: opens the panel, changes the level with the same call the buttons make (the HUD test
helper DebugChangeLevel), and takes a picture after each step: up, down, the top and the bottom of the range. Then the panel closes and a level up
shows the banner. The pictures are cut down to the admin panel and put in one sheet. A new game at the end keeps the level out of the saved progress.
Usage: python admin_level_test.py <out folder>"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)
    return path


subprocess.run([sys.executable, os.path.join(HERE, "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
hud = P.pie("HUD")[0]
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(2.5)
paths = [shot("1_start_level1")]

# the steps of the four buttons, and what the level should read after each: 1, 6, 5, 10 (clamped), 5, 1 (clamped)
for name, steps in (("2_plus5", 5), ("3_minus1", -1), ("4_plus5_top", 5), ("5_minus5", -5), ("6_minus5_bottom", -5)):
    ue.props(hud, DebugChangeLevel=steps)
    time.sleep(1.2)
    paths.append(shot(name))

# the panel closes, a level up in the game shows the banner and the new level on the life bar
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(1.5)
ue.props(hud, DebugChangeLevel=3)
time.sleep(1.0)
paths.append(shot("7_game_level4"))
ue.call(ue.E, "StopPIE")

# a sheet: the right part of each picture, where the panel is
ims = []
for p in paths[:-1]:
    im = Image.open(p).convert("RGB")
    w, h = im.size
    ims.append(im.crop((int(w * 0.52), int(h * 0.12), w, int(h * 0.88))))

ims.append(Image.open(paths[-1]).convert("RGB").crop((0, 0, Image.open(paths[-1]).size[0], int(Image.open(paths[-1]).size[1] * 0.6))))
cell_w = max(i.width for i in ims)
cell_h = max(i.height for i in ims)
cols = 4
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (cell_w * cols, cell_h * rows), (20, 16, 12))

for i, im in enumerate(ims):
    sheet.paste(im, ((i % cols) * cell_w, (i // cols) * cell_h))

sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet", sheet.size)

# a new game, so the level of the test does not stay in the saved progress
time.sleep(3)
P.start_pie(park_enemies=False)
time.sleep(4)
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
