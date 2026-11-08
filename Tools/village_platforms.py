"""The village plank ledges hung over the path with an invisible box under them that the hero ran into.
Each ledge too low to walk under now stands on a stack of crates and barrels, and its collision reaches the ground.
Ledges high enough to walk under stay as they are."""
import random
import ue
import levelgen as lg

VG = "/Game/GreenwoodFantasyVillage/Meshes/"
CRATE = VG + "SM_Crate.SM_Crate"        # 70 cube, bottom centre
BARREL = VG + "SM_Barrel.SM_Barrel"     # 66x66x69, bottom centre
CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"
F = "Gen/Zone2/Platforms"
WALK_UNDER = 215.0  # the hero needs this much room under a ledge

rnd = random.Random(2202)
boxes = []
for a in ue.call(ue.S, "find_actors", name="Platform_Collision", collision_channels=[], tag=""):
    t = ue.call(ue.A, "get_actor_transform", actor=a)
    l, s = t["location"], t["scale"]
    if 9500 < l["x"] < 21000:
        boxes.append((l["x"], l["x"] + s["x"] * 100, l["y"], l["y"] + s["y"] * 100, l["z"], l["z"] + s["z"] * 100))

ops = [{"op": "delete", "folder": F, "label_prefix": "Platform_Collision", "x_min": 9500, "x_max": 21000},
       {"op": "delete", "folder": F, "label_prefix": "Platform_Crates"}]
for x0, x1, y0, y1, z0, z1 in boxes:
    ground = min(lg.h(x0), lg.h(x1), lg.h((x0 + x1) / 2))
    low = z0 - ground < WALK_UNDER
    bottom = ground - 30 if low else z0
    ops.append(ue.sm(CUBE, x0, y0, bottom, sx=(x1 - x0) / 100.0, sy=(y1 - y0) / 100.0, sz=(z1 - bottom) / 100.0,
                     label="Platform_Collision", folder=F, collision="BlockAll", hidden=True))
    if not low:
        continue
    # crates in columns under the planks, a barrel here and there, all reaching the plank bottom
    fill = z1 - 14 - ground
    columns = max(2, int(round((x1 - x0) / 100.0)))
    width = (x1 - x0) / columns
    for c in range(columns):
        cx = x0 + (c + 0.5) * width
        rows = max(1, int(round(fill / 95.0)))
        size = fill / rows
        for r in range(rows):
            mesh = BARREL if (r == 0 and rnd.random() < 0.3) else CRATE
            base = 69.0 if mesh == BARREL else 70.0
            sxy = min(width, 110.0) / base
            ops.append(ue.sm(mesh, cx + rnd.uniform(-6, 6), rnd.uniform(-8, 8), ground - 4 + r * size, yaw=rnd.choice([0, 90, 180, 270]) + rnd.uniform(-6, 6),
                             sx=sxy, sy=sxy, sz=size / base, label="Platform_Crates", folder=F, tags=["Decoration"]))
    print("crates under ledge %d..%d, fill %d" % (x0, x1, fill))
print(ue.job(ops + [{"op": "save"}]))
