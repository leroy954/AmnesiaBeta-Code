"""Checks every chunk of the long world for decorations standing in the hero's line (y -120..120).
Decorations have no collision in the game, so a tall piece there swallows the hero. Builds each chunk without sending it.
Usage: python lw_audit.py [Z1 Z2 ...]"""
import sys, math, importlib
from collections import Counter
import lw, lwb

KEYS = sys.argv[1:] or [z.key for z in lw.ZONES]
# trunks are thin, grass and plants are soft, the rock faces and retaining walls stand in front on purpose
IGNORE = ("Spruce_0", "Small_Spruce", "SM_Tree_", "Ceiba_Tree", "Dry_Tree", "Broken_Tree", "Landscape", "Cube", "SM_River", "SM_Grass", "SM_Flowers",
          "SM_RockWall_01", "TopTrim", "DirtStony", "SM_Fern", "Monstera", "Heliconia", "Plant", "SM_Bush", "Bush_0", "SM_Dry_Bush", "Ivy")
bad = Counter()
examples = {}
total = 0
for key in KEYS:
    mod = importlib.import_module("lw_" + key.lower())
    z = lw.ZONE[key]
    for k in range(z.n):
        c = lw.Chunk(z, k)
        mod.build(c)
        for (mesh, shadow, cull, mat), items in c.batches.items():
            name = mesh.split("/")[-1].split(".")[0]
            if any(i in name for i in IGNORE):
                continue
            b = lwb.b(mesh)
            if "StalagmiteWall" in name:
                continue          # the cave's wall and lip lumps are flattened along their bulge, checked by eye
            for x, y, zz, pitch, yaw, roll, sx, sy, sz in items:
                total += 1
                height = (b[5] - b[2]) * sz
                if height < 70:
                    continue
                # the footprint's extent along y: the corners of the scaled box, turned by the yaw
                cs, sn = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
                ys = [sn * (px * sx) + cs * (py * sy) for px in (b[0], b[3]) for py in (b[1], b[4])]
                ymin, ymax = y + min(ys), y + max(ys)
                if ymax > -110 and ymin < 110 and zz < lw.ground(x, 0) + 260:
                    bad[name] += 1
                    examples.setdefault(name, (key, k, round(x), "y %d..%d" % (ymin, ymax), "h %d" % height))
print("instances checked:", total)
print("in the hero's line:", sum(bad.values()))
for name, n in bad.most_common(30):
    print("%5d  %-34s e.g. %s" % (n, name, examples[name]))
