"""Tries guard poses on the hero in a running game: for each (animation, moment) the guard is switched on with that held frame, and a close picture of
the hero goes into one contact sheet, so the best looking guard can be picked by eye. With "anim <asset path> [step]" it shows the frames of any animation. With "spear" as the second argument it shows the frames of the
six spear attacks instead, to pick the parts of them that make good thrusts. Usage: python guard_pose_sweep.py <out folder> [spear]"""
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

import ue
import pie_lib as P

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def obj(path):
    return path + "." + path.rsplit("/", 1)[1]


FAL = "/Game/FreeAnimationLibrary/Animations/"
MAGE = "/Game/MageBundleSamples/Animations/"
CANDIDATES = [
    (obj(MAGE + "01_PowerUp/MANNY/AS_Mage_Powerup_Light_01"), [None]),  # the current guard, for comparison (0.7 s)
    (obj("/Game/ElfArden/Animations/AS_attack_weapon"), [0.05, 0.12, 0.2, 0.35, 0.5]),
    (obj(FAL + "Counter-Finisher/anim_Counter_Attack_Idle"), [0.3]),
    (obj(FAL + "Counter-Finisher/anim_Counter_Attack_01"), [0.1, 0.25, 0.4]),
    (obj(FAL + "Counter-Finisher/anim_Counter_Attack_08"), [0.1, 0.25, 0.4]),
    (obj(FAL + "Counter-Finisher/anim_Counter_Victim_01"), [0.15, 0.3]),
    (obj(FAL + "Update_2/Spear/AS_Spear_Idle"), [0.3]),
    (obj(MAGE + "02_Ability/MANNY/AS_Mage_Ability_Spell_HighRtLight_02"), [0.3, 0.5]),
    (obj(MAGE + "03_Elemental/MANNY/AS_Mage_Elemental_Water_Conjure_01"), [0.3, 0.5]),
]

if len(sys.argv) > 2 and sys.argv[2] == "spear":
    CANDIDATES = [(obj(FAL + "Update_2/Spear/AS_Spear_Attack_%02d" % n), [0.05, 0.15, 0.25, 0.35, 0.45, 0.6, 0.75, 0.9]) for n in range(1, 7)]
if len(sys.argv) > 2 and sys.argv[2] == "spearfine":
    # the first one and a half seconds of the four attacks that start with a thrust, in steps of 0.1 s
    CANDIDATES = [(obj(FAL + "Update_2/Spear/AS_Spear_Attack_%02d" % n), [("s", round(0.1 * i, 2)) for i in range(16)]) for n in (2, 4, 5, 6)]

if len(sys.argv) > 3 and sys.argv[2] == "anim":
    # any animation: python guard_pose_sweep.py <out> anim <asset path> [step seconds]
    STEP = float(sys.argv[4]) if len(sys.argv) > 4 else 0.1
    sys.argv[3] = "/" + sys.argv[3].lstrip("/")  # Git Bash turns a leading /Game into a Windows path, so it comes without the slash
    LENGTH = ue.get_props({"refPath": sys.argv[3]}, "SequenceLength")["SequenceLength"]
    CANDIDATES = [(sys.argv[3], [("s", round(STEP * i, 2)) for i in range(min(int(LENGTH / STEP) + 1, 40))])]

subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
hero = P.start_pie(park_enemies=True)
time.sleep(3)
hero = P.live_hero()

cmc = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith("CharMoveComp")][0]
home = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]
if len(sys.argv) > 2 and sys.argv[2].startswith("spear"):
    ue.props(hero, DebugWeapon="spear")
    time.sleep(1.0)


def guarding():
    # the guard stops the turning with the movement, so this flag shows it
    return not ue.get_props(cmc, "bOrientRotationToMovement")["bOrientRotationToMovement"]


tiles = []
for path, fractions in CANDIDATES:
    try:
        length = ue.get_props({"refPath": path}, "SequenceLength")["SequenceLength"]
    except RuntimeError as e:
        print("missing", path, str(e)[:120])
        continue
    for f in fractions:
        t = 0.7 if f is None else (f[1] if isinstance(f, tuple) else round(length * f, 2))
        if guarding():
            ue.props(hero, bDebugGuard=True)
            time.sleep(0.3)
        # side on, facing right, as the game camera sees the hero
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(home["x"], 0, home["z"], 0, 0, 0, 1, 1, 1))
        ue.props(hero, GuardAnimationPath=path, GuardPoseTime=t)
        ue.props(hero, bDebugGuard=True)
        time.sleep(0.6)
        name = "%s_%.2f" % (path.rsplit(".", 1)[1], t)
        p = os.path.join(OUT, name + ".png")
        ue.shot(p)
        ok = guarding()
        im = Image.open(p).convert("RGB")
        w, h = im.size
        im = im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963)))
        w, h = im.size
        tiles.append((name + ("" if ok else " NOT GUARDING"), im.crop((int(w * 0.36), int(h * 0.40), int(w * 0.66), int(h * 0.88))).resize((300, 360))))
        ue.props(hero, bDebugGuard=True)
        time.sleep(0.3)
        print(name, ok)

ue.call(ue.E, "StopPIE")
cols = 10 if len(sys.argv) > 2 and sys.argv[2] == "anim" else 16 if len(sys.argv) > 2 and sys.argv[2] == "spearfine" else 8 if len(sys.argv) > 2 else 6
sheet = Image.new("RGB", (cols * 300, ((len(tiles) + cols - 1) // cols) * 360), (20, 20, 24))
d = ImageDraw.Draw(sheet)
for i, (name, im) in enumerate(tiles):
    sheet.paste(im, ((i % cols) * 300, (i // cols) * 360))
    d.text(((i % cols) * 300 + 4, (i // cols) * 360 + 4), "%d %s" % (i, name[-28:]), fill=(255, 255, 0))
sheet.save(os.path.join(OUT, "sheet.png"))
print("sheet:", os.path.join(OUT, "sheet.png"))
