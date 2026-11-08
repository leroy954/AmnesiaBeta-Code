"""Ground height behind the path: drops small probe cubes with a dry snap, reads the gaps and removes the probes.
Usage: python ground_probe.py x0 x1 dx y0 y1 dy   (prints a table of ground z)"""
import sys
import ue

x0, x1, dx, y0, y1, dy = (float(v) for v in sys.argv[1:7])
TOP = 5000.0
CUBE = "/Engine/BasicShapes/Cube.Cube"
F = "Temp/Probe"
ops = []
pts = []
x = x0
while x <= x1:
    y = y0
    while y >= y1:
        label = "Probe_%d_%d" % (x, y)
        ops.append(ue.sm(CUBE, x, y, TOP, s=0.1, label=label, folder=F, tags=["Decoration"]))
        pts.append((x, y, label))
        y -= dy
    x += dx
ue.job(ops)
r = ue.job([{"op": "snap", "folder": F, "dry": True, "max_drop": 20000}])
gaps = {}
for line in r["report"]:
    parts = line.split("|")
    if len(parts) >= 3:
        gaps[parts[1]] = float(parts[2])
    else:
        print("report:", line)
ue.job([{"op": "delete", "folder": F}])
ys = sorted({p[1] for p in pts}, reverse=True)
xs = sorted({p[0] for p in pts})
print("y\\x    " + " ".join("%6.0f" % x for x in xs))
for y in ys:
    row = []
    for x in xs:
        g = gaps.get("Probe_%d_%d" % (x, y))
        row.append("%6.0f" % (TOP - 5 - g) if g is not None else "     -")
    print("%7.0f " % y + " ".join(row))
