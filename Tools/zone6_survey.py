"""Bounds of the big backdrop pieces that could sit behind the last zone."""
import ue

for name in ("ForestFloor", "Mountain", "Hills", "Backdrop", "Storm", "Far", "Trees_R"):
    acts = [a for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag="") if "UEDPIE" not in a["refPath"]]
    print(name, len(acts))
    for a in acts[:400]:
        b = ue.call(ue.A, "get_actor_bounds", actor=a)
        if b["max"]["x"] < 52000 or b["min"]["x"] > 66000:
            continue
        print("   %-60s x %6.0f..%6.0f y %7.0f..%7.0f z %6.0f..%6.0f" % (a["refPath"].split(".")[-1][:60], b["min"]["x"], b["max"]["x"], b["min"]["y"], b["max"]["y"], b["min"]["z"], b["max"]["z"]))
