"""Walks the long world in the running game with the path_scan job op and lists anything that blocks the hero at body height.
The scan follows the floor from step to step, so it must start in the open: the cave is scanned in one go from the ruins in front of it."""
import sys, time
import ue
from pie_lib import start_play, pie
import lw

start_play()
time.sleep(3)
rows = []
spans = [(lw.X_START - 1500, 590000), (590000, 718000), (718000, lw.X_END + 500)]
for a, b in spans:
    x = a
    while x < b:
        r = ue.job([{"op": "path_scan", "x0": x, "x1": min(x + 130000, b), "step": 20}], world="pie", timeout=900)
        rows += r["report"]
        x += 130000
ue.call(ue.E, "StopPIE")
print(len(rows), "hits")
for line in rows[:60]:
    print(line)
