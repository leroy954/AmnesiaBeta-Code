"""Builds parts of the long world.
Usage: python lw_build.py terrain          ground slabs, floor strips, end walls, backdrop
       python lw_build.py z1 [k ...]       zone 1, all chunks or the listed ones
       python lw_build.py clear            removes the whole long world (folder LW)
"""
import sys, importlib
import ue
import lw
import levelgen as lg


def terrain():
    ops = lw.terrain_ops()
    for z in lw.ZONES:
        if z.key != "Z5":
            ops += lw.floor_ops(z)
    ops += lw.wall_ops()
    import random
    ops += lw.backdrop_ops(random.Random(3))
    res = ue.job(ops, timeout=900)
    print("terrain:", res["spawned"], "spawned", res["errors"])


def zone(key, chunks=None):
    mod = importlib.import_module("lw_" + key.lower())
    z = lw.ZONE[key]
    for k in (chunks if chunks else range(z.n)):
        lw.run_chunk(key, k, mod.build)


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        print(__doc__)
    elif args[0] == "terrain":
        terrain()
        print("ends:", ue.job(lw.end_ops(), timeout=600)["spawned"])
    elif args[0] == "clear":
        print(ue.job([{"op": "delete", "folder": "LW"}], timeout=900))
    else:
        zone(args[0].upper(), [int(a) for a in args[1:]] or None)
