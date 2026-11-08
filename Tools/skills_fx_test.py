"""Looks at the effects of the skills in slow motion: the frost nova ring and flames, a frozen, a chilled and a burning enemy, the flame burst.
Usage: python skills_fx_test.py <out folder> [steps]     steps: nova freeze chill burn burst   (default: all)"""
import os
import sys
import time

from PIL import Image

import ue
import cmdlib
import pie_lib as P

OUT = sys.argv[1]
STEPS = sys.argv[2:] or ["nova", "freeze", "chill", "burn", "burst"]
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)


def component():
    hero = P.live_hero()
    return [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]


def face_enemy():
    """Hero 420 cm in front of the nearest enemy that is far down the level, so it is awake and calm."""
    hero = P.live_hero()
    for e in P.pie("Ent_"):
        t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
        if t["x"] > 2000:
            ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
            return e
    return None


P.start_pie(park_enemies=False)
time.sleep(4)
skills = component()
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=2)
enemy = face_enemy()
print("enemy:", enemy["refPath"].split(":")[-1] if enemy else None)
time.sleep(3)
cmdlib.cmd(["slomo 0.12"], world="pie")

if "nova" in STEPS:
    ue.props(skills, bDebugNovaEffect=True)
    for i in range(4):
        shot("nova_%d" % i)

if "freeze" in STEPS:
    ue.props(skills, bDebugFreeze=True)
    time.sleep(1.5)
    mesh = [c for c in ue.call(ue.A, "get_components", actor=enemy) if c["refPath"].endswith(".CharacterMesh0")][0]
    before = ue.call(ue.A, "get_actor_transform", actor=enemy)["location"]
    print("anim rate scale of the frozen enemy:", ue.get_props(mesh, "GlobalAnimRateScale"), "at x", round(before["x"]))
    for i in range(2):
        shot("freeze_%d" % i)
    after = ue.call(ue.A, "get_actor_transform", actor=enemy)["location"]
    print("moved while frozen: %.0f cm" % abs(after["x"] - before["x"]))

if "chill" in STEPS:
    ue.props(skills, bDebugChill=True)
    shot("chill_0")

if "burn" in STEPS:
    ue.props(skills, DebugElement=1)
    ue.props(skills, bDebugIgnite=True)
    for i in range(2):
        shot("burn_%d" % i)

if "burst" in STEPS:
    ue.props(skills, bDebugFlameBurst=True)
    for i in range(3):
        shot("burst_%d" % i)

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")
