"""Mesh sizes for the long world builders, measured once through the mesh_bounds job op and kept in lwb_cache.json.
b(path) -> (minx, miny, minz, maxx, maxy, maxz) at scale 1."""
import json, os
import ue

CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lwb_cache.json")
_cache = {}
if os.path.exists(CACHE):
    with open(CACHE, encoding="utf-8") as f:
        _cache = json.load(f)


def load(paths):
    """Measures the meshes that are not cached yet."""
    need = [p for p in paths if p not in _cache]
    for i in range(0, len(need), 60):
        part = need[i:i + 60]
        for line in ue.job([{"op": "mesh_bounds", "meshes": part}])["report"]:
            parts = line.split()
            if len(parts) == 7:
                _cache[parts[0]] = [float(v) for v in parts[1:]]
    if need:
        with open(CACHE, "w", encoding="utf-8") as f:
            json.dump(_cache, f)
    return {p: _cache.get(p) for p in paths}


def b(path):
    if path not in _cache:
        load([path])
    return _cache[path]


def size(path):
    v = b(path)
    return v[3] - v[0], v[4] - v[1], v[5] - v[2]
