"""Dresses the way from the ruins into the cave (x 41500..45500), because it read as a flat board on a black slab.

What was wrong: the bare ramp was a straight bright plane with a razor edge, the grass block ended in a vertical cut, the pale cave-pack
lumps at the mouth were blown out by the sun and the front face of the slab was black. The black face was the material: MI_Ground_Cave
darkens the sides of everything below z 0 down to DarkenMin 0.1, so the slab face is now raised to 0.42 (set once, not here).

What this does:
  mouth   the glaring wall lumps beside the mouth are replaced by the mossy ruin boulders the mouth already has
  seam    mossy boulders at the end of the grass block, low enough not to cover the hero's feet
  ramp    paving tiles laid on the slope, thinning out into the cave, low broken stones along the lip, ivy hanging over the lip
Everything stands behind or at the lip (never above 40 cm near the hero's line) and is tagged Decoration, so it has no collision.
Usage: python cave_mouth.py"""
import math, random
import ue
import levelgen as lg

RU = "/Game/AncientTempleRuins/Environment/"
F = "Gen/Zone5/Mouth2"
DEC = ["Decoration"]
rnd = random.Random(4242)
h = lg.h


def m(rel):
    return RU + rel + "." + rel.split("/")[-1]


ROCK = m("Rock/Rock01/SM_Rock01")                         # 674x647x464, min z -83, pivot near the centre
TILES = [m("Rock/StoneTiles02/SM_StoneTiles_01_%s" % k) for k in "ABCDEFGH"]   # about 90-178 x 100 x 25, pivot in the centre
STONES = [(m("Rock/Stone02/SM_Stones_02_a"), -56.3, 82.8), (m("Rock/Stone02/SM_Stones_02_b"), -57.3, 117.1),
          (m("Rock/Stone02/SM_Stones_02_c"), -57.8, 101.2), (m("Rock/Stone02/SM_Stones_02_d"), -39.8, 80.5)]   # (mesh, min z, height)
IVY = [m("Foliage/Ivy_01/SM_Ivy_01_%s" % k) for k in "DEFGH"]

# the ivy hangs down from its pivot: its size, to set the length
ivy_h = {}
for line in ue.job([{"op": "mesh_bounds", "meshes": IVY}])["report"]:
    p = line.split()
    if len(p) == 7:
        ivy_h[p[0].split("/")[-1].split(".")[0]] = (float(p[6]) - float(p[3]), float(p[3]), float(p[6]))
print("ivy (height, min z, max z):", {k: tuple(round(v) for v in t) for k, t in ivy_h.items()})

ops = [{"op": "delete", "folder": F}]


def put(mesh, x, y, z, label, yaw=0.0, pitch=0.0, roll=0.0, s=1.0, sx=None, sy=None, sz=None):
    ops.append(ue.sm(mesh, x, y, z, yaw=yaw, pitch=pitch, roll=roll, s=s, sx=sx, sy=sy, sz=sz, label=label, folder=F, tags=DEC))


def slope(x):
    return math.degrees(math.atan2(h(x + 50) - h(x - 50), 100.0))


# ---------------------------------------------------------------- the mouth: mossy boulders instead of the pale lumps
# a pile at the left end of the cave wall, stacked so the top reaches the ceiling lumps that stay
for x, y, z, s, yaw in ((41690, -735, -60, 1.9, 20), (41610, -650, 300, 1.3, 140), (41800, -770, 520, 1.15, 75), (41720, -600, 760, 0.95, 200)):
    put(ROCK, x, y, z, "MouthPile", yaw=yaw, s=s)

# ---------------------------------------------------------------- the seam: where the grass block ends over the dark face
# the top stays at 60 cm and the footprint behind y 82 is free of the hero, so the feet stay visible
put(ROCK, 41985, 262, -150, "SeamRock", yaw=35, s=0.5)
put(ROCK, 42150, 235, -300, "SeamRock", yaw=-60, s=0.42)
put(ROCK, 41850, 285, -170, "SeamRock", yaw=110, s=0.36)

# ---------------------------------------------------------------- the ramp: paving that gives way to bare rock
x = 41960.0
while x < 43500.0:
    fade = 1.0 - (x - 41960.0) / 1500.0
    for y in (-95.0, 25.0):
        if rnd.random() < max(0.12, fade * 0.95):
            put(rnd.choice(TILES), x + rnd.uniform(-30, 30), y + rnd.uniform(-14, 14), h(x) - 4, "MouthTile", yaw=rnd.uniform(-10, 10),
                pitch=slope(x), s=rnd.uniform(1.0, 1.25))
    x += 135.0

# ---------------------------------------------------------------- the lip: broken stones lying along it, ivy over the edge
x = 41930.0
while x < 44900.0:
    mesh, minz, hgt = rnd.choice(STONES)
    s = min(rnd.uniform(0.55, 0.9), 36.0 / hgt)
    put(mesh, x, rnd.uniform(96, 124), h(x) - 8 - minz * s, "LipStone", yaw=rnd.uniform(-20, 20), pitch=slope(x), s=s)
    x += rnd.uniform(110, 240)

x = 42000.0
while x < 45300.0:
    name = rnd.choice(list(ivy_h))
    height, zmin, zmax = ivy_h[name]
    sc = rnd.uniform(150.0, 260.0) / height
    put(m("Foliage/Ivy_01/" + name), x, 152 + rnd.uniform(-6, 10), h(x) + 6 - zmax * sc, "LipIvy", yaw=rnd.uniform(-10, 10), s=sc)
    x += rnd.uniform(260, 520)

print("pieces:", len(ops) - 1)
print(ue.job(ops + [{"op": "save"}]))
