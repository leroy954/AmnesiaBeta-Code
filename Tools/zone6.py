"""Fills the last zone (storm forest, x 54000..64500): a stone circle with a giant armored stone golem standing in it,
rocky faces on the front of the hills, and rocks, dry grass, bushes, dead trees and broken pillars on the bare hills.
Usage: python zone6.py"""
import math, random
import ue
import levelgen as lg

RU = "/Game/AncientTempleRuins/Environment/"
SP = "/Game/Stylized_Spruce_Forest/Meshes/"


def m(path):
    return path + "." + path.split("/")[-1]


UPRIGHT = m(RU + "Prop/StoneHead/SM_StoneHead_01_A")   # 1144x1106x1825, z -619..1206
LINTEL = m(RU + "Prop/StoneHead/SM_StoneHead_02")      # 244x1095x236 (long along y), z -112..124
BLOCK = m(RU + "Prop/StoneHead/SM_StoneHead_07")       # 553x465x298, z -150..148
ROCK_WALL = m(RU + "Rock/Rockwall_01/SM_RockWall_01")  # 1014x372x479, x -514..500, y -3..369, z -4..475
MOSS_ROCK = m(RU + "Rock/Rock01/SM_Rock01")            # 674x647x464, z -83..381
PILLAR = m(RU + "Construction/Pillar/SM_Pillar_01_128_128_512_01")
RUBBLE = [m(RU + "Rock/Stone02/SM_Stones_02_%s" % k) for k in "abcd"]
ROCKS = [m(SP + "Rocks/SM_Rock_0%d" % i) for i in (1, 2, 3, 6, 7, 8, 9)]
GRASS = [m(SP + "Plants/SM_Grass_01"), m(SP + "Plants/SM_Grass_02")]
DRY_BUSH = [m(SP + "Plants/SM_Dry_Bush_01"), m(SP + "Plants/SM_Dry_Bush_02")]
DRY_TREE = [m(SP + "Debris/SM_Dry_Tree_01"), m(SP + "Debris/SM_Dry_Tree_02"), m(SP + "Debris/SM_Broken_Tree_01")]

F = "Gen/Zone6"
DEC = ["Decoration"]
rnd = random.Random(6060)
ops = [{"op": "delete", "folder": F + "/Circle"}, {"op": "delete", "folder": F + "/Dress"}, {"op": "delete", "folder": F + "/Cliffs"},
       {"op": "delete", "label_prefix": "GiantGolem"}]


def put(mesh, x, y, z, label, folder, yaw=0.0, pitch=0.0, roll=0.0, s=1.0, sx=None, sy=None, sz=None):
    ops.append(ue.sm(mesh, x, y, z, yaw=yaw, pitch=pitch, roll=roll, s=s, sx=sx, sy=sy, sz=sz, label=label, folder=folder, tags=DEC))


def ground(x, y):
    return lg.ground_z(x, y)


# ---------------------------------------------------------------- the stone circle and its guardian
# the giant stands far back on the plain, a colossus over the storm forest, inside a wide ring of standing stones.
# Its 7.5 scale stood 14.7 m tall; 35 makes it about 69 m
CX, CY, R = 59900.0, -9000.0, 5600.0
GIANT_SCALE = 35.0
SS = 2.6        # standing stone size against the old circle
EY = 0.34       # the ring is seen at a low angle, so it is squashed in depth
G = F + "/Circle"

ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaGiant", "loc": [CX, CY, -160], "rot": [0, 90, 0],
            "scale": [GIANT_SCALE] * 3, "label": "GiantGolem", "folder": G,
            "props": {"WatchRange": 30000.0, "ShakeStrength": 3.0}})
# cold light from the front picks the giant out of the storm night: one on the body, one on the head
lg.light(ops, CX + 1500, CY + 4000, 3500, (0.55, 0.7, 1.0), 900, 12000, 0.15, "Giant_Light", folder=G)
lg.light(ops, CX - 1200, CY + 3000, 6500, (0.5, 0.65, 1.0), 600, 9000, 0.2, "Giant_Light", folder=G)


def upright(x, y, h_scale, yaw):
    sz = 0.26 * SS * h_scale
    put(UPRIGHT, x, y, ground(x, y) - 60 + 619 * sz, "Standing", G, yaw=yaw, sx=0.15 * SS, sy=0.15 * SS, sz=sz)
    return ground(x, y) - 60 + 1825 * sz


# trilithons around the back half of the ring, so nothing stands between the camera and the giant
for i, angle in enumerate(range(165, 380, 18)):
    a = math.radians(angle)
    x, y = CX + R * math.cos(a), CY + R * math.sin(a) * EY
    tangent = math.degrees(a) + 90
    tx, ty = math.cos(a + math.pi / 2) * 140 * SS, math.sin(a + math.pi / 2) * 140 * SS * EY
    if i % 4 == 3:
        # one fell long ago and lies in the grass
        put(UPRIGHT, x, y, ground(x, y) + 60 * SS, "FallenStanding", G, yaw=tangent, pitch=84, sx=0.15 * SS, sy=0.15 * SS, sz=0.26 * SS)
        continue
    top_a = upright(x - tx, y - ty, rnd.uniform(0.9, 1.15), tangent + rnd.uniform(-6, 6))
    top_b = upright(x + tx, y + ty, rnd.uniform(0.9, 1.15), tangent + rnd.uniform(-6, 6))
    if i % 2 == 0:
        put(LINTEL, x, y, min(top_a, top_b) + 112 * 0.3 * SS - 10 * SS, "Lintel", G, yaw=tangent + 90, sx=0.3 * SS, sy=0.3 * SS, sz=0.3 * SS)

# the front corners of the ring: single stones that frame it without hiding the giant
for side in (-1, 1):
    a = math.radians(90 - side * 62)
    x, y = CX + R * 1.05 * math.cos(a), CY + R * math.sin(a) * EY
    upright(x, y, 0.8, math.degrees(a) + 90)

# an inner ring of low broken blocks, and cold glowing runes
for angle in range(0, 360, 30):
    a = math.radians(angle + 15)
    x, y = CX + R * 0.6 * math.cos(a), CY + R * 0.6 * math.sin(a) * EY
    if y > CY + 600:
        continue
    put(BLOCK, x, y, ground(x, y) - 40 * SS, "CircleBlock", G, yaw=rnd.uniform(0, 360), roll=rnd.uniform(-8, 8), s=rnd.uniform(0.35, 0.55) * SS)
    lg.light(ops, x, y, ground(x, y) + 160 * SS, (0.45, 0.7, 1.0), 25, 2600, 0.5, "Rune_Light", folder=G)

# ---------------------------------------------------------------- rock faces on the front of the hills
C = F + "/Cliffs"
x = 55500
while x < 63800:
    top = min(lg.h(x), lg.h(x + 600))
    if top > 40:
        put(ROCK_WALL, x + 300, 112, -40, "HillFace", C, yaw=rnd.choice([0, 180]) if False else 0, sx=0.62, sy=0.45, sz=(top + 30) / 479.0)
    x += 560

# ---------------------------------------------------------------- the bare hills get rocks, dry grass, bushes, dead trees and old pillars
D = F + "/Dress"
for i in range(260):
    x = rnd.uniform(54400, 64200)
    y = rnd.choice([rnd.uniform(-1350, -200), rnd.uniform(-1350, -200), rnd.uniform(-2600, -1450)])
    if abs(x - CX) < R * 1.1 and y < CY + R * EY + 300:
        continue
    z = ground(x, y)
    k = rnd.random()
    if k < 0.38:
        put(rnd.choice(GRASS), x, y, z - 2, "DryGrass", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.8, 2.6))
    elif k < 0.58:
        put(rnd.choice(DRY_BUSH), x, y, z - 10, "DryBush", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.0, 3.4))
    elif k < 0.8:
        put(rnd.choice(ROCKS), x, y, z - 25, "HillRock", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.9, 2.2))
    elif k < 0.9:
        put(rnd.choice(DRY_TREE), x, y, z - 20, "DeadTree", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.9, 1.4))
    elif k < 0.95:
        put(MOSS_ROCK, x, y, z - 60, "BigRock", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.4, 0.8))
    else:
        # the last broken pillars of the ruins, leaning
        put(PILLAR, x, y, z - 20, "OldPillar", D, yaw=rnd.uniform(0, 360), pitch=rnd.uniform(-12, 12), sx=0.8, sy=0.8, sz=rnd.uniform(0.25, 0.6))
        put(rnd.choice(RUBBLE), x + rnd.uniform(-150, 150), y + rnd.uniform(-80, 80), z + 10, "Rubble", D, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.4, 0.8))

print("pieces:", len(ops))
print(ue.job(ops + [{"op": "save"}]))

