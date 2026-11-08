"""Rebuilds zone 4 (x 32000..42000) as an overgrown, collapsed temple, laid out on purpose instead of scattered.

Route, left to right:
  gate (two tall pillars, one broken lintel, the other piece fallen)
  stairs up the ramp, a stepped parapet beside them
  temple terrace: paving with missing slabs, a colonnade of whole, broken and fallen pillars,
    a back wall with a broken stepped top, ivy and roots, an engraved circle as altar under the rest place
  stairs down, a fallen masonry block half sunk in the courtyard, pillar stumps and two standing torches
  stairs up to a small shrine: facade, two pillars and two torches
  rubble and roots towards the cave mouth
The jump ledges keep their collision boxes and get a broken upper floor look: a slab on support pillars.
Pieces stack on each other (a wall row only sits on a full row), and everything is set on the ground with h(x).

Usage: python zone4.py            rebuild
       python zone4.py --clear    only remove the old ruins
"""
import sys, math, random
import ue
import levelgen as lg

RU = "/Game/AncientTempleRuins/Environment/"


def m(rel):
    name = rel.split("/")[-1]
    return RU + rel + "." + name


PILLAR = m("Construction/Pillar/SM_Pillar_01_128_128_512_01")          # 146x147x486, bottom centre
PILLAR_THIN = m("Construction/Pillar/SM_Pillar_01_64_64_256_D")        # 67x76x261, bottom centre
BASE_TALL = m("Construction/Pillar/SM_PillarBase_01_128_192_01")       # 137x137x199, bottom centre
BASE_LOW = m("Construction/Pillar/SM_PillarBase_01_128_96_01_A")       # 137x137x104, bottom centre
WALL = m("Construction/Wall/SM_Wall_01_Linear_256_256_01A")            # 256x64x256, min corner
WALL_LOW = m("Construction/Wall/SM_Wall_01_Linear_256_128_02")         # 267x58x142, min (-6,-35,-7)
DOOR = m("Construction/Door/SM_Door_01_A")                             # 167x54x518, x -164..3
FLOOR = m("Construction/Floor/SM_Floor_01_Linear_768_512_01_A")        # 768x512x64, top at pivot, min corner
ENGRAVED = m("Construction/Floor/SM_Floor_Engraved_800_800_01_A")      # 602x602, z 32..62
STAIRS = m("Construction/Stair/SM_Stairs_02_Linear_800_500_01")        # 803x501x218, min (-2,-13,-1)
TRIM = m("Construction/Ceil/SM_TopTrim_01_Linear_128_256_A")           # 266x137x73, z -69..4
ORNAMENT = m("Construction/Ornament/SM_Ornament_01_A")
FACADE = m("Construction/Buildings/SM_Facade_Custom_01")               # 473x239x360, x centred, y -1..238
TORCH = m("Prop/SM_Torch_01")                                          # 10x10x79, bottom
POTS = [m("Prop/Pots/SM_Pot_01_A"), m("Prop/Pots/SM_Pot_01_Broken_A_01"), m("Prop/Pots/SM_Pot_03_Broken_A_01"),
        m("Prop/Pots/SM_Pot_02_Broken_A_01"), m("Prop/Pots/SM_Pot_02_A")]
BLOCK = m("Prop/StoneHead/SM_StoneHead_07")                            # a mossy masonry block, 553x465x298, z -150..148
RUBBLE = [m("Rock/Stone02/SM_Stones_02_a"), m("Rock/Stone02/SM_Stones_02_b"), m("Rock/Stone02/SM_Stones_02_c"), m("Rock/Stone02/SM_Stones_02_d")]
TILES = [m("Rock/StoneTiles02/SM_StoneTiles_01_%s" % k) for k in "ABCDEFGH"]
DIRT = [m("GroundScatter/SM_GS_DirtStony_01%s" % k) for k in "cdefg"]
IVY = [m("Foliage/Ivy_01/SM_Ivy_01_%s" % k) for k in "DEFGH"]          # hangs down from its pivot
ROOTS = m("Foliage/CeibaRoots_01/SM_Ceiba_Roots_03B")                  # 1170x902x953, z -918..35
FERNS = [m("Foliage/Fern_01/SM_Fern_01_%s" % k) for k in "ABCDEFGH"]
MONSTERA = [m("Foliage/Monstera_01/SM_Monstera_01_Cluster_0%d" % i) for i in (1, 2, 3, 4)]
HELICONIA = m("Foliage/Heliconia_01/SM_Heliconia_01")

F = "Gen/Zone4"
FR = F + "/Ruins"
DEC = ["Decoration"]

rnd = random.Random(4040)
ops = []
occupied = []  # (x0, x1, y0, y1) footprints the plants keep out of


def h(x):
    return lg.h(x)


def ground(x, y):
    return lg.ground_z(x, y)


def lowest(x0, x1, y):
    """Lowest ground under a span, so a piece standing on a slope never shows a gap."""
    return min(ground(x0, y), ground(x1, y), ground((x0 + x1) / 2.0, y))


def put(mesh, x, y, z, label, yaw=0.0, pitch=0.0, roll=0.0, s=1.0, sx=None, sy=None, sz=None, folder=FR, tags=DEC, collision=None):
    ops.append(ue.sm(mesh, x, y, z, yaw=yaw, pitch=pitch, roll=roll, s=s, sx=sx, sy=sy, sz=sz, label=label, folder=folder, tags=tags, collision=collision))


def occupy(x0, x1, y0, y1):
    occupied.append((min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1)))


def free(x, y, pad=60):
    return not any(x0 - pad < x < x1 + pad and y0 - pad < y < y1 + pad for x0, x1, y0, y1 in occupied)


# ---------------------------------------------------------------- pieces
def pillar(x, y, state="whole", scale=1.0, base=True):
    """A pillar on its base. state: whole, broken (snapped off), stump, fallen (stump plus the top lying next to it)."""
    z = ground(x, y) - 8
    if base:
        put(BASE_TALL, x, y, z, "PillarBase", yaw=rnd.choice([0, 90]), s=scale)
        z += 199 * scale - 6
    occupy(x - 80 * scale, x + 80 * scale, y - 80 * scale, y + 80 * scale)
    if state == "whole":
        put(PILLAR, x, y, z, "Pillar", yaw=rnd.uniform(-4, 4), s=scale)
        top = z + 486 * scale
        if rnd.random() < 0.6:
            put(IVY[rnd.randrange(len(IVY))], x + rnd.uniform(-30, 30), y + 70 * scale, top - 10, "Ivy", yaw=rnd.uniform(-20, 20), s=rnd.uniform(1.0, 1.5))
        return top
    keep = {"broken": rnd.uniform(0.5, 0.7), "stump": rnd.uniform(0.2, 0.32), "fallen": rnd.uniform(0.25, 0.4)}[state]
    put(PILLAR, x, y, z, "Pillar", yaw=rnd.uniform(-6, 6), sx=scale, sy=scale, sz=keep * scale)
    top = z + 486 * keep * scale
    # chunks of the lost part around the foot
    for _ in range(rnd.randint(1, 3)):
        rx, ry = x + rnd.uniform(-220, 220), y + rnd.uniform(-160, 40)
        put(rnd.choice(RUBBLE), rx, ry, ground(rx, ry) + 30, "Rubble", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.35, 0.7))
    if state == "fallen":
        # the rest of the shaft lies on the ground beside it, rolled a little
        lie = rnd.choice([-1, 1])
        length = 486 * (1.0 - keep) * scale
        fx = x + lie * 120
        put(PILLAR, fx, y - 60, ground(fx, y) + 75 * scale, "FallenPillar", yaw=90 if lie > 0 else -90, pitch=0, roll=90,
            sx=scale, sy=scale, sz=(1.0 - keep - 0.05) * scale)
        occupy(min(fx, fx + lie * length), max(fx, fx + lie * length), y - 140, y + 20)
    return top


def wall_run(x0, columns, y, heights, folder=FR, trim=True, ivy=0.35, label="Wall"):
    """A wall of 256 wide stones, columns rows high each, from x0 along x, its face towards the camera at y + 64.
    A row only ever sits on a full row below it, so the broken top steps down instead of floating."""
    tops = []
    for i in range(columns):
        cx = x0 + i * 256
        base = lowest(cx, cx + 256, y) - 15
        rows = heights[i]
        for r in range(rows):
            put(WALL, cx, y, base + r * 256, label, folder=folder)
        top = base + rows * 256
        tops.append(top)
        if rows == 0:
            # a fallen column: its stones lie in front of the gap
            for _ in range(rnd.randint(2, 3)):
                rx = cx + rnd.uniform(20, 236)
                put(rnd.choice(RUBBLE), rx, y + rnd.uniform(40, 160), ground(rx, y + 100) + 30, "Rubble", folder=folder,
                    yaw=rnd.uniform(0, 360), s=rnd.uniform(0.5, 0.9))
            continue
        # the cornice survives only on full height columns with a neighbour as tall
        full = rows == max(heights)
        if trim and full and i > 0 and heights[i - 1] == rows:
            put(TRIM, cx - 5, y - 30, top + 69, "WallTrim", folder=folder)
        if rnd.random() < ivy:
            put(IVY[rnd.randrange(len(IVY))], cx + rnd.uniform(40, 216), y + 70, top - 5, "Ivy", folder=folder, yaw=rnd.uniform(-15, 15),
                s=rnd.uniform(1.1, 1.8) * (1.0 + 0.3 * (rows - 1)))
        # loose stones at the foot of the broken parts
        if not full and rnd.random() < 0.6:
            rx = cx + rnd.uniform(20, 236)
            put(rnd.choice(RUBBLE), rx, y + rnd.uniform(70, 150), ground(rx, y + 100) + 25, "Rubble", folder=folder, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.4, 0.75))
    occupy(x0, x0 + columns * 256, y - 30, y + 180)
    return tops


def broken_heights(columns, tall, rnd_walk=True):
    """Column heights for a ruined wall: mostly tall, with notches and a gap or two."""
    hs = []
    level = tall
    for i in range(columns):
        if rnd_walk:
            level = max(0, min(tall, level + rnd.choice([-1, 0, 0, 1])))
            if i in (0, columns - 1):
                level = max(1, level - 1)
        hs.append(level)
    return hs


def stairs(x0, x1, z0, z1):
    """Stone stairs on the walking ramp between x0 and x1. They are only a look (no collision): the ramp carries the hero."""
    up = z1 > z0
    length = x1 - x0
    rise = abs(z1 - z0)
    sx, sz = length / 800.0, rise / 218.0
    # the front of the steps lines up with the front of the hill (y 140), so they read as cut into it, not as a block before it
    if up:
        put(STAIRS, x0, -350, min(z0, z1) - 6, "Stairs", sx=sx, sy=1.0, sz=sz, tags=[], collision="NoCollision")
    else:
        # turned around, the mesh reaches from y + 13 back to y - 488
        put(STAIRS, x1, 125, min(z0, z1) - 6, "Stairs", yaw=180, sx=sx, sy=1.0, sz=sz, tags=[], collision="NoCollision")


def ledge(x0, x1, top, slab_y0=-340, slab_y1=130, support_y=-260):
    """A broken stretch of upper floor around a jump ledge's collision box: a slab and the pillars it still rests on."""
    length = x1 - x0
    put(FLOOR, x0, slab_y0, top, "LedgeSlab", sx=length / 768.0, sy=(slab_y1 - slab_y0) / 512.0, sz=1.0)
    for sx in (x0 + 70, x1 - 70):
        g = ground(sx, support_y)
        gap = top - 64 - g + 10
        if gap > 40:
            put(BASE_TALL, sx, support_y, g - 8, "LedgeSupport", sx=0.75, sy=0.75, sz=gap / 199.0)
    if rnd.random() < 0.7:
        put(IVY[rnd.randrange(len(IVY))], x0 + rnd.uniform(60, length - 60), slab_y1 - 10, top - 20, "Ivy", s=rnd.uniform(0.9, 1.3))
    occupy(x0, x1, slab_y0, slab_y1)


def torch_stand(x, y, label="Torch"):
    g = ground(x, y)
    put(BASE_LOW, x, y, g - 6, "TorchStand", s=0.7)
    put(TORCH, x, y, g - 6 + 104 * 0.7 - 4, label, s=2.0)
    lg.light(ops, x, y + 20, g + 104 * 0.7 + 150, (1.0, 0.58, 0.22), 14, 900, 0.35, "Torch_Light", folder=F + "/Lights")
    occupy(x - 60, x + 60, y - 60, y + 60)


def pots(x, y, count):
    for _ in range(count):
        px, py = x + rnd.uniform(-90, 90), y + rnd.uniform(-60, 60)
        put(rnd.choice(POTS), px, py, ground(px, py) - 3, "Pot", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.9, 1.25),
            roll=rnd.choice([0, 0, 0, 80]))


def paving(x0, x1, y0, top, missing=0.25):
    """Floor slabs behind the path, with some missing and their broken bits lying around."""
    x = x0
    while x < x1 - 50:
        w = min(768.0, x1 - x)
        if rnd.random() > missing:
            put(FLOOR, x, y0, top + 2, "Paving", sx=w / 768.0, sy=1.0, sz=0.5)
        else:
            for _ in range(rnd.randint(3, 6)):
                tx, ty = x + rnd.uniform(40, w - 40), y0 + rnd.uniform(40, 470)
                put(rnd.choice(TILES), tx, ty, top + 4, "BrokenTile", yaw=rnd.uniform(0, 360), s=rnd.uniform(1.4, 2.2), pitch=rnd.uniform(-6, 6))
        x += w


# ---------------------------------------------------------------- the layout
def build():
    # 1) gate at the zone start
    gl = pillar(32050, -520, "whole", 1.05)
    pillar(32480, -520, "broken", 1.05)
    put(TRIM, 32050 - 80, -560, gl + 69, "Lintel", sx=1.2)  # what is left of the lintel, still resting on the left pillar
    put(TRIM, 32620, -380, ground(32620, -380) + 40, "FallenLintel", yaw=28, pitch=-12, roll=8, sx=1.2)
    for x in (31900, 32250, 32700):
        rx = x + rnd.uniform(-60, 60)
        put(rnd.choice(DIRT), rx, -350, ground(rx, -350) - 8, "Dirt", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.8, 1.2))

    # 2) stairs up, with a stepped parapet behind them
    stairs(33200, 34400, 0, 250)
    x = 33150
    while x < 34400:
        b = ground(x, -420) - 12
        put(WALL_LOW, x + 6, -420 + 35, b + 7, "Parapet")
        x += 262
    occupy(33150, 34420, -460, -330)

    # 3) temple terrace
    T = 250
    paving(34420, 36780, -800, T, missing=0.3)
    occupy(34420, 36780, -800, -290)
    hs = broken_heights(10, 3)
    hs[6] = hs[7] = 3  # the part behind the rest place still stands tall
    wall_run(34320, 10, -900, hs)
    put(DOOR, 34320 + 7 * 256 - 40, -900 + 40, T - 5, "Doorway")  # an empty doorway in the tall part
    # roots from the jungle spill over the back wall
    put(ROOTS, 35050, -1030, T + 918 * 0.45 - 40, "WallRoots", yaw=180, s=0.45)
    put(ROOTS, 36550, -1050, T + 918 * 0.4 - 40, "WallRoots", yaw=200, s=0.4)
    # the colonnade, with a gap for the rest place at 36000
    for px, state in ((34700, "whole"), (35180, "fallen"), (35640, "whole"), (36360, "broken"), (36700, "stump")):
        pillar(px, -470, state, 1.0)
    pots(35420, -560, 3)
    pots(36550, -600, 2)
    # an engraved circle under the rest place, like an altar
    put(ENGRAVED, 36000, -330, T - 62 + 2, "Altar", s=0.9)

    # 4) stairs down, a fallen block and the courtyard
    stairs(36800, 38000, 250, 0)
    put(ENGRAVED, 38500, -360, -62 + 2, "CourtCircle", s=1.0)
    for px, py, st in ((38080, -620, "stump"), (38920, -620, "broken"), (39050, -330, "stump")):
        pillar(px, py, st, 0.85)
    torch_stand(38150, -330)
    torch_stand(38850, -330)
    hs = broken_heights(6, 2)
    wall_run(37900, 6, -1250, hs, ivy=0.5)
    pots(38600, -620, 3)

    # 5) up to the shrine
    stairs(39000, 40000, 0, 160)
    S = 160
    paving(40000, 41000, -760, S, missing=0.15)
    put(FACADE, 40500, -880, S - 4, "Shrine", s=1.15)
    occupy(40220, 40780, -880, -600)
    pillar(40130, -560, "whole", 0.95)
    pillar(40870, -560, "broken", 0.95)
    torch_stand(40280, -430)
    torch_stand(40720, -430)
    wall_run(39500, 2, -1000, [2, 1], ivy=0.6)
    wall_run(41000, 3, -1000, [1, 2, 0], ivy=0.6)

    # 6) down to the cave mouth: the temple gives way to rock and roots
    stairs(41000, 41800, 160, 0)
    for x in (41150, 41450, 41700):
        rx = x + rnd.uniform(-60, 60)
        put(rnd.choice(RUBBLE), rx, -420, ground(rx, -420) + 30, "Rubble", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.6, 1.1))
    pillar(41550, -620, "fallen", 0.9)
    put(ROOTS, 41850, -1150, 918 * 0.5 - 60, "MouthRoots", yaw=160, s=0.5)

    # no floating ledges: two fallen masonry blocks lie on the path to jump onto, solid down to the ground
    solid_block(32650, 32700 + 450, 150)
    solid_block(38560, 38560 + 420, 175)

    # the raised ground shows its earth face to the camera: dressed stone retaining walls cover it
    retaining_walls()

    # behind the temple the city goes on: a near row of broken buildings, a far row of big ones in the mist
    city_row(33600, 41800, -1900, scale=1.0, seed=11, folder=F + "/City")
    city_row(32000, 42400, -3400, scale=2.1, seed=12, folder=F + "/CityFar")
    city_row(31800, 42600, -6200, scale=3.6, seed=13, folder=F + "/CityFar", sparse=True)

    # no bare earth along the path: stony dirt, moss and loose tiles
    ground_cover()

    # plants grow where the stone left room
    undergrowth()


CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"
WALL_OPEN = m("Construction/Wall/SM_Wall_01_Open_192_256_01_A")        # 521x199x521, an arch, min (-5,-100,-4)
PILLAR_BIG = m("Construction/Pillar/SM_Pillar_02_256_512_01")          # 236x287x617, y -2..285
ROOF = m("Construction/Ceil/SM_Roof_02_Linear_256_512_01_A")           # 415x284x221, min (-16,-12,-13)
CEIBA = m("Foliage/Ceiba_Tree_01/SM_Ceiba_Tree_01_A")
MOSS = [m("Foliage/Moss_01/SM_Moss_Cluster_01_A"), m("Foliage/Moss_01/SM_Moss_Cluster_01_B")]


def solid_block(x0, x1, height):
    """A fallen block of masonry on the path: a collision box and the mossy block mesh, both reaching the ground."""
    g = lowest(x0, x1, 0) - 30
    top = g + 30 + height
    ops.append(ue.sm(CUBE, x0, -130, g, sx=(x1 - x0) / 100.0, sy=2.6, sz=(top - g) / 100.0, label="Block_Collision", folder=FR,
                     collision="BlockAll", hidden=True, tags=[]))
    sz = (top - g + 8) / 298.0
    put(BLOCK, (x0 + x1) / 2.0, -10, g + 150 * sz, "PathBlock", sx=(x1 - x0 + 20) / 553.0, sy=300 / 465.0, sz=sz)
    for _ in range(3):
        rx = rnd.uniform(x0 - 120, x1 + 120)
        put(rnd.choice(RUBBLE), rx, rnd.uniform(-200, 160), ground(rx, 0) + 25, "Rubble", yaw=rnd.uniform(0, 360), s=rnd.uniform(0.35, 0.7))
    occupy(x0 - 60, x1 + 60, -220, 220)


def retaining_walls():
    """Stone facing on the front of every raised stretch of ground, so no earth face shows from the camera."""
    x = 33100
    while x < 41900:
        top = max(h(x), h(x + 256))
        if top > 8:
            sz = (top + 40) / 256.0
            put(WALL, x, 140, -40, "Retaining", sx=1.0, sy=1.0, sz=sz)
            # a cornice where the top runs level, moss and ivy where it doesn't
            if abs(h(x) - h(x + 256)) < 2:
                put(TRIM, x - 5, 140 + 30, top + 69 - 4, "RetainingTrim")
            elif rnd.random() < 0.5:
                put(IVY[rnd.randrange(len(IVY))], x + rnd.uniform(30, 220), 210, top - 5, "Ivy", s=rnd.uniform(0.8, 1.3))
        x += 256


def city_row(x0, x1, y, scale, seed, folder, sparse=False):
    """A row of ruined buildings: broken wall shells with side returns, arches, corner pillars, roofs on the parts still standing."""
    local = random.Random(seed)
    unit = 256.0 * scale
    x = x0 + local.uniform(0, unit)
    while x < x1:
        cols = local.randint(3, 6)
        tall = local.randint(2, 4)
        hs = []
        level = tall
        for i in range(cols):
            level = max(1, min(tall, level + local.choice([-1, 0, 0, 1])))
            hs.append(level)
        hs[0] = max(1, hs[0] - 1)
        g = -20.0 * scale
        # the front wall
        i = 0
        while i < cols:
            cx = x + i * unit
            # an arch now and then, two columns wide
            if i + 1 < cols and hs[i] >= 2 and hs[i + 1] >= 2 and local.random() < 0.3:
                put(WALL_OPEN, cx + 5 * scale, y + 100 * scale, g, "CityArch", s=scale * 0.98, folder=folder)
                for r in range(2, min(hs[i], hs[i + 1])):
                    for k in (0, 1):
                        put(WALL, cx + k * unit, y, g + r * unit, "CityWall", s=scale, folder=folder)
                i += 2
                continue
            for r in range(hs[i]):
                put(WALL, cx, y, g + r * unit, "CityWall", s=scale, folder=folder)
            i += 1
        # side walls going back, so the building has depth and is not a flat card
        # (turned -90, a wall piece reaches from its pivot back to y - 256 and over x 0..64)
        depth = local.randint(2, 3)
        for side, hx in ((0, x), (1, x + cols * unit - 64 * scale)):
            rows = max(1, hs[0 if side == 0 else -1] - 1)
            for d in range(depth):
                for r in range(max(1, rows - d)):
                    put(WALL, hx, y - d * unit, g + r * unit, "CityWall", yaw=-90, s=scale, folder=folder)
        # corner pillars
        for px in (x - 30 * scale, x + cols * unit + 30 * scale):
            put(PILLAR_BIG, px, y - 140 * scale, g, "CityPillar", s=scale * local.uniform(0.85, 1.0), sz=scale * local.uniform(0.6, 1.05), folder=folder)
        # roof slabs only over full height stretches, resting on the wall top
        top_full = max(hs)
        for i in range(cols):
            if hs[i] == top_full and local.random() < 0.55:
                put(ROOF, x + i * unit + 16 * scale, y - 200 * scale, g + hs[i] * unit + 13 * scale, "CityRoof", s=scale, folder=folder)
            if local.random() < 0.4:
                put(IVY[local.randrange(len(IVY))], x + i * unit + local.uniform(40, 216) * scale, y + 70 * scale, g + hs[i] * unit - 5,
                    "Ivy", s=local.uniform(1.2, 2.0) * scale * (1 + 0.25 * (hs[i] - 1)), folder=folder)
        # a big jungle tree grows out of some ruins
        if local.random() < (0.25 if sparse else 0.45):
            put(CEIBA, x + local.uniform(0, cols * unit), y - local.uniform(200, 900) * scale, -30, "CityCeiba",
                yaw=local.uniform(0, 360), s=local.uniform(0.35, 0.55) * max(1.0, scale * 0.7), folder=folder)
        gap = local.uniform(0.6, 2.2) * unit if not sparse else local.uniform(1.5, 3.5) * unit
        x += cols * unit + gap


def ground_cover():
    """Stony dirt patches, moss and loose tiles on the walking ground, so the plain ground material never shows alone."""
    G = F + "/Ground"
    x = 32000
    while x < 41900:
        y = rnd.uniform(-260, 120)
        put(rnd.choice(DIRT), x, y, ground(x, y) - 10, "PathDirt", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.7, 1.1))
        if rnd.random() < 0.5:
            tx = x + rnd.uniform(-150, 150)
            put(rnd.choice(TILES), tx, rnd.uniform(-200, 100), ground(tx, 0) + 3, "PathTile", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.3, 2.0))
        x += rnd.uniform(220, 380)


def undergrowth():
    G = F + "/Undergrowth"
    for i in range(380):
        x = rnd.uniform(32000, 42000)
        y = rnd.choice([rnd.uniform(-1350, -260), rnd.uniform(-1350, -260), rnd.uniform(160, 250)])
        if not free(x, y) or lg.in_cave(x, 100):
            continue
        z = ground(x, y)
        k = rnd.random()
        # small near the ruins so they stay readable, bigger further back
        near = y > -1000
        if k < 0.55:
            put(rnd.choice(FERNS), x, y, z - 3, "Fern", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.9, 1.5) if near else rnd.uniform(1.4, 2.2))
        elif k < 0.8:
            put(rnd.choice(MONSTERA), x, y, z - 3, "Monstera", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.8, 1.3) if near else rnd.uniform(1.2, 1.8))
        elif k < 0.9:
            put(HELICONIA, x, y, z - 2, "Heliconia", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.5, 4.0))
        else:
            put(rnd.choice(DIRT), x, y, z - 10, "Dirt", folder=G, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.6, 1.0))


def clear_ops():
    return [
        {"op": "delete", "folder": F + "/Ruins"},
        {"op": "delete", "folder": F + "/Debris"},
        {"op": "delete", "folder": F + "/Undergrowth"},
        {"op": "delete", "folder": F + "/Lights"},
        {"op": "delete", "folder": F + "/Jungle", "label_prefix": "Palm"},   # giant palm leaves lying in the air
        {"op": "delete", "label_prefix": "Platform_Stone"},
        {"op": "delete", "label_prefix": "Torch_Light"},
        {"op": "delete", "folder": F + "/FG", "label_prefix": "FG_Trunk"},  # dead trunks right before the camera cut the ruins in half
        {"op": "delete", "folder": F + "/City"},
        {"op": "delete", "folder": F + "/CityFar"},
        {"op": "delete", "folder": F + "/Ground"},
        # the floating jump ledges and their invisible boxes go; the spruce forest behind gives way to the city
        {"op": "delete", "folder": F + "/Platforms"},
        {"op": "delete", "label_prefix": "Platform_Collision", "x_min": 31900, "x_max": 42100},
        {"op": "delete", "folder": F + "/Trees_R3"},
        {"op": "delete", "folder": F + "/Trees_R4"},
    ]


if __name__ == "__main__":
    if "--clear" in sys.argv:
        print(ue.job(clear_ops()))
    else:
        build()
        print("pieces:", len(ops))
        print(ue.job(clear_ops() + ops))

