"""Where does the blade go during the sword animation? Plays the whole AS_attack_weapon as a swing with the hero standing still,
reads the blade positions the hero writes to the log (bDebugLogBlade), and prints them against the animation time.
Positions are centimetres from the hero's centre: x forward, z up.
Usage: python blade_path.py [start] [length] [rate] [step]     (default 0 3.3 1 0.049)"""
import os
import re
import sys
import time

import ue
from pie_lib import pie, start_play, live_hero, HERE

PROJECT = os.path.dirname(HERE)
LOG = os.path.join(PROJECT, "Saved", "Logs", "AmnesiaBeta.log")
START = float(sys.argv[1]) if len(sys.argv) > 1 else 0.0
LENGTH = float(sys.argv[2]) if len(sys.argv) > 2 else 3.3
RATE = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
STEP = float(sys.argv[4]) if len(sys.argv) > 4 else 0.049      # print a row every STEP animation seconds
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}

ue.props(PERF, bThrottleCPUWhenNotForeground=False)
try:
    start_play()
    ue.props(pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(3)
    hero = live_hero()
    for n, e in enumerate(pie("Ent_") + pie("Skeleton") + [g for g in pie("Golem") if "Giant" not in g["refPath"]]):
        try:
            ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        except RuntimeError:
            pass
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(15000, 0, 300, 0, 0, 0, 1, 1, 1))
    time.sleep(2.5)                                          # lands and stands still
    mark = os.path.getsize(LOG)
    ue.props(hero, bDebugLogBlade=True)
    time.sleep(0.3)
    ue.props(hero, DebugSlashStartDurationRate={"x": START, "y": LENGTH, "z": RATE}, bDebugSlash=True)
    time.sleep(LENGTH / RATE + 0.6)
    ue.props(hero, bDebugLogBlade=False)
    time.sleep(1.0)
finally:
    try:
        ue.call(ue.E, "StopPIE")
    except Exception:
        pass
    ue.props(PERF, bThrottleCPUWhenNotForeground=True)

time.sleep(1.0)
with open(LOG, "r", encoding="utf-8", errors="replace") as f:
    f.seek(mark)
    text = f.read()

t0 = None
rows = []
for line in text.splitlines():
    m = re.search(r"Swing starts t=([\d.]+), animation from ([\d.]+), rate ([\d.]+)", line)
    if m:
        t0, a0, rate = float(m.group(1)), float(m.group(2)), float(m.group(3))
        continue
    m = re.search(r"Blade t=([\d.]+) base=(-?\d+),(-?\d+) tip=(-?\d+),(-?\d+)", line)
    if m and t0 is not None:
        t = float(m.group(1))
        rows.append((a0 + (t - t0) * rate, int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))

print("%d blade samples" % len(rows))
print("anim s   base x,z    tip x,z")
last = -1.0
for a, bx, bz, tx, tz in rows:
    if a - last >= STEP:
        print("%6.2f   %5d,%5d  %5d,%5d" % (a, bx, bz, tx, tz))
        last = a
