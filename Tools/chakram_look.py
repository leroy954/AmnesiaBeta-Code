"""Films the chakram in a running game: the hero holds it, throws it on a tap at three enemies standing in a row (with every skill learned it
bounces between them and comes back), then the held throw that whirls at the far end. A picture every ~0.15 s, the hit points of the enemies
before and after. Every skill is learned (that owns the weapons); a new game at the end keeps the saved progress.
With "slow" as the second argument the disc flies at a third of its speed, so the pictures catch it.
Usage: python chakram_look.py <out folder> [slow]"""
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
    return im.crop((int(w * 0.15), int(h * 0.30), int(w * 0.95), int(h * 0.90))).resize((520, 330))


def hp(actor):
    try:
        return ue.get_props(actor, "CurrentHP")["CurrentHP"]
    except RuntimeError:
        return 0.0


def loc(actor):
    t = ue.call(ue.A, "get_actor_transform", actor=actor)
    return t["location"] if "location" in t else t


subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(hero, DebugWeapon="chakram")
time.sleep(1.0)
if len(sys.argv) > 2 and sys.argv[2] == "slow":
    ue.props(hero, ChakramSpeed=650.0)

# three enemies in a row in front of the hero, the rest parked far away
enemies = sorted(P.pie("Ent_"), key=lambda e: loc(e)["x"])
row = [e for e in enemies if 1500 < loc(e)["x"] < 9000][:3]
n = 0
for e in enemies:
    if e not in row:
        ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        n += 1
for i, e in enumerate(row):
    ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(5650 + i * 300, 0, 320, 0, 180, 0, 1, 1, 1))
# flat ground (the mound of the first zone is at x 2450..4550, its rocks would stop the flight)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(5000, 0, 150, 0, 0, 0, 1, 1, 1))
time.sleep(2.0)

tiles = []
p = os.path.join(OUT, "hold.png")
ue.shot(p)
tiles.append(("in the hand", crop(p)))

# five quick taps: with every skill learned four discs may fly at once, the fifth tap does nothing
counts = []
for tap in range(5):
    ue.props(P.live_hero(), bDebugGroundSlash=True)
    time.sleep(0.5)
    counts.append(len(P.pie("AmnesiaChakram")))
print("discs in the air after each of five quick taps:", counts)
p = os.path.join(OUT, "multi.png")
ue.shot(p)
tiles.append(("five quick taps: %s discs" % counts, crop(p)))
time.sleep(4.0)

for label, flag, seconds in (("tap throw", "bDebugGroundSlash", 1.8), ("held throw (whirl)", "bDebugHeavySlash", 3.2)):
    before = [hp(e) for e in row]
    ue.props(P.live_hero(), **{flag: True})
    t0 = time.time()
    k = 0
    while time.time() - t0 < seconds:
        time.sleep(0.12)
        p = os.path.join(OUT, "%s_%02d.png" % (label.split()[0], k))
        ue.shot(p)
        if k % 2 == 0 or k < 6:
            tiles.append(("%s +%.2fs" % (label, time.time() - t0), crop(p)))
        k += 1
    after = [hp(e) for e in row]
    print("%s: enemy hit points %s -> %s" % (label, ["%.1f" % v for v in before], ["%.1f" % v for v in after]))
    time.sleep(1.0)

ue.call(ue.E, "StopPIE")

# a new game, so the weapons and skills of the test do not stay in the saved progress
time.sleep(3)
if P.start_play():
    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)
    ue.call(ue.E, "StopPIE")

cols = 5
sheet = Image.new("RGB", (cols * 520, ((len(tiles) + cols - 1) // cols) * 330), (20, 20, 24))
d = ImageDraw.Draw(sheet)
for i, (label, im) in enumerate(tiles):
    sheet.paste(im, ((i % cols) * 520, (i // cols) * 330))
    d.text(((i % cols) * 520 + 6, (i // cols) * 330 + 4), label, fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
