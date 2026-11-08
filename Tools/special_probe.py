"""Probes the heavy attack special of one weapon in a running game: two enemies stand 230 cm apart in front of the hero, and after the heavy attack the hit points of
both, whether they are dormant, and where they stand are printed every 0.25 s for two seconds.
Usage: python special_probe.py <weapon id> [distance of the neighbor behind the target, default 230] [ground: a ground swing instead of the heavy attack]"""
import subprocess
import sys
import time

import ue
import pie_lib as P

WEAPON = sys.argv[1]
GAP = float(sys.argv[2]) if len(sys.argv) > 2 else 230.0


def props(actor, *names):
    try:
        return ue.get_props(actor, *names)
    except RuntimeError:
        return None


def at(actor):
    return ue.call(ue.A, "get_actor_transform", actor=actor)["location"]


subprocess.run([sys.executable, "close_msglog.py"])
if not P.start_play():
    raise SystemExit("spectator")
ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
# the dragon would add its own fireballs to the numbers
for dragon in P.pie("Companion"):
    try:
        ue.call(ue.S, "remove_from_scene", actor=dragon)
        print("removed the dragon")
    except RuntimeError as error:
        print("could not remove the dragon", str(error)[:80])
cands = sorted([e for e in P.pie("Ent_") if 2000 < at(e)["x"] < 20500], key=lambda e: at(e)["x"])
target, neighbor = cands[2], cands[3]
t = at(target)
ue.call(ue.A, "set_actor_transform", actor=neighbor, xform=ue.xform(t["x"] + GAP, 0, t["z"] + 10, 0, 0, 0, 1, 1, 1))
ue.props(hero, DebugWeapon=WEAPON)
time.sleep(1.8)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 170, 0, t["z"] + 60, 0, 0, 0, 1, 1, 1))
time.sleep(3.0)
print("weapon:", WEAPON, "| hero x %.0f, target x %.0f, neighbor x %.0f" % (at(hero)["x"], at(target)["x"], at(neighbor)["x"]))
print("dormant:", props(target, "bIsDormant"), props(neighbor, "bIsDormant"))
ue.props(hero, bDebugGroundSlash=True if len(sys.argv) > 3 and sys.argv[3] == "ground" else False, bDebugHeavySlash=False if len(sys.argv) > 3 and sys.argv[3] == "ground" else True)
t0 = time.time()
print("melee damage of the hero:", ue.get_props(hero, "MeleeDamage"))
for i in range(16):
    row = []
    for e in (target, neighbor):
        p = props(e, "CurrentHP")
        row.append("hp %s x %.0f" % ("%.1f" % p["CurrentHP"] if p else "gone", at(e)["x"] if p else 0))
    print("t=%.2f  target: %s | neighbor: %s" % (time.time() - t0, row[0], row[1]))
    time.sleep(0.25)
ue.call(ue.E, "StopPIE")
