"""Places the cats of the zones: one sleeping cat in each of the six zones. The ground height comes from the level scripts (the cat snaps to the real
ground when the game starts). Idempotent: the outliner folder Gen/Cats is deleted first.
Usage: python cats.py [place | clear]"""
import sys

import ue
import pie_lib as P

FOLDER = "Gen/Cats"
CLASS = "/Script/AmnesiaBeta.AmnesiaCat"

# id, label, coat of the pack (1..11), x, y (a little behind the walking line), yaw (0 faces +x, 180 faces -x)
# The places are the ones where a test cat could be seen from all sides (cat_sweep.py), the coats stand out against the zone
CATS = [
    ("forest", "Erdő", 7, 2050.0, -25.0, 0.0),
    ("village", "Falu", 5, 12800.0, -25.0, 0.0),
    ("snow", "Havas erdő", 4, 28500.0, -25.0, 0.0),
    ("ruins", "Romok", 2, 40000.0, -25.0, 0.0),
    ("cave", "Barlang", 9, 49500.0, -25.0, 0.0),
    ("storm", "Vihar", 1, 61750.0, -25.0, 0.0),
]
SCALE = 1.8


def build():
    ops = [{"op": "delete", "folder": FOLDER}]
    for cat_id, zone, coat, x, y, yaw in CATS:
        z = P.ground_h(x)
        ops.append({"op": "spawn_actor", "class": CLASS, "loc": [x, y, z], "rot": [0.0, yaw, 0.0], "label": "Cat_" + cat_id, "folder": FOLDER,
                    "props": {"CatId": cat_id, "Coat": coat, "CatScale": SCALE}})
        print("%-8s x %7.0f ground %6.0f" % (cat_id, x, z))
    ops.append({"op": "save"})
    return ops


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "place"
    if what == "clear":
        print(ue.job([{"op": "delete", "folder": FOLDER}, {"op": "save"}], timeout=300))
    else:
        print(ue.job(build(), timeout=300))
