"""Finds where the swings, thrusts and casts are in long animations: the hero plays the animation in a running game as a ground slash with the blade log on
(where the blade in the hand goes, relative to the hero), and the fast parts of the hand's motion are printed as segments: start, end, peak speed and the
farthest forward reach of the blade tip. Usage: python anim_probe.py <csv out folder> <asset path> [<asset path> ...]   (paths like Game/X/AS_Y.AS_Y, no leading slash)"""
import os
import re
import subprocess
import sys
import time

import ue
import pie_lib as P

OUT = sys.argv[1]
PATHS = [(a if a.startswith('/') else '/' + a) for a in sys.argv[2:]]  # Git Bash rewrites a leading slash, so pass Game/X/AS_Y.AS_Y
LOG = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "Logs", "AmnesiaBeta.log")
os.makedirs(OUT, exist_ok=True)
subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "close_msglog.py")])
P.start_pie(park_enemies=True)
time.sleep(4)
hero = P.live_hero()
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(-1500, 0, P.ground_h(-1500) + 120, 0, 0, 0, 1, 1, 1))
time.sleep(2)

for path in PATHS:
    asset = ue.call(ue.AS, "load_asset", asset_path=path)
    length = ue.get_props(asset, "SequenceLength")["SequenceLength"]
    ue.props(hero, GroundSlashAnimation=path)
    ue.props(hero, DebugSlashStartDurationRate={"x": 0.0, "y": float(length), "z": 1.0})
    ue.props(hero, bDebugLogBlade=True)
    size = os.path.getsize(LOG)
    ue.props(hero, bDebugSlash=True)
    time.sleep(length + 1.2)
    ue.props(hero, bDebugLogBlade=False)
    time.sleep(0.4)
    with open(LOG, "rb") as f:
        f.seek(size)
        text = f.read().decode("utf-8", errors="replace")
    rows = []
    for m in re.finditer(r"Blade t=([\d.]+) base=(-?\d+),(-?\d+) tip=(-?\d+),(-?\d+)", text):
        rows.append(tuple(float(v) for v in m.groups()))
    name = path.split("/")[-1].split(".")[0]
    with open(os.path.join(OUT, name + ".csv"), "w") as f:
        f.write("t,base_x,base_z,tip_x,tip_z\n")
        for r in rows:
            f.write(",".join("%g" % v for v in r) + "\n")
    print("==", name, "length %.2f s," % length, len(rows), "frames")
    if len(rows) < 10:
        continue
    t0 = rows[0][0]
    # speed of the tip in cm/s, smoothed over 3 frames
    speeds = []
    for i in range(1, len(rows)):
        dt = rows[i][0] - rows[i - 1][0]
        if dt <= 0:
            speeds.append(0.0)
            continue
        dx, dz = rows[i][3] - rows[i - 1][3], rows[i][4] - rows[i - 1][4]
        speeds.append(((dx * dx + dz * dz) ** 0.5) / dt)
    sm = [sum(speeds[max(0, i - 1):i + 2]) / len(speeds[max(0, i - 1):i + 2]) for i in range(len(speeds))]
    segs, cur = [], None
    for i, v in enumerate(sm):
        t = rows[i + 1][0] - t0
        if v > 350:
            if cur is None:
                cur = [t, t, v, rows[i + 1][3], t]
            cur[1] = t
            cur[2] = max(cur[2], v)
            if rows[i + 1][3] > cur[3]:
                cur[3] = rows[i + 1][3]
                cur[4] = t
        elif cur is not None and t - cur[1] > 0.12:
            segs.append(cur)
            cur = None
    if cur:
        segs.append(cur)
    for s in segs:
        if s[1] - s[0] >= 0.06:
            print("   fast %.2f - %.2f s (%.2f s), peak %.0f cm/s, farthest reach x %.0f at %.2f s" % (s[0], s[1], s[1] - s[0], s[2], s[3], s[4]))
ue.call(ue.E, "StopPIE")
