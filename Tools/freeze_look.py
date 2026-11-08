"""Looks at the ice of the frost nova in slow motion: the hero casts the nova (the debug flag of the skill component) 420 cm in front of an enemy, and the
game picture is taken while the crystals grow, while the enemy stands frozen, and when the ice breaks. The frost actors are counted too.
Usage: python freeze_look.py <out folder> [slow motion factor, default 0.2; 1 means none] [distance, default 420] [shot times in seconds after the cast]"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import cmdlib
import pie_lib as P

OUT = sys.argv[1]
SLOW = float(sys.argv[2]) if len(sys.argv) > 2 else 0.2
DISTANCE = float(sys.argv[3]) if len(sys.argv) > 3 else 420.0
os.makedirs(OUT, exist_ok=True)


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).save(path)
    return path


subprocess.run([sys.executable, "close_msglog.py"])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
print("nova freeze time of the component:", ue.get_props(skills, "FrostNovaFreezeTime"))
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
print("enemy", enemy["refPath"].split(":")[-1], "frost actors before:", len(P.pie("AmnesiaFrost")))
shot("before")

if SLOW != 1.0:
    cmdlib.cmd(["slomo %s" % SLOW], world="pie")
ue.props(skills, bDebugFrostNova=True)
t0 = time.time()
# the shots follow each other as fast as the capture goes, then wait for the steady state and the break (game seconds = real seconds * SLOW)
plan = [float(v) for v in (sys.argv[4] if len(sys.argv) > 4 else "0,0.6,1.5,3.0,4.0,4.3,4.5,4.7,4.9,5.2,5.6,6.2").split(",")]
for i, at in enumerate(plan):
    while time.time() - t0 < at:
        time.sleep(0.05)
    real = time.time() - t0
    rate = ue.get_props(mesh, "GlobalAnimRateScale")["GlobalAnimRateScale"]
    frost = P.pie("AmnesiaFrost")
    shot("freeze_%02d" % i)
    print("shot %02d real %.1f s (game %.2f s): anim rate %s, frost actors %d" % (i, real, real * SLOW, rate, len(frost)))

cmdlib.cmd(["slomo 1"], world="pie")
ue.call(ue.E, "StopPIE")
