"""Fights an orc in a running game with the sound log on and prints the sounds that played near it: its battle roars, the hurt growls, the metal hits, the
death roar and the fall. The orc is nearly dead when the hero swings, so the death plays too. Starts a new game is not needed: nothing is saved.
Usage: python orc_sound_test.py [label part, default Orc]"""
import os
import re
import subprocess
import sys
import time

import ue
import pie_lib as P

WHO = sys.argv[1] if len(sys.argv) > 1 else "Orc"
LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Saved", "Logs", "AmnesiaBeta.log")

subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=False)
time.sleep(4)
hero = P.live_hero()
hud = P.pie("HUD")[0]
ue.props(hud, bDebugLogSounds=True)

orc = None
n = 0
for e in P.pie("Ent_"):
    if WHO.lower() in e["refPath"].lower() or WHO.lower() in ue.call(ue.A, "get_label", actor=e).lower():
        orc = e
    else:
        ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        n += 1

o = ue.call(ue.A, "get_actor_transform", actor=orc)["location"]
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(o["x"] - 900, 0, o["z"] + 120, 0, 0, 0, 1, 1, 1))
mark = os.path.getsize(LOG)
time.sleep(2.5)

# the hero hits hard: one swing kills the orc, and its roar and fall are heard
ue.props(hero, MeleeDamage=200)
for _ in range(6):
    hero = P.live_hero()
    try:
        t = ue.call(ue.A, "get_actor_transform", actor=orc)["location"]
    except RuntimeError:
        break  # dead and gone
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 130, 0, t["z"] + 20, 0, 0, 0, 1, 1, 1))
    ue.props(hero, bDebugGroundSlash=True)
    time.sleep(0.7)
time.sleep(3)

with open(LOG, "rb") as f:
    f.seek(mark)
    text = f.read().decode("utf-8", "replace")
sounds = re.findall(r"Sound: (\S+) at X (-?\d+)", text)
for name, x in sounds:
    print("%-28s X %s" % (name, x))
ue.call(ue.E, "StopPIE")
