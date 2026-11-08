"""Casts the frost nova through the debug flag and polls the enemies near the hero (animation rate, position, the hit points) for a few seconds,
to see which of them really freeze. Usage: python freeze_probe.py [distance]"""
import subprocess
import sys
import time

import ue
import pie_lib as P

DISTANCE = float(sys.argv[1]) if len(sys.argv) > 1 else 300.0
subprocess.run([sys.executable, "close_msglog.py"])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
ue.props(skills, DebugElement=2)
enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        enemy = e
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - DISTANCE, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        break
mesh = [c for c in ue.call(ue.A, "get_components", actor=enemy) if c["refPath"].endswith(".CharacterMesh0")][0]
time.sleep(3)
hx = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
ex = ue.call(ue.A, "get_actor_transform", actor=enemy)["location"]["x"]
print("hero x %.0f enemy x %.0f, distance %.0f" % (hx, ex, abs(ex - hx)))
ue.props(skills, bDebugFrostNova=True)
t0 = time.time()
for i in range(16):
    rate = ue.get_props(mesh, "GlobalAnimRateScale")["GlobalAnimRateScale"]
    x = ue.call(ue.A, "get_actor_transform", actor=enemy)["location"]["x"]
    print("t=%.2f rate %s x %.0f" % (time.time() - t0, rate, x))
    time.sleep(0.25)
ue.call(ue.E, "StopPIE")
