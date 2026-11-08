"""Finds static meshes whose bounds reach into x 54300..56800 high above the path (the odd slab at the top left of zone 6's start)."""
import ue

seen = set()
for name in ("Cave", "Ceil", "Slab", "Ground", "Hill", "Cliff", "Wall", "Rock"):
    for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag=""):
        p = a["refPath"]
        if "UEDPIE" in p or p in seen:
            continue
        seen.add(p)
        try:
            b = ue.call(ue.A, "get_actor_bounds", actor=a)
        except Exception:
            continue
        if b["max"]["x"] > 54300 and b["min"]["x"] < 56800 and b["max"]["z"] > 500 and b["min"]["z"] > -200 and b["min"]["y"] > -3000:
            print(p.split(".")[-1][:60], "x %.0f..%.0f y %.0f..%.0f z %.0f..%.0f" % (b["min"]["x"], b["max"]["x"], b["min"]["y"], b["max"]["y"], b["min"]["z"], b["max"]["z"]))
print("checked", len(seen))
