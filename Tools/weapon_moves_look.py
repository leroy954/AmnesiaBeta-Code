"""Films the left button moves of the spear and the wand in a running game: the spear combo (three taps, one after the other) and two wand taps,
a picture every few tenths of a second, with how far the hero stepped. The enemies are parked. The weapons are put in the hand with the
DebugWeapon test flag after every skill is learned (that owns the weapons); a new game at the end keeps the saved progress. Usage: python weapon_moves_look.py <out folder>"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def crop(path):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
    w, h = im.size
    return im.crop((int(w * 0.30), int(h * 0.35), int(w * 0.85), int(h * 0.88))).resize((440, 300))


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
hero = P.start_pie(park_enemies=True)
time.sleep(3)
hero = P.live_hero()
home = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
rows = []
for weapon, taps, gap in (("spear", 3, 0.45), ("wand", 2, 0.75)):
    ue.props(hero, DebugWeapon=weapon)
    time.sleep(1.0)
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(home["x"], 0, home["z"], 0, 0, 0, 1, 1, 1))
    time.sleep(0.8)
    x0 = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
    frames = []
    for tap in range(taps):
        ue.props(hero, bDebugGroundSlash=True)
        t0 = time.time()
        while time.time() - t0 < gap:
            p = os.path.join(OUT, "%s_%d_%d.png" % (weapon, tap, len(frames)))
            ue.shot(p)
            frames.append(("%s tap %d +%.2fs" % (weapon, tap + 1, time.time() - t0), crop(p)))
    x1 = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
    print("%s: %d pictures, the hero moved %.0f cm" % (weapon, len(frames), x1 - x0))
    rows.append(frames)
ue.call(ue.E, "StopPIE")

# a new game, so the weapons and skills of the test do not stay in the saved progress
time.sleep(3)
if P.start_play():
    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)
    ue.call(ue.E, "StopPIE")

cols = max(len(r) for r in rows)
sheet = Image.new("RGB", (cols * 440, len(rows) * 300), (20, 20, 24))
d = ImageDraw.Draw(sheet)
for r, frames in enumerate(rows):
    for c, (label, im) in enumerate(frames):
        sheet.paste(im, (c * 440, r * 300))
        d.text((c * 440 + 6, r * 300 + 4), label, fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
