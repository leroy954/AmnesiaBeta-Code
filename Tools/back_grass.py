"""Grass behind the walking path, where the ground between the path grass and the trees was bare.
Zones 1, 2, 3 and 6 get it; the ruins (4) and the cave (5) are left as they are. Grass casts no shadow, to keep the shadow maps cheap.
Usage: python back_grass.py            rebuild
       python back_grass.py --clear    only remove it"""
import sys, math, random
import ue
import levelgen as lg

SP = "/Game/Stylized_Spruce_Forest/Meshes/"
FLOWERS = lg.FLOWERS
GRASS = lg.GRASS
PLANTS = lg.PLANTS
DRY = lg.DRY_BUSH

ROOT = "Gen/BackGrass"
rnd = random.Random(8080)
ops = [{"op": "delete", "folder": ROOT}]

# village buildings (x, y, scale) from levelgen.step_zone2: grass stays out of their footprint
HOUSES = [(9900, -650, 1.5), (10500, -900, 1.9), (11900, -1750, 1.9), (13300, -1700, 2.2), (14300, -1750, 1.8), (15200, -560, 1.5),
          (15850, -330, 2.4), (16650, -620, 1.5), (16400, -1100, 1.7), (18100, -1750, 1.9), (19300, -1700, 1.8), (20700, -700, 1.4)]


def in_house(x, y):
    return any(abs(x - hx) < 330 * s and hy - 120 * s < y < hy + 330 * s for hx, hy, s in HOUSES)


def in_mound(x, y):
    return lg.MOUND[0] <= x <= lg.MOUND[1] and y > -700


def lanes(zone, x0, x1, ys, spacing, meshes, extra=None, skip=None, scale=(1.7, 2.5), depth_gain=0.0002, shadow=False):
    """Rows of grass clumps at the given depths. Clumps grow a little with depth so the far ones still read as grass."""
    folder = "%s/%s" % (ROOT, zone)
    n = 0
    for y0 in ys:
        x = x0 + rnd.uniform(0, spacing)
        while x < x1:
            if not lg.in_cave(x, 100):
                y = y0 + rnd.uniform(-70, 70)
                if not (skip and skip(x, y)):
                    pick = rnd.random()
                    mesh = rnd.choice(meshes)
                    if extra:
                        for share, pool in extra:
                            if pick < share:
                                mesh = rnd.choice(pool)
                                break
                            pick -= share
                    gain = 1.0 + abs(y) * depth_gain
                    ops.append(ue.sm(mesh, x, y, lg.ground_z(x, y) - 2, yaw=rnd.uniform(0, 360), s=rnd.uniform(*scale) * gain,
                                     label="BackGrass", folder=folder, tags=["Decoration"], shadow=shadow))
                    n += 1
            x += spacing * rnd.uniform(0.7, 1.3)
    print("%-6s %4d clumps" % (zone, n))


if "--clear" not in sys.argv:
    # zone 1: summer meadow, then the bare strip up to the river and both banks of it. The river meanders (river2.py), so the
    # lanes there follow its centre line instead of fixed depths
    import river2
    RX0, RX1 = river2.X0 - 300, river2.X1 + 300

    def in_water(x, y):
        if RX0 <= x <= RX1:
            yc, w, _ = river2.sample(min(max(x, river2.X0), river2.X1))
            return abs(y - yc) < w / 2 + 30
        return False

    lanes("Zone1", -2100, 10800, (-330, -500, -680, -860, -1000), 105, GRASS, extra=[(0.1, [FLOWERS]), (0.07, PLANTS)],
          skip=lambda x, y: in_mound(x, y))
    lanes("Zone1", -2100, 10800, (-1180, -1380, -1580, -1780, -1980, -2180), 150, GRASS, extra=[(0.08, [FLOWERS]), (0.06, PLANTS)],
          skip=lambda x, y: in_water(x, y))
    # both banks: lanes at fixed distances from the edge of the water
    for side in (-1, 1):
        for d in (60, 230, 460, 800):
            x = river2.X0 - 200
            n = 0
            while x < river2.X1 + 200:
                yc, w, _ = river2.sample(min(max(x, river2.X0), river2.X1))
                y = yc + side * (w / 2 + d + rnd.uniform(-50, 50))
                if y < -120 and not in_water(x, y) and not in_mound(x, y):
                    mesh = FLOWERS if rnd.random() < 0.08 else rnd.choice(GRASS)
                    ops.append(ue.sm(mesh, x, y, -4, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.7, 2.5) * (1.0 + abs(y) * 0.0002),
                                     label="BackGrass", folder=ROOT + "/Banks", tags=["Decoration"], shadow=False))
                    n += 1
                x += 125 * rnd.uniform(0.7, 1.3)
        print("banks side %d done" % side)
    # zone 2: autumn village, around the houses
    lanes("Zone2", 10800, 21000, (-330, -520, -720, -930, -1150), 115, GRASS, extra=[(0.05, [FLOWERS]), (0.06, PLANTS)],
          skip=lambda x, y: in_house(x, y))
    # zone 3: snow, the grass is frosted by the winter parameter
    lanes("Zone3", 21000, 31500, (-330, -520, -720, -930, -1150), 120, GRASS, extra=[(0.12, DRY)])
    # zone 6: dry, tall grass of the storm plain, not on the stone road to the giant
    road = lambda x, y: abs(x - 59900) < 300 and y < -420
    lanes("Zone6", 54800, 64400, (-300, -480, -680, -900, -1150, -1400, -1750, -2200, -2700), 105, GRASS, extra=[(0.2, DRY)], skip=road,
          scale=(2.0, 2.9))

print(ue.job(ops + [{"op": "save"}], timeout=600))
