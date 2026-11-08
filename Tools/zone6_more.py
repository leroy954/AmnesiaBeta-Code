"""Fills the storm plain of the last zone (x 54500..64500, y -450..-5400) between the walking path and the stone ring:
a broken flagstone road to the giant lined with pillars, ruined wall runs and gate arches, rune obelisks, dead trees,
dark spruces on both flanks and boulders. Builds on zone6.py (ring, giant, hill faces) and back_grass.py (grass).
Usage: python zone6_more.py"""
import math, random
import ue
import levelgen as lg

RU = "/Game/AncientTempleRuins/Environment/"


def m(rel):
    return RU + rel + "." + rel.split("/")[-1]


PILLAR = m("Construction/Pillar/SM_Pillar_01_128_128_512_01")      # 146x147x486, bottom centre
BASE = m("Construction/Pillar/SM_PillarBase_01_128_192_01")        # 137x137x199, bottom centre
WALL = m("Construction/Wall/SM_Wall_01_Linear_256_256_01A")        # 256x64x256, min corner
WALL_OPEN = m("Construction/Wall/SM_Wall_01_Open_192_256_01_A")    # 521x199x521 an arch, min (-5,-100,-4)
FLOOR = m("Construction/Floor/SM_Floor_01_Linear_768_512_01_A")    # 768x512x64, top at pivot, min corner
RUBBLE = [m("Rock/Stone02/SM_Stones_02_%s" % k) for k in "abcd"]
UPRIGHT = m("Prop/StoneHead/SM_StoneHead_01_A")                    # 1144x1106x1825, z -619..1206
MOSS_ROCK = lg.MOSS_ROCK
ROCKS = lg.SPRUCE_ROCKS
DEAD = lg.DRY_TREE
SPRUCES = lg.TREES

F = "Gen/Zone6/More"
DEC = ["Decoration"]
CX = 59900.0
rnd = random.Random(6161)
ops = [{"op": "delete", "folder": F}]


def ground(x, y):
    return lg.ground_z(x, y)


def lowest(x0, x1, y):
    return min(ground(x0, y), ground(x1, y), ground((x0 + x1) / 2.0, y))


def put(mesh, x, y, z, label, yaw=0.0, pitch=0.0, roll=0.0, s=1.0, sx=None, sy=None, sz=None, shadow=None):
    ops.append(ue.sm(mesh, x, y, z, yaw=yaw, pitch=pitch, roll=roll, s=s, sx=sx, sy=sy, sz=sz, label=label, folder=F, tags=DEC, shadow=shadow))


def rubble(x, y, spread, n):
    for _ in range(n):
        rx, ry = x + rnd.uniform(-spread, spread), y + rnd.uniform(-spread * 0.6, spread * 0.6)
        put(rnd.choice(RUBBLE), rx, ry, ground(rx, ry) + 25, "Rubble", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.45, 0.9))


def rune(x, y, z, intensity=22, radius=2400):
    lg.light(ops, x, y, z, (0.45, 0.7, 1.0), intensity, radius, 0.5, "Rune_Light", folder=F)


# ---------------------------------------------------------------- the flagstone road to the giant, broken and sunk
ROAD_FROM, ROAD_TO = -480.0, -7000.0
y = ROAD_FROM
while y > ROAD_TO:
    y0 = y - 768
    if rnd.random() > 0.22:
        # turned to run along y: the footprint is x [px-512, px], y [py, py+768]
        put(FLOOR, CX + 256 + rnd.uniform(-14, 14), y0, ground(CX, y0) + 6 + rnd.uniform(-8, 3), "RoadSlab", yaw=90 + rnd.uniform(-2.5, 2.5),
            pitch=rnd.uniform(-1.5, 1.5), roll=rnd.uniform(-1.5, 1.5), sx=1.0, sy=rnd.uniform(0.97, 1.0), sz=1.0)
    else:
        rubble(CX, y0 + 384, 220, 3)
    y -= 768

# pillars on both sides of the road: whole, broken, stumps and fallen ones
n_runes = 0
for i, py in enumerate([-900 - 980 * k for k in range(6)]):
    for side in (-1, 1):
        x = CX + side * (470 + rnd.uniform(-50, 50))
        yy = py + rnd.uniform(-90, 90)
        z = ground(x, yy) - 8
        sc = 1.15 + 0.07 * i                  # taller towards the giant, to lead the eye
        state = rnd.choice(["whole", "broken", "broken", "stump", "fallen"] if i > 0 else ["broken", "stump"])
        put(BASE, x, yy, z, "PillarBase", yaw=rnd.choice([0, 90]), s=sc)
        zt = z + 199 * sc - 6
        keep = {"whole": 1.0, "broken": rnd.uniform(0.45, 0.7), "stump": rnd.uniform(0.18, 0.3), "fallen": rnd.uniform(0.25, 0.4)}[state]
        put(PILLAR, x, yy, zt, "Pillar", yaw=rnd.uniform(-5, 5), sx=sc, sy=sc, sz=keep * sc)
        if state == "fallen":
            lie = -side
            put(PILLAR, x + lie * 180, yy - 40, ground(x, yy) + 75 * sc, "FallenPillar", yaw=90 if lie > 0 else -90, roll=90, sx=sc, sy=sc, sz=(1.0 - keep - 0.05) * sc)
        if state != "whole":
            rubble(x, yy, 200, 2)
        elif n_runes < 6:
            rune(x, yy + 90, zt + 380 * sc)
            n_runes += 1

# ---------------------------------------------------------------- ruined wall runs and gate arches on both sides
def wall_run(x0, y, heights):
    for i, rows in enumerate(heights):
        cx = x0 + i * 256
        base = lowest(cx, cx + 256, y) - 15
        for r in range(rows):
            put(WALL, cx, y, base + r * 256, "RuinWall", yaw=0)
        if rows == 0:
            rubble(cx + 128, y + 100, 150, 3)
        elif rnd.random() < 0.55:
            rubble(cx + 128, y + 110, 130, 1)


def side_run(x, y0, heights):
    """A wall running away from the camera (along -y), turned -90 degrees, so the plain has walls seen from the side too."""
    for i, rows in enumerate(heights):
        py = y0 - i * 256
        base = lowest(x, x + 64, py - 128) - 15
        for r in range(rows):
            put(WALL, x, py, base + r * 256, "RuinWall", yaw=-90)
        if rows == 0:
            rubble(x + 30, py - 128, 150, 3)


# low, broken profiles: a 3 row slab reads as a black monolith from the side
RUNS = [
    (55400, -900, [1, 2, 1, 0, 1]), (56900, -1900, [1, 1, 2, 1, 0, 1]), (58100, -3300, [2, 1, 2, 0]),
    (61500, -1100, [1, 2, 1, 2, 0, 1]), (62700, -2300, [1, 0, 2, 1, 1, 0]), (63300, -3600, [2, 1, 0, 1, 1]),
    (54900, -3500, [1, 2, 0, 0, 1]), (61100, -4600, [1, 2, 1, 0]),
]
for x0, yy, heights in RUNS:
    wall_run(x0, yy, heights)

for sx0, sy0, heights in ((58900, -1500, [2, 1, 1, 0, 1]), (60900, -2000, [1, 2, 0, 1]), (57000, -4300, [1, 1, 2, 0, 1]), (63900, -2800, [2, 1, 0, 1, 1])):
    side_run(sx0, sy0, heights)

for ax, ay, sc in ((57500, -2700, 1.3), (62300, -2900, 1.3), (56000, -4400, 1.1)):
    g = lowest(ax, ax + 521 * sc, ay)
    put(WALL_OPEN, ax, ay + 100 * sc, g - 8, "RuinArch", s=sc)
    wall_run(ax - 2 * 256, ay, [1, 2])
    wall_run(ax + 521 * sc + 60, ay, [2, 1])

# ---------------------------------------------------------------- rune obelisks on the plain, cold light on their faces
for k, (ox, oy) in enumerate(((CX - 1500, -2300), (CX + 1700, -3000), (CX - 2300, -4000), (CX + 2600, -4500), (CX - 900, -5000), (CX + 1000, -5300))):
    sz = rnd.uniform(0.3, 0.4)
    put(UPRIGHT, ox, oy, ground(ox, oy) - 40 + 619 * sz, "Obelisk", yaw=rnd.uniform(-20, 20), roll=rnd.uniform(-4, 4), sx=0.16, sy=0.16, sz=sz)
    rune(ox, oy + 160, ground(ox, oy) + 1825 * sz * 0.45, intensity=28, radius=2800)

# ---------------------------------------------------------------- dead trees over the plain, dark spruces on the flanks, boulders
def clear_of_road(x):
    return abs(x - CX) > 420


for _ in range(80):
    x, yy = rnd.uniform(54600, 64300), rnd.uniform(-600, -5300)
    if not clear_of_road(x):
        continue
    put(rnd.choice(DEAD), x, yy, ground(x, yy) - 20, "DeadTree", yaw=rnd.uniform(0, 360), s=rnd.uniform(1.1, 2.3) * (1.0 + abs(yy) * 0.00012), shadow=False)

# dense groves of dead trees on both flanks, a dark frame around the plain (living spruces glow pale under the storm light)
for side in (-1, 1):
    for _ in range(34):
        x = CX + side * rnd.uniform(3300, 5200)
        yy = rnd.uniform(-2400, -5300)
        if not 54200 < x < 64600:
            continue
        put(rnd.choice(DEAD), x, yy, ground(x, yy) - 20, "DeadTree", yaw=rnd.uniform(0, 360), s=rnd.uniform(1.8, 3.0), shadow=False)

for _ in range(46):
    x, yy = rnd.uniform(54600, 64300), rnd.uniform(-700, -5200)
    if not clear_of_road(x):
        continue
    if rnd.random() < 0.35:
        put(MOSS_ROCK, x, yy, ground(x, yy) - 60, "Boulder", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.8, 1.7))
    else:
        put(rnd.choice(ROCKS), x, yy, ground(x, yy) - 25, "Boulder", yaw=rnd.uniform(0, 360), s=rnd.uniform(1.4, 3.0))

print("pieces:", len(ops))
print(ue.job(ops + [{"op": "save"}], timeout=600))
