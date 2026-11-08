"""Sends only the enemies and lights of every chunk again (after they were removed for a measurement), leaving the instanced meshes alone.
Usage: python lw_restore.py [enemies] [lights]     default: both"""
import sys, importlib
import ue
import lw

want = sys.argv[1:] or ["enemies", "lights"]
total = 0
for z in lw.ZONES:
    mod = importlib.import_module("lw_" + z.key.lower())
    for k in range(z.n):
        c = lw.Chunk(z, k)
        mod.build(c)
        ops = []
        for op in c.ops:
            label, folder = op.get("label", ""), op.get("folder", "")
            if "enemies" in want and label.startswith("Ent_L"):
                ops.append(op)
            if "lights" in want and folder.endswith("/Lights"):
                ops.append(op)
        if ops:
            ue.job(ops, timeout=600)
            total += len(ops)
    print(z.key, "done")
print(total, "actors sent")
