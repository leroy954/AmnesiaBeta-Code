"""Looks at the weapons in the hero's hand in a running game: the camera comes close, the weapons are put in the hand one after the other through the
DebugWeapon helper of the hero, and each one is photographed standing still, and then (with --swing) the first swing animation of the weapon is played slowly
through the debug slash of the hero and pictures are taken while it plays. Saved/WeaponTuning.json can be changed between runs without a build.
Usage: python weapons_look.py <out folder> [--swing] [--rate 0.2] [--arm 380] [--low 100] [weapon ids...]    (default weapons: sword spear axe wand)"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

args = sys.argv[1:]
OUT = args.pop(0)
SWING = "--swing" in args
RATE = float(args[args.index("--rate") + 1]) if "--rate" in args else 0.2
ARM = float(args[args.index("--arm") + 1]) if "--arm" in args else 380.0
LOW = float(args[args.index("--low") + 1]) if "--low" in args else 100.0
WEAPONS = [a for i, a in enumerate(args) if not a.startswith("--") and (i == 0 or args[i - 1] not in ("--arm", "--low", "--rate"))] or ["sword", "spear", "axe", "wand"]
os.makedirs(OUT, exist_ok=True)


# the first swing of each weapon: animation, where it starts, how long it is (animation seconds). The sword's own animation is the hero's default
SWINGS = {
    "spear": ("/Game/FreeAnimationLibrary/Animations/Update_2/Spear/AS_Spear_Attack_01.AS_Spear_Attack_01", 0.95, 0.85),
    "wand": ("/Game/MageBundleSamples/Animations/03_Elemental/MANNY/AS_Mage_Elemental_Fire_Volley_01.AS_Mage_Elemental_Fire_Volley_01", 1.35, 0.95),
}


def shot(name, box=(0.139, 0.133, 0.781, 0.963)):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * box[0]), int(h * box[1]), int(w * box[2]), int(h * box[3]))).save(path)
    return path


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
ue.props(boom, TargetArmLength=ARM, TargetOffset={"x": 0.0, "y": 0.0, "z": -LOW})
time.sleep(1.5)
print("arm length now", ue.get_props(boom, "TargetArmLength"))

for weapon in WEAPONS:
    ue.props(hero, DebugWeapon=weapon)
    time.sleep(1.8)
    shot("%s_idle" % weapon)
    print("shot", weapon)

    if SWING:
        if weapon in SWINGS:
            animation, start, duration = SWINGS[weapon]
            ue.props(hero, GroundSlashAnimation=animation)
        else:
            second = ue.get_props(hero, "GroundSlashes")["GroundSlashes"][1]
            start, duration = second["startTime"], second["duration"]
        ue.props(hero, DebugSlashStartDurationRate={"x": float(start), "y": float(duration), "z": RATE})
        ue.props(hero, bDebugSlash=True)
        for i in range(8):
            shot("%s_swing%d" % (weapon, i))
        time.sleep(1.5)

ue.call(ue.E, "StopPIE")
