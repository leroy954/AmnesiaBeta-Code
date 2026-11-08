"""Checks the last details of the skills in PIE: the HUD hint for unspent points, the blue fireball of the dragon with an ice sword,
the pop of the dragon's shield, and the skill screen with the plain body font.
Usage: python skills_misc_test.py <out folder>"""
import os
import sys
import time

from PIL import Image

import ue
import cmdlib
import pie_lib as P

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)


P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
hud = P.pie("HUD")[0]

ue.props(skills, BonusPoints=60)
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=2)
time.sleep(2.5)
shot("hud_points")

ue.props(hud, bDebugToggleSkills=True)
time.sleep(2.5)
shot("screen_body_font")
ue.props(hud, bDebugToggleSkills=True)
time.sleep(1.5)

# the enemy in range, the dragon spits its first ice fire at it
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=P.live_hero(), xform=ue.xform(t["x"] - 600, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        break
cmdlib.cmd(["slomo 0.2"], world="pie")
for i in range(7):
    shot("dragon_fire_%d" % i)

cmdlib.cmd(["slomo 0.12"], world="pie")
ue.props(skills, bDebugGuardPop=True)
for i in range(3):
    shot("guard_pop_%d" % i)

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")
