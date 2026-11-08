"""Fights with every weapon in a running game through the attack test flags of the hero (bDebugGroundSlash, bDebugHeavySlash): the hit points the swings of the
ground combo take from an enemy, then the heavy attack with the special of the weapon (every skill is learned), with pictures of the heavy attack and a count of
the enemies hurt around the target. The hero gets a lot of hit points, and the enemy too, so nothing dies and the numbers can be compared.
Starts a new game at the end, so the weapons and the skills of the test do not stay in the saved progress.
Usage: python weapons_combat_test.py <out folder> [weapon ids...]    (default: sword spear axe wand)"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
WEAPONS = sys.argv[2:] or ["sword", "spear", "axe", "wand"]
os.makedirs(OUT, exist_ok=True)


def shot(name, box=(0.139, 0.133, 0.781, 0.963)):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * box[0]), int(h * box[1]), int(w * box[2]), int(h * box[3]))).save(path)
    return path


def hp(actor):
    """The hit points of an enemy, 0 for one that is gone."""
    try:
        return ue.get_props(actor, "CurrentHP")["CurrentHP"]
    except RuntimeError:
        return 0.0


def at(actor):
    return ue.call(ue.A, "get_actor_transform", actor=actor)["location"]


subprocess.run([sys.executable, "close_msglog.py"])
if not P.start_play():
    raise SystemExit("the play session came up as a spectator")
ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(4)
hero = P.live_hero()
comps = ue.call(ue.A, "get_components", actor=hero)
skills = [c for c in comps if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)

# the enemies of the first zones, nearest first: one fresh enemy for each weapon, since they die after a few hits
candidates = sorted([e for e in P.pie("Ent_") if 2000 < at(e)["x"] < 20500], key=lambda e: at(e)["x"])
print("candidates:", len(candidates), [round(at(e)["x"]) for e in candidates])


def stand(target, distance):
    t = at(target)
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - distance, 0, t["z"] + 60, 0, 0, 0, 1, 1, 1))


for number, weapon in enumerate(WEAPONS):
    target, neighbor = candidates[number * 2], candidates[number * 2 + 1]

    # a second enemy stands right behind the first, so the area attacks have something more to hit
    t = at(target)
    ue.call(ue.A, "set_actor_transform", actor=neighbor, xform=ue.xform(t["x"] + 230, 0, t["z"] + 10, 0, 0, 0, 1, 1, 1))
    ue.props(hero, DebugWeapon=weapon)
    time.sleep(1.8)
    stand(target, 170.0)
    time.sleep(1.5)
    print("== %s (enemy x %.0f, hp %.1f)" % (weapon, at(target)["x"], hp(target)))

    # the first swing of the ground combo
    before = hp(target)
    ue.props(hero, bDebugGroundSlash=True)
    time.sleep(1.3)
    print("   swing 1 took %.2f hp" % (before - hp(target)))

    # the heavy attack with the special: who around the target is hurt
    stand(target, 170.0)
    time.sleep(1.0)
    around = [target, neighbor]
    before = [hp(e) for e in around]
    x0 = at(hero)["x"]
    ue.props(hero, bDebugHeavySlash=True)
    time.sleep(0.45)
    shot("%s_heavy_a" % weapon)
    time.sleep(0.35)
    shot("%s_heavy_b" % weapon)
    time.sleep(1.2)
    after = [hp(e) for e in around]
    print("   heavy: hero moved %.0f cm; hp taken by the target and the one behind it:" % (at(hero)["x"] - x0), ["%.1f" % (b - a) for b, a in zip(before, after)])
    time.sleep(1.0)

ue.call(ue.E, "StopPIE")

# a new game, so the progress of the test does not stay
time.sleep(3)
if not P.start_play():
    raise SystemExit("the play session came up as a spectator")
ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(4)
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
