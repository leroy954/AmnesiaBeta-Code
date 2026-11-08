"""Looks at each weapon's real combo in a running game: the weapon goes in the hand, the three combo swings are tapped one after the other through
the hero's bDebugGroundSlash (the same path as the button), then the heavy attack through bDebugHeavySlash. The hero runs slowed down
(CustomTimeDilation) so several pictures fit in each swing; the combo reset time is stretched for the test so the swings follow on.
One contact sheet per run (a row per swing); a new game at the end keeps the saved progress as it was.
Usage: python combo_look.py <out folder> [--slow 0.25] [--shots 5] [weapons...]   (default: sword axe wand chakram)"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

args = sys.argv[1:]
OUT = args.pop(0)
SLOW = float(args[args.index("--slow") + 1]) if "--slow" in args else 0.25
SHOTS = int(args[args.index("--shots") + 1]) if "--shots" in args else 5
WEAPONS = [a for i, a in enumerate(args) if not a.startswith("--") and (i == 0 or args[i - 1] not in ("--slow", "--shots"))] or ["sword", "axe", "wand", "chakram"]
os.makedirs(OUT, exist_ok=True)
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
rows = []


def shot():
    path = os.path.join(OUT, "tmp.png")
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.24), int(h * 0.10), int(w * 0.69), int(h * 0.70)))
    im.thumbnail((360, 480))
    return im


def swing(label, flag, seconds):
    ue.props(hero, **{flag: True})
    tiles = []
    start = time.time()

    for i in range(SHOTS):
        time.sleep(max(0.0, start + seconds * (i + 0.6) / SHOTS - time.time()))
        tiles.append(shot())

    rows.append((label, tiles))
    time.sleep(0.6)


ue.props(PERF, bThrottleCPUWhenNotForeground=False)
subprocess.run([sys.executable, "close_msglog.py"])
P.start_pie(park_enemies=True)
time.sleep(4)
hero = P.live_hero()
comps = ue.call(ue.A, "get_components", actor=hero)
skills = [c for c in comps if c["refPath"].endswith(".Skills")][0]
boom = [c for c in comps if c["refPath"].endswith(".CameraBoom")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(-1500, 0, P.ground_h(-1500) + 120, 0, 0, 0, 1, 1, 1))
time.sleep(2)
ue.props(boom, TargetArmLength=420.0, TargetOffset={"x": 0.0, "y": 0.0, "z": -90.0})
ue.props(hero, SlashComboResetTime=60.0)

for weapon in WEAPONS:
    ue.props(hero, DebugWeapon=weapon)
    time.sleep(2.5)
    ue.props(hero, CustomTimeDilation=SLOW)
    taps = 1 if weapon in ("wand", "chakram") else 3

    for i in range(taps):
        swing("%s %d" % (weapon, i + 1), "bDebugGroundSlash", 0.75 / SLOW)

    swing("%s heavy" % weapon, "bDebugHeavySlash", 1.0 / SLOW)
    ue.props(hero, CustomTimeDilation=1.0)
    time.sleep(2.0)
    print("done", weapon)

ue.call(ue.E, "StopPIE")

# a new game, so the weapons and skills of the test do not stay in the saved progress
time.sleep(3)
if P.start_play():
    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)
    ue.call(ue.E, "StopPIE")

tw, th = rows[0][1][0].size
sheet = Image.new("RGB", (SHOTS * tw, len(rows) * th), (20, 20, 24))
d = ImageDraw.Draw(sheet)

for r, (label, tiles) in enumerate(rows):
    for c, im in enumerate(tiles):
        sheet.paste(im, (c * tw, r * th))

    d.text((6, r * th + 4), label, fill=(255, 255, 0))

sheet.save(os.path.join(OUT, "sheet.png"))
os.remove(os.path.join(OUT, "tmp.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
