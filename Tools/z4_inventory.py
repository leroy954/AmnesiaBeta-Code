"""Prints the jump ledges in zone 4 (their hidden collision boxes) and the gameplay actors standing there."""
import ue

for name in ("Ledge_Collision", "Checkpoint_0", "HealOrb", "Ent_"):
    for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag=""):
        t = ue.call(ue.A, "get_actor_transform", actor=a)
        x = t["location"]["x"]
        if 31500 < x < 42500:
            print("%-22s x %7.0f y %5.0f z %6.0f  scale %.2f %.2f %.2f" % (ue.call(ue.A, "get_label", actor=a), x, t["location"]["y"], t["location"]["z"],
                                                                        t["scale"]["x"], t["scale"]["y"], t["scale"]["z"]))
