"""Looks at retargeted combat animations on the hero in a running game: the camera comes close, a weapon goes in the hand (every skill is learned, so
it is owned), each animation plays slowly through the debug slash and pictures are taken while it plays; one contact sheet at the end.
A new game at the end keeps the saved progress as it was.
Usage: python combat_anim_look.py <out folder> <weapon id> [--rate 0.3] [--shots 6] <asset path> [...]   (paths like Game/X/AS_Y.AS_Y, no leading slash)"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

args = sys.argv[1:]
OUT, WEAPON = args.pop(0), args.pop(0)
RATE = float(args[args.index("--rate") + 1]) if "--rate" in args else 0.3
SHOTS = int(args[args.index("--shots") + 1]) if "--shots" in args else 6
PATHS = [(a if a.startswith("/") else "/" + a) for i, a in enumerate(args) if not a.startswith("--") and (i == 0 or args[i - 1] not in ("--rate", "--shots"))]
os.makedirs(OUT, exist_ok=True)
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
tiles = []


def shot(label):
    path = os.path.join(OUT, "tmp.png")
    ue.shot(path)
    im = Image.open(path).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.24), int(h * 0.10), int(w * 0.69), int(h * 0.70)))
    im.thumbnail((420, 520))
    tiles.append((label, im))


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
ue.props(hero, DebugWeapon=WEAPON)
time.sleep(2)

# the first debug slash after the weapon change does not always start: play one quick swing first
ue.props(hero, GroundSlashAnimation=PATHS[0])
ue.props(hero, DebugSlashStartDurationRate={"x": 0.0, "y": 0.5, "z": 1.0})
ue.props(hero, bDebugSlash=True)
time.sleep(1.5)

for path in PATHS:
    name = path.split("/")[-1].split(".")[0]
    asset = ue.call(ue.AS, "load_asset", asset_path=path)
    length = ue.get_props(asset, "SequenceLength")["SequenceLength"]
    ue.props(hero, GroundSlashAnimation=path)
    ue.props(hero, DebugSlashStartDurationRate={"x": 0.0, "y": float(length), "z": RATE})
    ue.props(hero, bDebugSlash=True)
    start = time.time()
    real = length / RATE

    for i in range(SHOTS):
        due = start + real * (i + 0.5) / SHOTS
        time.sleep(max(0.0, due - time.time()))
        shot("%s  %.2f s" % (name, (time.time() - start) * RATE))

    time.sleep(1.2)
    print("done", name, "%.2f s" % length)

ue.call(ue.E, "StopPIE")

# a new game, so the weapons and skills of the test do not stay in the saved progress
time.sleep(3)
if P.start_play():
    ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(4)
    ue.call(ue.E, "StopPIE")

cols = SHOTS
tw, th = 420, 520
rows = (len(tiles) + cols - 1) // cols
sheet = Image.new("RGB", (cols * tw, rows * th), (20, 20, 24))
d = ImageDraw.Draw(sheet)
for i, (label, im) in enumerate(tiles):
    sheet.paste(im, ((i % cols) * tw, (i // cols) * th))
    d.text(((i % cols) * tw + 6, (i // cols) * th + 4), label, fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
os.remove(os.path.join(OUT, "tmp.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
