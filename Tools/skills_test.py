"""Plays the skills in PIE and takes pictures: the skill screen (empty, partly and fully learned), the sword aura of each element,
the flame burst and the frost nova on an enemy, the burning and the frozen enemy, the HUD with the recharge.
Usage: python skills_test.py <out folder> [steps]    steps: any of screen aura fire ice  (default: all)"""
import os
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
STEPS = sys.argv[2:] or ["screen", "aura", "fire", "ice"]
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    print("shot", name)


def skills_component():
    hero = P.live_hero()
    for c in ue.call(ue.A, "get_components", actor=hero):
        if c["refPath"].endswith(".Skills"):
            return c
    raise RuntimeError("the hero has no Skills component")


def hud():
    return P.pie("HUD")[0]


def toggle_screen():
    ue.props(hud(), bDebugToggleSkills=True)
    time.sleep(2.5)


def near_enemy():
    """Puts the hero 400 cm in front of the nearest awake enemy (facing +x)."""
    enemies = [e for e in P.pie("Ent_") if "L" not in e["refPath"].split("_")[-1][:0]]
    hero = P.live_hero()
    for e in enemies:
        t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
        if t["x"] < 2000:
            continue
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        return e
    return None


hero = P.start_pie(park_enemies=False)
time.sleep(4)
skills = skills_component()
print("skills component:", skills["refPath"].split(".")[-1])

if "screen" in STEPS:
    toggle_screen()
    shot("skills_empty")
    toggle_screen()

    # a few points to spend, then a few skills learned for the mixed look
    try:
        ue.props(skills, BonusPoints=7)
    except Exception as e:
        print("no BonusPoints:", str(e)[:80])
    time.sleep(1)
    toggle_screen()
    shot("skills_points")
    toggle_screen()

if "aura" in STEPS or "fire" in STEPS or "ice" in STEPS:
    ue.props(skills, bDebugLearnAll=True)
    time.sleep(1.5)

if "screen" in STEPS:
    ue.props(skills, DebugElement=1)
    time.sleep(1)
    toggle_screen()
    shot("skills_full")
    toggle_screen()

if "aura" in STEPS:
    for element, name in ((0, "steel"), (1, "fire"), (2, "ice")):
        ue.props(skills, DebugElement=element)
        time.sleep(3.0)
        shot("aura_" + name)

if "fire" in STEPS or "ice" in STEPS:
    enemy = near_enemy()
    print("enemy:", enemy["refPath"].split(":")[-1] if enemy else None)
    time.sleep(2.5)

if "ice" in STEPS:
    ue.props(skills, bDebugFrostNova=True)
    time.sleep(0.45)
    shot("nova_0")
    time.sleep(1.0)
    shot("nova_1")
    time.sleep(3.0)

if "fire" in STEPS:
    ue.props(skills, DebugElement=1)
    time.sleep(0.5)
    ue.props(skills, bDebugFlameBurst=True)
    time.sleep(0.45)
    shot("burst_0")
    time.sleep(1.2)
    shot("burst_1")

ue.call(ue.E, "StopPIE")
