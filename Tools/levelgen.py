"""Procedural level builder for Lvl_Forest01. Runs through ue.job (the editor-side job runner).

Usage: python levelgen.py <step> [<step> ...]
Steps: clear terrain boundaries background zone1 zone2 zone3 zone4 zone5 zone6 enemies lights moods
Everything generated lives under the 'Gen/...' outliner folders so it can be cleared and rebuilt.
"""
import sys, math, random
import ue

CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"
RAMP = "/Game/LevelPrototyping/Meshes/SM_Ramp.SM_Ramp"
GROUND = "/Game/Amnesia/Materials/M_Amnesia_Ground.M_Amnesia_Ground"
CAVE_GROUND = "/Game/Amnesia/Materials/MI_Ground_Cave.MI_Ground_Cave"
MUD_GROUND = "/Game/Amnesia/Materials/MI_Ground_Mud.MI_Ground_Mud"

X_MIN = -2100.0
X_MAX = 64500.0

SP = "/Game/Stylized_Spruce_Forest/Meshes/"
TREES = [SP + "Trees/SM_Spruce_0%d.SM_Spruce_0%d" % (i, i) for i in range(1, 8)]
SMALL_SPRUCE = SP + "Trees/SM_Small_Spruce_01.SM_Small_Spruce_01"
GRASS = [SP + "Plants/SM_Grass_01.SM_Grass_01", SP + "Plants/SM_Grass_02.SM_Grass_02"]
BUSH = SP + "Plants/SM_Spruce_Bush_01.SM_Spruce_Bush_01"
BUSH2 = SP + "Plants/SM_Bush_01.SM_Bush_01"
FLOWERS = SP + "Plants/SM_Flowers_01.SM_Flowers_01"
PLANTS = [SP + "Plants/SM_Plant_01.SM_Plant_01", SP + "Plants/SM_Blue_Plant_01.SM_Blue_Plant_01",
          SP + "Plants/SM_Red_Plant_01.SM_Red_Plant_01", SP + "Plants/SM_White_Plant_01.SM_White_Plant_01"]
DRY_BUSH = [SP + "Plants/SM_Dry_Bush_01.SM_Dry_Bush_01", SP + "Plants/SM_Dry_Bush_02.SM_Dry_Bush_02"]
DRY_TREE = [SP + "Debris/SM_Dry_Tree_01.SM_Dry_Tree_01", SP + "Debris/SM_Dry_Tree_02.SM_Dry_Tree_02",
            SP + "Debris/SM_Broken_Tree_01.SM_Broken_Tree_01"]
LOG = SP + "Debris/SM_Log_01.SM_Log_01"
STUMP = SP + "Debris/SM_Stump_01.SM_Stump_01"
MUSH = [SP + "Debris/SM_Mushroom_0%d.SM_Mushroom_0%d" % (i, i) for i in (1, 2, 3)]
SPRUCE_ROCKS = [SP + "Rocks/SM_Rock_0%d.SM_Rock_0%d" % (i, i) for i in (1, 2, 3, 6, 7, 8, 9)]
SMALL_ROCKS = [SP + "Rocks/SM_Small_Rock_0%d.SM_Small_Rock_0%d" % (i, i) for i in (1, 2, 3)]

RU = "/Game/AncientTempleRuins/Environment/"
MOSS_ROCK = RU + "Rock/Rock01/SM_Rock01.SM_Rock01"
ROCK_WALL = RU + "Rock/Rockwall_01/SM_RockWall_01.SM_RockWall_01"
STONES = RU + "Rock/Stone01/SM_Stones01_a.SM_Stones01_a"

VG = "/Game/GreenwoodFantasyVillage/Meshes/"

DEC = ["Decoration"]


def G(rel):
    return ue.asset_path(rel) if "." not in rel.split("/")[-1] else rel


# ---------------------------------------------------------------- terrain profile
# Walkable surface along the path, as (x, z) points. Straight lines between points.
PROFILE = [
    (-2100, 0), (9000, 0),
    # zone 2: village
    (10800, 0), (12200, 300), (13500, 300), (14900, 0), (17000, 0), (18200, 220), (19300, 220), (20400, 0),
    # zone 3: snow forest
    (22500, 0), (23700, 350), (25200, 350), (26600, 120), (27600, 120), (28800, 420), (30000, 420), (31200, 0),
    # zone 4: ruins
    (33200, 0), (34400, 250), (36800, 250), (38000, 0), (39000, 0), (40000, 160), (41000, 160), (41800, 0),
    # zone 5: cave, underground
    (42000, 0), (45000, -1400), (46800, -1400), (48000, -1150), (49000, -1150), (50200, -1400), (51000, -1400), (54000, 0),
    # zone 6: storm forest
    (55500, 0), (56800, 300), (58200, 300), (59600, 0), (60800, 0), (62000, 260), (63000, 260), (63800, 0), (64500, 0),
]

CAVE_X0, CAVE_X1 = 42000.0, 54000.0
CEIL_GAP = 1050.0


def h(x):
    """Walkable ground height at x."""
    if x <= PROFILE[0][0]:
        return PROFILE[0][1]
    for (x0, z0), (x1, z1) in zip(PROFILE, PROFILE[1:]):
        if x0 <= x <= x1:
            if x1 == x0:
                return z1
            return z0 + (z1 - z0) * (x - x0) / (x1 - x0)
    return PROFILE[-1][1]


def slope_deg(x):
    for (x0, z0), (x1, z1) in zip(PROFILE, PROFILE[1:]):
        if x0 <= x <= x1 and x1 != x0:
            return abs(math.degrees(math.atan2(z1 - z0, x1 - x0)))
    return 0.0


def flat_at(x, margin=250):
    return slope_deg(x) < 0.5 and slope_deg(x - margin) < 0.5 and slope_deg(x + margin) < 0.5


def slab(x0, z0, x1, z1, thick, y0, y1, label, folder, material=GROUND, collision="BlockAll", hidden=False, tags=None):
    """A box whose top face runs from (x0,z0) to (x1,z1), thick units deep, spanning y0..y1."""
    dx, dz = x1 - x0, z1 - z0
    length = math.hypot(dx, dz)
    theta = math.atan2(dz, dx)
    s, c = math.sin(theta), math.cos(theta)
    corner_x = x0 + thick * s
    corner_z = z0 - thick * c
    return ue.sm(CUBE, corner_x, y0, corner_z, pitch=math.degrees(theta), sx=length / 100.0, sy=(y1 - y0) / 100.0,
                 sz=thick / 100.0, label=label, folder=folder, collision=collision, material=material, hidden=hidden, tags=tags)


def box(x0, x1, y0, y1, z0, z1, label, folder, material=GROUND, collision="BlockAll", hidden=False, tags=None):
    return ue.sm(CUBE, x0, y0, z0, sx=(x1 - x0) / 100.0, sy=(y1 - y0) / 100.0, sz=(z1 - z0) / 100.0, label=label,
                 folder=folder, collision=collision, material=material, hidden=hidden, tags=tags)


# ---------------------------------------------------------------- steps
def step_clear():
    ops = [{"op": "delete", "folder": "Gen"}]
    print(ue.job(ops))


def step_terrain():
    ops = []
    F = "Gen/Terrain"
    # surface floors: the old floor is resized, a second one continues after the cave
    ops.append({"op": "set_transform", "label": "ForestFloor", "loc": [-8000, -9000, -1000], "scale": [(CAVE_X0 + 8000) / 100.0, 92.6, 10]})
    ops.append(box(CAVE_X1, 76000, -9000, 260, -1000, 0, "ForestFloor_B", F, GROUND))

    # the ground profile as slabs, only where it leaves z=0, or inside the cave
    for (x0, z0), (x1, z1) in zip(PROFILE, PROFILE[1:]):
        if x1 <= 9000:
            continue
        if z0 == 0 and z1 == 0 and not (CAVE_X0 <= x0 < CAVE_X1):
            continue
        inside_cave = CAVE_X0 <= x0 and x1 <= CAVE_X1
        mat = CAVE_GROUND if inside_cave else GROUND
        thick = 3000 if inside_cave else 700
        ops.append(slab(x0, z0, x1, z1, thick, -1400, 140, "Ground_Slab", F, mat))

    # cave: ceiling, mountain mass, back wall
    cave_pts = [(x, z) for (x, z) in PROFILE if CAVE_X0 <= x <= CAVE_X1]
    cp = [(CAVE_X0, 1100)] + [(x, z + CEIL_GAP) for (x, z) in cave_pts[1:-1]] + [(CAVE_X1, 1100)]
    # the cave mouth is tall, then the ceiling follows the floor
    cp = [(CAVE_X0, 1500), (43500, h(43500) + CEIL_GAP)] + [(x, z + CEIL_GAP) for (x, z) in cave_pts if 45000 <= x <= 51000] + [(52500, h(52500) + CEIL_GAP), (CAVE_X1, 1500)]
    for (x0, z0), (x1, z1) in zip(cp, cp[1:]):
        # slab top = ceiling bottom + thickness, so build it with the bottom line and a negative offset
        ops.append(slab(x0, z0 + 3500, x1, z1 + 3500, 3500, -700, 300, "Cave_Ceiling", F + "/Cave", CAVE_GROUND))
    ops.append(box(CAVE_X0 - 200, CAVE_X1 + 200, -760, -640, -3200, 4800, "Cave_BackWall", F + "/Cave", CAVE_GROUND))

    # invisible walls at both ends of the level
    ops.append({"op": "set_transform", "label": "Wall_Right", "loc": [X_MAX, -400, -100], "scale": [1, 7, 25]})
    print(ue.job(ops))


def step_boundaries():
    F = "Gen/Boundaries"
    rnd = random.Random(7)
    # the old rock piles go, the invisible walls stay
    ops = [{"op": "delete", "folder": "Boundaries", "label_prefix": "Rocks_"}]

    def cliff(x_edge, direction, tag):
        # direction -1 grows to the left of x_edge, +1 to the right. Everything starts beyond the invisible wall
        for layer in range(3):
            for i in range(6):
                s = rnd.uniform(1.15, 1.6) - layer * 0.1
                half = 513 * s
                x = x_edge + direction * (half * 0.8 + 150 + i * 760 + rnd.uniform(-60, 60))
                z = layer * 300 - 60
                y = -(368 * s) - 80 - layer * 120
                ops.append(ue.sm(ROCK_WALL, x, y, z, yaw=rnd.uniform(-8, 8), s=s, label=tag + "_Wall", folder=F, tags=DEC))
        for i in range(8):
            s = rnd.uniform(1.1, 1.9)
            x = x_edge + direction * (500 * s + rnd.uniform(100, 3200))
            ops.append(ue.sm(MOSS_ROCK, x, -400 - rnd.uniform(0, 300), -50, yaw=rnd.uniform(0, 360), s=s,
                             label=tag + "_Rock", folder=F, tags=DEC))
        for i in range(6):
            x = x_edge + direction * (rnd.uniform(400, 3400))
            ops.append(ue.sm(rnd.choice(TREES), x, -900 - rnd.uniform(0, 400), -30, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.8, 1.05),
                             label=tag + "_Tree", folder=F, tags=DEC))

    cliff(X_MIN - 100, -1, "Left")
    cliff(X_MAX + 100, +1, "Right")
    print(ue.job(ops))


# ---------------------------------------------------------------- generic builders
def in_cave(x, margin=0):
    return CAVE_X0 - margin <= x <= CAVE_X1 + margin


def ground_z(x, y):
    """Surface height at (x, y): hills only cover y in [-1400, 140]."""
    if -1400 <= y <= 140:
        return h(x)
    return 0.0


# tree rows: (name, y centre, y jitter, scale range, spacing)
ROWS = [
    ("R0", -480, 140, (0.55, 0.8), 330),
    ("R1", -1000, 200, (0.75, 0.95), 230),
    ("R2", -1950, 260, (0.95, 1.2), 260),
    ("R3", -3700, 700, (1.0, 1.4), 310),
    ("R4", -6000, 600, (1.5, 1.7), 380),
    ("R5", -8500, 700, (1.9, 2.3), 600),
]


def forest_rows(ops, x0, x1, folder, species, rnd, dens=1.0, rows=ROWS, zfn=None, seed_off=0.0, small=None):
    """Fills rows of trees between x0 and x1. species is a list of tree meshes."""
    for name, yc, yj, (s0, s1), spacing in rows:
        x = x0 + rnd.uniform(0, spacing / dens)
        while x < x1:
            if not in_cave(x, 150):
                y = yc + rnd.uniform(-yj, yj)
                z = ground_z(x, y) - 30
                mesh = rnd.choice(species)
                if small and name == "R0":
                    mesh = small
                ops.append(ue.sm(mesh, x, y, z, yaw=rnd.uniform(0, 360), s=rnd.uniform(s0, s1), label="Tree_" + name, folder=folder + "/Trees_" + name, tags=DEC))
            x += (spacing / dens) * rnd.uniform(0.6, 1.4)


def scatter(ops, x0, x1, folder, label, meshes, rnd, spacing, y_range, s_range, z_off=0.0, yaw_range=(0, 360), skip_flat=False, on_ground=True, tilt=0.0):
    """Scatters meshes along x at random depth. spacing is the mean distance between objects."""
    x = x0 + rnd.uniform(0, spacing)
    while x < x1:
        if not in_cave(x, 100):
            y = rnd.uniform(*y_range)
            z = (ground_z(x, y) if on_ground else 0.0) + z_off
            ops.append(ue.sm(rnd.choice(meshes), x, y, z, yaw=rnd.uniform(*yaw_range), s=rnd.uniform(*s_range),
                             pitch=rnd.uniform(-tilt, tilt), roll=rnd.uniform(-tilt, tilt), label=label, folder=folder, tags=DEC))
        x += spacing * rnd.uniform(0.5, 1.5)


def path_grass(ops, x0, x1, folder, rnd, spacing=95, meshes=GRASS, scale=(2.2, 3.3), flowers=0.0, skip=None):
    """Grass clumps along the walkable path, on hills too."""
    for ylane, jit in ((-180, 60), (-60, 50), (70, 60), (170, 50), (250, 10)):
        x = x0 + rnd.uniform(0, spacing)
        while x < x1:
            if not in_cave(x, 100) and not (skip and skip(x)):
                y = ylane + rnd.uniform(-jit, jit)
                z = ground_z(x, y) - 2
                mesh = FLOWERS if rnd.random() < flowers else rnd.choice(meshes)
                ops.append(ue.sm(mesh, x, y, z, yaw=rnd.uniform(0, 360), s=rnd.uniform(*scale), label="Grass", folder=folder, tags=DEC))
            x += spacing * rnd.uniform(0.7, 1.4)


def foreground(ops, x0, x1, folder, rnd, spacing=420, trunk_every=1900, rock_mesh=None):
    """Big blurry silhouettes at the camera side: bushes and trunks below the ground line."""
    scatter(ops, x0, x1, folder + "/FG", "FG_Bush", [BUSH, BUSH2], rnd, spacing, (430, 600), (4.6, 6.2), z_off=-250, on_ground=False)
    scatter(ops, x0, x1, folder + "/FG", "FG_Grass", GRASS, rnd, spacing * 0.8, (400, 560), (4.5, 6.5), z_off=-230, on_ground=False)
    scatter(ops, x0, x1, folder + "/FG", "FG_Rock", SPRUCE_ROCKS, rnd, spacing * 2.2, (450, 620), (1.2, 2.2), z_off=-120, on_ground=False)
    scatter(ops, x0, x1, folder + "/FG", "FG_Trunk", [DRY_TREE[0], DRY_TREE[1]], rnd, trunk_every, (620, 700), (1.0, 1.35), z_off=-200, on_ground=False)


def light(ops, x, y, z, color, intensity, radius, flicker=0.0, label="Glow", folder="Gen/Lights"):
    ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaGlowLight", "loc": [x, y, z], "label": label, "folder": folder,
                "props": {"LightColor": "(R=%.3f,G=%.3f,B=%.3f,A=1.0)" % color, "Intensity": intensity, "Radius": radius, "FlickerAmount": flicker}})


def platform(ops, x, ztop, length, kind, folder, depth=260, label="Platform"):
    """A jumpable platform: a hidden collision box plus a visible mesh on top of it."""
    ops.append(box(x - length / 2, x + length / 2, -depth / 2, depth / 2, ztop - 50, ztop, label + "_Collision", folder, collision="BlockAll", hidden=True))
    if kind == "plank":
        planks = VG + "SM_PathwayPlanks.SM_PathwayPlanks"
        # planks are 534 long along y, 192 wide, 14 thick. Turn them to run along x
        n = max(1, int(math.ceil(length / 480.0)))
        for i in range(n):
            cx = x - length / 2 + (i + 0.5) * length / n
            ops.append(ue.sm(planks, cx, 0, ztop - 12, yaw=90, sx=length / n / 534.0, sy=1.4, sz=1.6, label=label + "_Planks", folder=folder, tags=DEC))
        for px in (x - length / 2 + 40, x + length / 2 - 40):
            ops.append(ue.sm(DRY_TREE[0], px, 0, ztop - 20, s=0.45, yaw=0, sz=0.5 + (ztop / 600.0), label=label + "_Post", folder=folder, tags=DEC, collision="NoCollision"))
    elif kind == "rock":
        s = max(0.5, length / 1014.0)
        ops.append(ue.sm(ROCK_WALL, x, -368 * s * 0.5, ztop - 475 * s * 0.92, yaw=0, s=s, label=label + "_Rock", folder=folder, tags=DEC))
        ops.append(ue.sm(MOSS_ROCK, x - length * 0.3, -120, ztop - 380, yaw=rnd_yaw(x), s=1.0, label=label + "_Rock", folder=folder, tags=DEC))
    elif kind == "stone":
        slabm = RU + "Construction/Floor/SM_Floor_02_Linear_512_512_01_A.SM_Floor_02_Linear_512_512_01_A"
        n = max(1, int(math.ceil(length / 512.0)))
        for i in range(n):
            cx = x - length / 2 + i * length / n
            ops.append(ue.sm(slabm, cx, -274, ztop - 108, sx=length / n / 512.0, sy=1.0, sz=1.0, label=label + "_Stone", folder=folder, tags=DEC))


def rnd_yaw(x):
    return (x * 37.0) % 360


# ---------------------------------------------------------------- zones
MOUND = (2450, 4550)  # the old mound in zone 1 has its own ground


def step_background():
    ops = []
    F = "Gen/Background"
    b1 = "/Game/Stylized_Spruce_Forest/Meshes/Background/SM_Background_Landscape_01.SM_Background_Landscape_01"
    b2 = "/Game/Stylized_Spruce_Forest/Meshes/Background/SM_Background_Landscape_02.SM_Background_Landscape_02"
    b3 = "/Game/Stylized_Spruce_Forest/Meshes/Background/SM_Background_Landscape_03.SM_Background_Landscape_03"
    x = 17000
    i = 0
    while x < 76000:
        ops.append(ue.sm([b1, b3][i % 2], x, -14000, -300, yaw=0 if i % 3 else 180, label="Mountains", folder=F, tags=DEC))
        ops.append(ue.sm(b2, x - 5000, -10500, -200, yaw=180 if i % 2 else 0, label="Hills", folder=F, tags=DEC))
        x += 10000
        i += 1
    print(ue.job(ops))


def step_zone1():
    """Zone 1 is the existing forest. Add more trees, undergrowth and flora between the old ones."""
    ops = []
    rnd = random.Random(101)
    F = "Gen/Zone1"
    forest_rows(ops, -3200, 9000, F, TREES, rnd, dens=0.85, rows=[r for r in ROWS if r[0] in ("R1", "R2", "R3", "R4")], small=SMALL_SPRUCE)
    forest_rows(ops, -3200, 9000, F, TREES, rnd, dens=1.0, rows=[ROWS[0]], small=SMALL_SPRUCE)
    scatter(ops, -2500, 9000, F + "/Undergrowth", "Bush", [BUSH, BUSH2], rnd, 240, (-2800, -350), (3.0, 4.6), z_off=-20)
    scatter(ops, -2500, 9000, F + "/Undergrowth", "Plant", PLANTS + [FLOWERS], rnd, 160, (-1500, -200), (2.0, 3.4))
    scatter(ops, -2500, 9000, F + "/Debris", "Mushroom", MUSH, rnd, 700, (-700, -150), (1.6, 2.6))
    scatter(ops, -2500, 9000, F + "/Debris", "Rock", SPRUCE_ROCKS + SMALL_ROCKS, rnd, 800, (-900, -250), (0.9, 1.8), z_off=-30)
    print(ue.job(ops))


def step_grass():
    ops = []
    rnd = random.Random(55)
    path_grass(ops, -2000, 64400, "Gen/PathGrass", rnd, spacing=90, flowers=0.08, skip=lambda x: MOUND[0] <= x <= MOUND[1] or in_cave(x, 0))
    print(ue.job(ops))


def step_zone2():
    """Village in autumn light."""
    ops = []
    rnd = random.Random(202)
    F = "Gen/Zone2"
    X0, X1 = 9000, 21000
    village_trees = TREES + [VG + "SM_Tree_B.SM_Tree_B", VG + "SM_Tree_C.SM_Tree_C"] * 2
    forest_rows(ops, X0, X1, F, village_trees, rnd, dens=1.1, small=SMALL_SPRUCE)
    scatter(ops, X0, X1, F + "/Undergrowth", "Bush", [BUSH, BUSH2], rnd, 220, (-2800, -350), (3.0, 4.6), z_off=-20)
    scatter(ops, X0, X1, F + "/Undergrowth", "BigBush", [VG + "SM_BushBig_A.SM_BushBig_A"], rnd, 600, (-2800, -1300), (1.6, 2.6), z_off=-20)
    scatter(ops, X0, X1, F + "/Undergrowth", "Plant", PLANTS + [FLOWERS, VG + "SM_Flower_A.SM_Flower_A"], rnd, 150, (-1500, -200), (2.0, 3.4))
    foreground(ops, X0, X1, F, rnd)

    # buildings. Front of the meshes is guessed, adjust yaw after looking at them
    HOUSES = [VG + "SM_HouseC.SM_HouseC", VG + "SM_HouseA.SM_HouseA", VG + "SM_HouseB.SM_HouseB", VG + "SM_Barn.SM_Barn"]
    placed = [
        (VG + "SM_HouseC.SM_HouseC", 9900, -650, 1.5), (VG + "SM_Mill.SM_Mill", 10500, -900, 1.9),
        (VG + "SM_HouseA.SM_HouseA", 11900, -1750, 1.9), (VG + "SM_Mill.SM_Mill", 13300, -1700, 2.2),
        (VG + "SM_HouseC.SM_HouseC", 14300, -1750, 1.8),
        (VG + "SM_HouseC.SM_HouseC", 15200, -560, 1.5), (VG + "SM_ShopA.SM_ShopA", 15850, -330, 2.4),
        (VG + "SM_HouseB.SM_HouseB", 16650, -620, 1.5), (VG + "SM_Barn.SM_Barn", 16400, -1100, 1.7),
        (VG + "SM_HouseA.SM_HouseA", 18100, -1750, 1.9), (VG + "SM_HouseC.SM_HouseC", 19300, -1700, 1.8),
        (VG + "SM_Lighthouse.SM_Lighthouse", 19800, -3600, 2.0), (VG + "SM_HouseB.SM_HouseB", 20700, -700, 1.4),
    ]
    for mesh, x, y, s in placed:
        ops.append(ue.sm(mesh, x, y, ground_z(x, y), yaw=0, s=s, label="Building", folder=F + "/Buildings", tags=DEC))

    # village props on the flats
    for x0, x1 in ((9100, 10700), (15000, 16950), (20450, 20950)):
        scatter(ops, x0, x1, F + "/Props", "Crate", [VG + "SM_Crate.SM_Crate", VG + "SM_Barrel.SM_Barrel"], rnd, 330, (-300, -140), (1.7, 2.3))
        scatter(ops, x0, x1, F + "/Props", "Haystack", [VG + "InventoryProps/SM_HaystackA.SM_HaystackA"], rnd, 900, (-420, -250), (0.9, 1.3))
        scatter(ops, x0, x1, F + "/Props", "Fence", [VG + "SM_FenceBig_4m.SM_FenceBig_4m"], rnd, 520, (-230, -210), (1.5, 1.6), yaw_range=(0, 0))
    ops.append(ue.sm(VG + "InventoryProps/SM_Cart.SM_Cart", 16100, -230, 0, yaw=20, s=2.0, label="Cart", folder=F + "/Props", tags=DEC))
    ops.append(ue.sm(VG + "SM_Well.SM_Well", 16350, -260, 0, yaw=0, s=1.7, label="Well", folder=F + "/Props", tags=DEC))
    ops.append(ue.sm(VG + "SM_Bonfire.SM_Bonfire", 15600, -120, 0, s=2.4, label="Bonfire", folder=F + "/Props", tags=DEC))
    ops.append(ue.sm(VG + "SM_WaymarkA.SM_WaymarkA", 9300, -180, 0, s=2.0, label="Waymark", folder=F + "/Props", tags=DEC))
    ops.append(ue.sm(VG + "SM_WaymarkA.SM_WaymarkA", 20800, -180, 0, yaw=180, s=2.0, label="Waymark", folder=F + "/Props", tags=DEC))
    light(ops, 15600, -120, 90, (1.0, 0.55, 0.2), 22, 1100, 0.35, "Bonfire_Light")
    for x in (9500, 10400, 15100, 16800, 20500):
        ops.append(ue.sm(VG + "SM_LightColumn_FireBowl.SM_LightColumn_FireBowl", x, -260, 0, s=1.7, label="Lamp", folder=F + "/Props", tags=DEC))
        light(ops, x, -240, 520, (1.0, 0.6, 0.25), 14, 1000, 0.25, "Lamp_Light")

    # wooden platforms climbing over the first hill
    platform(ops, 11200, 140, 520, "plank", F + "/Platforms", label="Plank")
    platform(ops, 11900, 290, 520, "plank", F + "/Platforms", label="Plank")
    platform(ops, 13700, 450, 600, "plank", F + "/Platforms", label="Plank")
    platform(ops, 18700, 400, 560, "plank", F + "/Platforms", label="Plank")
    platform(ops, 19400, 520, 560, "plank", F + "/Platforms", label="Plank")
    print(ue.job(ops))


def step_zone3():
    """Snowy spruce forest, the densest one."""
    ops = []
    rnd = random.Random(303)
    F = "Gen/Zone3"
    X0, X1 = 21000, 32000
    forest_rows(ops, X0, X1, F, TREES, rnd, dens=1.5, small=SMALL_SPRUCE)
    scatter(ops, X0, X1, F + "/Undergrowth", "Bush", [BUSH, BUSH2, SMALL_SPRUCE], rnd, 170, (-2800, -300), (3.0, 4.8), z_off=-20)
    scatter(ops, X0, X1, F + "/Undergrowth", "Dry", DRY_BUSH, rnd, 260, (-1800, -250), (2.0, 3.2))
    scatter(ops, X0, X1, F + "/Debris", "Log", [LOG, STUMP], rnd, 520, (-700, -200), (1.3, 2.0))
    scatter(ops, X0, X1, F + "/Debris", "Rock", SPRUCE_ROCKS + [MOSS_ROCK, STONES], rnd, 380, (-1200, -250), (0.9, 2.0), z_off=-30)
    foreground(ops, X0, X1, F, rnd, spacing=380, trunk_every=1500)
    # rock ledges to jump along
    platform(ops, 23100, 140, 700, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 24000, 290, 700, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 27100, 330, 800, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 28300, 520, 700, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 30500, 600, 800, "rock", F + "/Platforms", label="Ledge")
    print(ue.job(ops))


def step_zone4():
    """Ancient ruins in a green mist."""
    ops = []
    rnd = random.Random(404)
    F = "Gen/Zone4"
    X0, X1 = 32000, 42000
    JU = RU + "Foliage/"
    jungle = [JU + "Ceiba_Tree_01/SM_Ceiba_Tree_01_A.SM_Ceiba_Tree_01_A", JU + "Palm_Giant_01/SM_PalmGiant_01_A.SM_PalmGiant_01_A"]
    forest_rows(ops, X0, X1, F, TREES, rnd, dens=1.0, rows=[r for r in ROWS if r[0] in ("R3", "R4", "R5")])
    # jungle trees in the middle distance, big canopies
    x = X0
    while x < X1:
        ops.append(ue.sm(jungle[0], x, rnd.uniform(-2600, -1700), -30, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.55, 0.8), label="Ceiba", folder=F + "/Jungle", tags=DEC))
        x += rnd.uniform(1100, 1700)
    scatter(ops, X0, X1, F + "/Jungle", "Palm", [jungle[1]], rnd, 520, (-1500, -350), (1.4, 2.4))
    fern = JU + "Fern_01/SM_Fern_01_A.SM_Fern_01_A"
    mons = JU + "Monstera_01/SM_Monstera_01_Cluster_01.SM_Monstera_01_Cluster_01"
    scatter(ops, X0, X1, F + "/Undergrowth", "Fern", [fern], rnd, 120, (-900, 120), (2.6, 4.2))
    scatter(ops, X0, X1, F + "/Undergrowth", "Monstera", [mons], rnd, 340, (-900, -150), (1.8, 2.8))
    scatter(ops, X0, X1, F + "/Undergrowth", "Bush", [BUSH, BUSH2], rnd, 260, (-2800, -400), (3.0, 4.4), z_off=-20)
    scatter(ops, X0, X1, F + "/Debris", "Roots", [JU + "CeibaRoots_01/SM_Ceiba_Roots_01_B.SM_Ceiba_Roots_01_B"], rnd, 900, (-500, -200), (0.7, 1.2))
    scatter(ops, X0, X1, F + "/Debris", "Stones", [STONES, MOSS_ROCK], rnd, 450, (-900, -200), (0.8, 1.5), z_off=-30)
    foreground(ops, X0, X1, F, rnd, spacing=450, trunk_every=2400)

    # ruin structures
    C = RU + "Construction/"
    pillar_a = C + "Pillar/SM_Pillar_01_128_128_512_01.SM_Pillar_01_128_128_512_01"
    pillar_b = C + "Pillar/SM_Pillar_02_256_512_01.SM_Pillar_02_256_512_01"
    wall = C + "Wall/SM_Wall_01_Linear_256_256_01A.SM_Wall_01_Linear_256_256_01A"
    door = C + "Door/SM_Door_01_A.SM_Door_01_A"
    stairs = C + "Stair/SM_Stairs_02_Linear_800_500_01.SM_Stairs_02_Linear_800_500_01"
    floor2 = C + "Floor/SM_Floor_02_Linear_512_512_01_A.SM_Floor_02_Linear_512_512_01_A"
    engraved = C + "Floor/SM_Floor_Engraved_800_800_01_A.SM_Floor_Engraved_800_800_01_A"
    head = RU + "Prop/StoneHead/SM_StoneHead_04.SM_StoneHead_04"
    torch = RU + "Prop/SM_Torch_01.SM_Torch_01"
    ivy = RU + "Foliage/Ivy_01/SM_Ivy_01_A.SM_Ivy_01_A"

    # pillars along the path, some broken
    px = X0 + 300
    while px < X1 - 200:
        broken = rnd.random() < 0.4
        mesh = pillar_b if rnd.random() < 0.5 else pillar_a
        sc = rnd.uniform(1.0, 1.5)
        z = ground_z(px, -280)
        ops.append(ue.sm(mesh, px, -280, z, yaw=rnd.uniform(-8, 8), sx=sc, sy=sc, sz=sc * (0.45 if broken else 1.0), label="Pillar", folder=F + "/Ruins", tags=DEC))
        if rnd.random() < 0.5:
            ops.append(ue.sm(ivy, px + 60, -200, z + 100, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.6, 2.6), label="Ivy", folder=F + "/Ruins", tags=DEC))
        px += rnd.uniform(650, 1250)

    # broken walls behind the path
    wx = X0 + 200
    while wx < X1:
        run = int(rnd.uniform(2, 6))
        for i in range(run):
            z = ground_z(wx + i * 512, -520)
            ops.append(ue.sm(wall, wx + i * 512, -520, z, yaw=0, sx=2.0, sy=2.0, sz=2.0 * rnd.choice([1.0, 1.0, 0.6, 0.35]), label="Wall", folder=F + "/Ruins", tags=DEC))
            if rnd.random() < 0.4:
                ops.append(ue.sm(wall, wx + i * 512, -520, z + 512, yaw=0, sx=2.0, sy=2.0, sz=2.0 * rnd.choice([0.5, 0.3]), label="Wall", folder=F + "/Ruins", tags=DEC))
        if rnd.random() < 0.5:
            ops.append(ue.sm(door, wx + run * 512 * 0.5, -520, ground_z(wx, -520), yaw=0, s=2.2, label="Door", folder=F + "/Ruins", tags=DEC))
        wx += run * 512 + rnd.uniform(500, 1600)

    # stairs and floors on the two terraces
    for (xa, xb, top) in ((34400, 36800, 250), (40000, 41000, 160)):
        ops.append(ue.sm(floor2, xa + 20, -274, top - 105, sx=(xb - xa) / 512.0, sy=1.0, sz=1.0, label="TerraceFloor", folder=F + "/Ruins", tags=DEC))
    ops.append(ue.sm(stairs, 33200, -200, 2, yaw=0, sx=1.5, sy=1.0, sz=1.15, label="Stairs", folder=F + "/Ruins", tags=DEC))
    ops.append(ue.sm(stairs, 38000, 300, 2, yaw=180, sx=1.5, sy=1.0, sz=1.15, label="Stairs", folder=F + "/Ruins", tags=DEC))
    for x in (33000, 35600, 38800, 40500):
        ops.append(ue.sm(engraved, x, -250, ground_z(x, -250) - 28, s=1.3, label="Circle", folder=F + "/Ruins", tags=DEC))
    for x in (33600, 36000, 39500):
        ops.append(ue.sm(head, x, -180, ground_z(x, -180), yaw=rnd.uniform(-30, 30), s=2.0, label="StoneHead", folder=F + "/Ruins", tags=DEC))
    for x in range(32600, 41800, 700):
        z = ground_z(x, -230)
        ops.append(ue.sm(torch, x, -230, z, s=3.0, label="Torch", folder=F + "/Ruins", tags=DEC))
        light(ops, x, -220, z + 250, (1.0, 0.6, 0.25), 11, 800, 0.3, "Torch_Light")

    platform(ops, 32700, 150, 600, "stone", F + "/Platforms", label="Ledge")
    platform(ops, 33800, 300, 600, "stone", F + "/Platforms", label="Ledge")
    platform(ops, 37200, 470, 700, "stone", F + "/Platforms", label="Ledge")
    platform(ops, 38900, 330, 600, "stone", F + "/Platforms", label="Ledge")
    platform(ops, 40600, 440, 700, "stone", F + "/Platforms", label="Ledge")
    print(ue.job(ops))


def step_zone5():
    """The cave under the mountain."""
    ops = []
    rnd = random.Random(505)
    F = "Gen/Zone5"
    CV = "/Game/MysteryCave/Meshes/"

    def ceil_z(x):
        return h(x) + CEIL_GAP

    # stalactites on the ceiling, big ones at the front as dark silhouettes
    stal = [CV + "SM_stal_StalactiteBig_%d.SM_stal_StalactiteBig_%d" % (i, i) for i in (1, 2, 3, 4, 5)]
    stal_small = [CV + "SM_stal_StalactiteSmall_%d.SM_stal_StalactiteSmall_%d" % (i, i) for i in (1, 2, 3, 4)]
    strip = [CV + "SM_stal_StalactiteStrip_%d.SM_stal_StalactiteStrip_%d" % (i, i) for i in (1, 2, 3)]
    mite = [CV + "SM_stal_StalagmiteBig_%d.SM_stal_StalagmiteBig_%d" % (i, i) for i in (1, 3, 4)]
    mite_small = [CV + "SM_stal_StalagmiteSingle_%d.SM_stal_StalagmiteSingle_%d" % (i, i) for i in (1, 2, 3, 4)]
    mite_tiny = [CV + "SM_stal_StalagmiteTiny_%d.SM_stal_StalagmiteTiny_%d" % (i, i) for i in (1, 2, 3)]
    groups = [CV + "SM_gr_StalagmitesGroup_%d.SM_gr_StalagmitesGroup_%d" % (i, i) for i in (1, 2, 3, 4)]
    pillar = [CV + "SM_stal_Pillar_%d.SM_stal_Pillar_%d" % (i, i) for i in (1, 2, 3)]
    mush_big = [CV + "SM_pln_Mushroom_%s.SM_pln_Mushroom_%s" % (n, n) for n in ("21", "22", "31", "41", "51", "61")]
    bubble = [CV + "SM_pln_MushroomBubble_%d.SM_pln_MushroomBubble_%d" % (i, i) for i in (1, 2, 3, 4)]
    plants = [CV + "SM_pln_Plant%d_1.SM_pln_Plant%d_1" % (i, i) for i in (2, 3, 4, 5, 6)]

    x = CAVE_X0 + 200
    while x < CAVE_X1 - 100:
        y = rnd.choice([rnd.uniform(-600, -100), rnd.uniform(-600, -100), rnd.uniform(160, 330)])
        mesh = rnd.choice(stal + strip + stal_small)
        ops.append(ue.sm(mesh, x, y, ceil_z(x) + 20, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.6, 3.4) if y > 100 else rnd.uniform(1.2, 2.6), label="Stalactite", folder=F + "/Ceiling", tags=DEC))
        x += rnd.uniform(130, 340)
    x = CAVE_X0 + 300
    while x < CAVE_X1 - 100:
        y = rnd.uniform(-650, -120)
        ops.append(ue.sm(rnd.choice(stal_small + stal), x, y, ceil_z(x) + 20, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.8, 1.6), label="Stalactite", folder=F + "/Ceiling", tags=DEC))
        x += rnd.uniform(200, 450)

    # floor: stalagmites, mushrooms, plants
    x = CAVE_X0 + 150
    while x < CAVE_X1:
        y = rnd.uniform(-650, 150)
        z = h(x)
        r = rnd.random()
        if r < 0.3:
            mesh, sc = rnd.choice(mite_small + mite_tiny), rnd.uniform(1.4, 3.0)
        elif r < 0.5:
            mesh, sc = rnd.choice(mite), rnd.uniform(0.6, 1.3)
        elif r < 0.65:
            mesh, sc = rnd.choice(groups), rnd.uniform(0.5, 1.0)
        elif r < 0.85:
            mesh, sc = rnd.choice(mush_big), rnd.uniform(1.6, 3.2)
        else:
            mesh, sc = rnd.choice(plants), rnd.uniform(2.0, 3.6)
        ops.append(ue.sm(mesh, x, y, z, yaw=rnd.uniform(0, 360), s=sc, label="CaveFloor", folder=F + "/Floor", tags=DEC))
        x += rnd.uniform(90, 260)
    # tall pillars connect the floor to the ceiling at the back
    x = CAVE_X0 + 1500
    while x < CAVE_X1 - 800:
        z0 = h(x)
        sc = (CEIL_GAP + 120) / 2223.0
        ops.append(ue.sm(rnd.choice(pillar), x, rnd.uniform(-620, -460), z0 - 40, yaw=rnd.uniform(0, 360), sx=sc * 1.1, sy=sc * 1.1, sz=sc * 1.1, label="Pillar", folder=F + "/Back", tags=DEC))
        x += rnd.uniform(1100, 1900)
    # glowing mushrooms with lights
    glow = [(0.25, 0.75, 1.0), (0.7, 0.35, 1.0), (0.2, 1.0, 0.65)]
    x = CAVE_X0 + 600
    while x < CAVE_X1 - 300:
        z = h(x)
        col = rnd.choice(glow)
        for k in range(3):
            ops.append(ue.sm(rnd.choice(bubble), x + rnd.uniform(-80, 80), rnd.uniform(-300, 40), z + 5, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.4, 4.4), label="GlowMushroom", folder=F + "/Floor", tags=DEC))
        light(ops, x, -150, z + 160, col, 16, 900, 0.12, "Cave_Glow")
        x += rnd.uniform(650, 1000)
    # statue in the middle
    ops.append(ue.sm(CV + "SM_statue.SM_statue", 48500, -300, h(48500) + 60, yaw=0, s=1.6, label="Statue", folder=F + "/Back", tags=DEC))
    light(ops, 48500, -200, h(48500) + 450, (1.0, 0.8, 0.5), 30, 1500, 0.2, "Statue_Glow")

    # cave mouth: rocks, roots and ivy at both ends
    ivy = RU + "Foliage/Ivy_01/SM_Ivy_01_A.SM_Ivy_01_A"
    for side, xe in ((1, CAVE_X0), (-1, CAVE_X1)):
        for i in range(5):
            ops.append(ue.sm(MOSS_ROCK, xe + side * rnd.uniform(-100, 500), rnd.uniform(-400, 80), -60 if side == 1 else 0, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.4, 2.4), label="MouthRock", folder=F + "/Mouth", tags=DEC))
        for i in range(8):
            ops.append(ue.sm(ivy, xe + side * rnd.uniform(0, 900), rnd.uniform(200, 290), 1500 if i % 2 else 1250, yaw=rnd.uniform(-10, 10), s=rnd.uniform(2.5, 4.0), label="MouthIvy", folder=F + "/Mouth", tags=DEC))
    # a torch at each mouth so the entrance reads
    for xt in (CAVE_X0 - 150, CAVE_X1 + 150):
        light(ops, xt, -100, 380, (1.0, 0.6, 0.25), 16, 1200, 0.3, "Mouth_Torch")
    print(ue.job(ops))


def step_zone6():
    """The storm forest: dark, dead trees, mud."""
    ops = []
    rnd = random.Random(606)
    F = "Gen/Zone6"
    X0, X1 = 54000, 64500
    # an open view: only low, dead trees close to the path, so the storm sky shows above them
    low_rows = [("R1", -1100, 300, (0.35, 0.55), 330), ("R2", -2100, 400, (0.4, 0.6), 380)]
    forest_rows(ops, X0, X1, F, [SMALL_SPRUCE] + DRY_TREE, rnd, dens=1.0, rows=low_rows)
    scatter(ops, X0, X1, F + "/Dead", "DeadTree", DRY_TREE, rnd, 520, (-1600, -300), (0.9, 1.5), z_off=-20)
    scatter(ops, X0, X1, F + "/Undergrowth", "DryBush", DRY_BUSH + [BUSH], rnd, 210, (-2500, -300), (2.6, 4.2), z_off=-20)
    scatter(ops, X0, X1, F + "/Debris", "Log", [LOG, STUMP], rnd, 600, (-800, -200), (1.3, 2.2))
    scatter(ops, X0, X1, F + "/Debris", "Rock", SPRUCE_ROCKS + [MOSS_ROCK], rnd, 380, (-1400, -250), (1.0, 2.4), z_off=-40)
    foreground(ops, X0, X1, F, rnd, spacing=420, trunk_every=1400)
    platform(ops, 55800, 150, 650, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 56700, 480, 700, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 60400, 330, 800, "rock", F + "/Platforms", label="Ledge")
    platform(ops, 61400, 520, 700, "rock", F + "/Platforms", label="Ledge")
    print(ue.job(ops))


ENEMY_BP = {
    "Oak": "/Game/Amnesia/Blueprints/BP_AmnesiaEnemy.BP_AmnesiaEnemy_C",
    "Ash": "/Game/Amnesia/Blueprints/BP_Ent_Ash.BP_Ent_Ash_C",
    "Birch": "/Game/Amnesia/Blueprints/BP_Ent_Birch.BP_Ent_Birch_C",
    "Spruce": "/Game/Amnesia/Blueprints/BP_Ent_Spruce.BP_Ent_Spruce_C",
}

# (x from, x to, level from, level to, count)
ENEMY_ZONES = [
    (-600, 8800, 1, 2, 5),
    (9600, 20800, 2, 3, 9),
    (21500, 31800, 3, 5, 11),
    (32500, 41600, 5, 7, 10),
    (42400, 53600, 7, 9, 11),
    (54400, 64000, 9, 10, 10),
]


def step_enemies():
    rnd = random.Random(909)
    ops = [{"op": "delete", "folder": "Gen/Enemies"}]
    kinds = list(ENEMY_BP.keys())
    n = 0
    for x0, x1, l0, l1, count in ENEMY_ZONES:
        xs = []
        tries = 0
        while len(xs) < count and tries < 400:
            tries += 1
            x = rnd.uniform(x0, x1)
            if any(abs(x - o) < 650 for o in xs):
                continue
            if MOUND[0] - 300 < x < MOUND[1] + 300 and x0 < 8000:
                continue
            if not flat_at(x, 220):
                continue
            xs.append(x)
        xs.sort()
        for i, x in enumerate(xs):
            lvl = int(round(l0 + (l1 - l0) * (i / max(1, len(xs) - 1)) + rnd.choice([0, 0, 1, -1] if l1 > l0 else [0])))
            lvl = max(l0, min(l1, lvl))
            kind = kinds[(i + int(x / 1000)) % 4] if rnd.random() < 0.65 else rnd.choice(kinds)
            n += 1
            ops.append({"op": "spawn_actor", "class": ENEMY_BP[kind], "loc": [x, 0, h(x) + 320], "rot": [0, 180 if rnd.random() < 0.5 else 0, 0],
                        "label": "Ent_%02d_%s_L%d" % (n, kind, lvl), "folder": "Gen/Enemies", "props": {"EnemyLevel": lvl}})
    # the four ents of the first version stay, at their own levels and away from the rest places
    for label, x, lvl in OLD_ENTS:
        ops.append({"op": "set_props", "label": label, "props": {"EnemyLevel": lvl}})
        ops.append({"op": "set_transform", "label": label, "loc": [x, 0, h(x) + 320]})
    print(ue.job(ops))


MOOD_ZONES = [
    ("Zone1_Forest", -2100, 9000, ['Summer Golden', 'Summer Blue']),
    ("Zone2_Village", 9000, 21000, ['Autumn Golden', 'Summer Blue']),
    ("Zone3_Snow", 21000, 32000, ['Winter Golden', 'Winter Blue']),
    ("Zone4_Ruins", 32000, 42000, ['Ruins Mist', 'Ruins Rain']),
    ("Zone5_Cave", 42000, 54000, ['Cave']),
    ("Zone6_Storm", 54000, 64500, ['Storm']),
]


def step_moods():
    import json
    ops = [{"op": "delete", "label_prefix": "MoodZone_"}, {"op": "delete", "folder": "Gen/Moods"}, {"op": "delete", "label_prefix": "Weather"}]
    for name, xa, xb, moods in MOOD_ZONES:
        text = "(" + ",".join('"%s"' % m for m in moods) + ")"
        ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaMoodVolume", "loc": [(xa + xb) / 2.0, 0, 1000], "label": "MoodZone_" + name,
                    "folder": "Gen/Moods", "props": {"MoodsOnEnter": text, "BlendTime": 5}})
    ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaWeather", "loc": [0, 0, 500], "label": "Weather", "folder": "Lighting"})
    print(ue.job(ops))
    # box extents live on the volume's component
    for name, xa, xb, moods in MOOD_ZONES:
        for a in ue.call(ue.S, "find_actors", name="MoodZone_" + name, collision_channels=[], tag=""):
            if "UEDPIE" in a["refPath"]:
                continue
            for c in ue.call(ue.A, "get_components", actor=a):
                if c["refPath"].endswith(":Box"):
                    ue.props(c, BoxExtent={"x": (xb - xa) / 2.0, "y": 1500, "z": 5000})
    print("extents set")


STEPS = {k[5:]: v for k, v in globals().items() if k.startswith("step_")}

if __name__ == "__main__":
    for name in sys.argv[1:]:
        print("==", name)
        STEPS[name]()
