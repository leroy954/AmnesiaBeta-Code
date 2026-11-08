"""Forest toolkit of the long world: tree rows, scatter, grass lanes, foreground, rock faces on the hills, a meandering river and
small landmarks (camps, stone circles, fallen trunks...). Built on lw.Chunk and the meshes of the first level.
Every function takes the chunk and adds to it; nothing is sent until the chunk is flushed."""
import math, random
import lw, lwb
import levelgen as lg

VG = lg.VG


def vg(name):
    return VG + name + "." + name.split("/")[-1]


GRASS = lg.GRASS
FLOWERS = lg.FLOWERS
PLANTS = lg.PLANTS
TREES = lg.TREES
SMALL_SPRUCE = lg.SMALL_SPRUCE
BUSHES = [lg.BUSH, lg.BUSH2]
ROCKS = lg.SPRUCE_ROCKS
SMALL_ROCKS = lg.SMALL_ROCKS
MUSH = lg.MUSH
DRY_BUSH = lg.DRY_BUSH
DRY_TREE = lg.DRY_TREE
LOG, STUMP = lg.LOG, lg.STUMP
MOSS_ROCK, ROCK_WALL = lg.MOSS_ROCK, lg.ROCK_WALL


# ---------------------------------------------------------------- rows and scatter
def tree_rows(c, species, rows=None, dens=1.0, small=None, skip=None, shadow_rows=("R0", "R1", "R2")):
    """Rows of trees behind the path, the same rows as the first level (near and small, far and huge)."""
    rnd = c.rnd
    for name, yc, yj, (s0, s1), spacing in (rows or lg.ROWS):
        x = c.x0 + rnd.uniform(0, spacing / dens)
        while x < c.x1:
            y = yc + rnd.uniform(-yj, yj)
            if not (skip and skip(x, y, name)):
                mesh = small if (small and name == "R0") else rnd.choice(species)
                c.inst(mesh, x, y, lw.ground(x, y) - 30, s=rnd.uniform(s0, s1), shadow=name in shadow_rows)
            x += (spacing / dens) * rnd.uniform(0.6, 1.4)


def scatter(c, meshes, spacing, y_range, s_range, z_off=0.0, skip=None, on_ground=True, tilt=0.0, shadow=True, x0=None, x1=None, yaw=None):
    rnd = c.rnd
    x = (x0 if x0 is not None else c.x0) + rnd.uniform(0, spacing)
    end = x1 if x1 is not None else c.x1
    while x < end:
        y = rnd.uniform(*y_range)
        if not (skip and skip(x, y)):
            z = (lw.ground(x, y) if on_ground else 0.0) + z_off
            c.inst(rnd.choice(meshes), x, y, z, yaw=yaw, s=rnd.uniform(*s_range), pitch=rnd.uniform(-tilt, tilt), roll=rnd.uniform(-tilt, tilt), shadow=shadow)
        x += spacing * rnd.uniform(0.5, 1.5)


def path_grass(c, spacing=90, scale=(1.5, 2.2), flowers=0.08, skip=None, meshes=None, lanes=((-180, 60), (-60, 50), (70, 60), (170, 50), (250, 10))):
    """Knee-high grass along the walking path, on the hills too."""
    rnd = c.rnd
    meshes = meshes or GRASS
    for ylane, jit in lanes:
        x = c.x0 + rnd.uniform(0, spacing)
        while x < c.x1:
            y = ylane + rnd.uniform(-jit, jit)
            if not (skip and skip(x, y)):
                mesh = FLOWERS if rnd.random() < flowers else rnd.choice(meshes)
                c.inst(mesh, x, y, lw.ground(x, y) - 2, s=rnd.uniform(*scale), shadow=False)
            x += spacing * rnd.uniform(0.7, 1.4)


def back_grass(c, ys, spacing=110, scale=(1.7, 2.5), gain=0.0002, skip=None, meshes=None, extra=None):
    """Grass lanes behind the path. extra: [(share, [meshes])] mixed in (flowers, plants, dry bushes)."""
    rnd = c.rnd
    meshes = meshes or GRASS
    for y0 in ys:
        x = c.x0 + rnd.uniform(0, spacing)
        while x < c.x1:
            y = y0 + rnd.uniform(-70, 70)
            if not (skip and skip(x, y)):
                pick = rnd.random()
                mesh = rnd.choice(meshes)
                for share, pool in (extra or []):
                    if pick < share:
                        mesh = rnd.choice(pool)
                        break
                    pick -= share
                c.inst(mesh, x, y, lw.ground(x, y) - 2, s=rnd.uniform(*scale) * (1.0 + abs(y) * gain), shadow=False)
            x += spacing * rnd.uniform(0.7, 1.3)


def foreground(c, spacing=420, trunk_every=1900, skip=None):
    """Big blurry silhouettes on the camera side, below the ground line: bushes, grass, rocks and dead trunks."""
    scatter(c, BUSHES, spacing, (430, 600), (4.6, 6.2), z_off=-250, on_ground=False, shadow=False, skip=skip)
    scatter(c, GRASS, spacing * 0.8, (400, 560), (4.5, 6.5), z_off=-230, on_ground=False, shadow=False, skip=skip)
    scatter(c, ROCKS, spacing * 2.2, (450, 620), (1.2, 2.2), z_off=-120, on_ground=False, shadow=False, skip=skip)
    scatter(c, DRY_TREE[:2], trunk_every, (620, 700), (1.0, 1.35), z_off=-200, on_ground=False, shadow=False, skip=skip)


def hill_faces(c, step=240, mesh=None):
    """Rock faces on the front of the hills (the slabs' front side), and big rocks along their back edge. The pieces are narrow, so on a
    slope the lowest point under each one is close to the surface and no dark triangle of slab shows above them."""
    rnd = c.rnd
    mesh = mesh or ROCK_WALL
    x = c.x0
    while x < c.x1:
        top = min(lw.h(x), lw.h(x + step))
        if top > 40:
            c.inst(mesh, x + step / 2, 112, -40, yaw=0, sx=step * 1.25 / 1014.0, sy=0.42, sz=(top + 30) / 479.0, shadow=False)
        x += step
    x = c.x0
    while x < c.x1:
        top = lw.h(x)
        if top > 60:
            c.inst(rnd.choice(ROCKS), x, -1400 - rnd.uniform(0, 120), top - 60, s=rnd.uniform(1.6, 3.0), shadow=False)
            c.inst(rnd.choice(BUSHES), x + rnd.uniform(-100, 100), -1380 - rnd.uniform(0, 200), top - 20, s=rnd.uniform(2.6, 4.0), shadow=False)
        x += rnd.uniform(240, 420)


# ---------------------------------------------------------------- the plain behind the path
def plain(c, material=None, river=None, bed_material=None):
    """The ground behind the walking strip as visual boxes. With a river, the boxes stop at its banks and a carved bed goes between."""
    CUBE = lw.CUBE
    material = material or lw.GROUND
    bed_material = bed_material or lg.MUD_GROUND
    spans = river.spans(c.x0, c.x1) if river else []
    x = c.x0
    for xa, xb in spans + [(c.x1, c.x1)]:
        if xa > x:
            c.inst(CUBE, x, -9000, -1000, yaw=0, sx=(xa - x + 1) / 100.0, sy=76.0, sz=10.0, shadow=False, material=material)
        if xb > xa:
            step = 55.0
            xx = xa
            while xx < xb - 1:
                xe = min(xx + step, xb)
                yc, w, _ = river.sample((xx + xe) / 2)
                ya, yb = yc - w / 2, yc + w / 2
                c.inst(CUBE, xx, -9000, -1000, yaw=0, sx=(xe - xx + 1) / 100.0, sy=(ya + 9000) / 100.0, sz=10.0, shadow=False, material=material)
                c.inst(CUBE, xx, yb, -1000, yaw=0, sx=(xe - xx + 1) / 100.0, sy=(-1400 - yb) / 100.0, sz=10.0, shadow=False, material=material)
                c.inst(CUBE, xx, ya, -1000, yaw=0, sx=(xe - xx + 1) / 100.0, sy=(yb - ya) / 100.0, sz=8.5, shadow=False, material=bed_material)
                xx = xe
        x = max(x, xb)


# ---------------------------------------------------------------- the river
WATER_MESH = "/Game/Stylized_Spruce_Forest/Meshes/SM_River.SM_River"
WATER_MAT = "/Game/Amnesia/Materials/MI_River_Water.MI_River_Water"
BED_Z, WATER_Z = -150, -38


def catmull(p0, p1, p2, p3, t):
    return 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t)


class River:
    """A river through x xa..xb: it comes in from far behind the plain, winds between y -2300 and -3900 with straight stretches, goes back out."""

    def __init__(self, xa, xb, seed):
        rnd = random.Random(seed)
        self.xa, self.xb = xa, xb
        pts = [(xa, -5400.0, 700.0), (xa + 900, -4700.0, 760.0)]
        x, y = xa + 900, -4700.0
        while x < xb - 3200:
            x += rnd.uniform(800, 1500)
            if rnd.random() < 0.3:
                pass                                           # a straight stretch repeats y
            else:
                y = max(-3900.0, min(-2300.0, y + rnd.uniform(-900, 900)))
            pts.append((x, y, rnd.uniform(760, 900)))
        pts.append((xb - 1000, -4700.0, 760.0))
        pts.append((xb, -5400.0, 700.0))
        self.pts = pts

    def sample(self, x):
        pts = self.pts
        x = min(max(x, self.xa), self.xb - 0.01)
        i = 0
        while i < len(pts) - 2 and pts[i + 1][0] <= x:
            i += 1
        p0 = pts[max(i - 1, 0)]
        p1, p2 = pts[i], pts[i + 1]
        p3 = pts[min(i + 2, len(pts) - 1)]
        t = min(max((x - p1[0]) / float(p2[0] - p1[0]), 0.0), 1.0)
        y = catmull(p0[1], p1[1], p2[1], p3[1], t)
        w = catmull(p0[2], p1[2], p2[2], p3[2], t)
        t2 = min(t + 0.01, 1.0)
        y2 = catmull(p0[1], p1[1], p2[1], p3[1], t2)
        dx = (t2 - t) * (p2[0] - p1[0])
        return y, w, ((y2 - y) / dx if dx else 0.0)

    def spans(self, x0, x1):
        """The part of the river inside x0..x1, as a list of (xa, xb)."""
        a, b = max(x0, self.xa), min(x1, self.xb)
        return [(a, b)] if b > a else []

    def covers(self, x, y, pad=130):
        if self.xa - 150 <= x <= self.xb + 150:
            yc, w, _ = self.sample(x)
            return abs(y - yc) < w / 2 + pad
        return False

    def build(self, c):
        """Water ribbon, banks with rocks and plants, reeds, stones in the stream and a fallen trunk, for the part inside the chunk."""
        rnd = c.rnd
        for xa, xb in self.spans(c.x0, c.x1):
            x = xa + 27.5
            while x < xb:
                yc, w, slope = self.sample(x)
                yaw = math.degrees(math.atan2(slope, 1.0))
                length = 55.0 * 1.9 * math.sqrt(1 + slope * slope)
                c.inst(WATER_MESH, x, yc, WATER_Z, yaw=yaw, sx=length / 1000.0, sy=(w * 0.96) / 1000.0, sz=1.0, shadow=False, material=WATER_MAT)
                x += 55.0
            x = xa + 60
            while x < xb - 60:
                yc, w, slope = self.sample(x)
                for side in (-1, 1):
                    edge = yc + side * w / 2
                    y = edge + side * rnd.uniform(-10, 130)
                    r = rnd.random()
                    if r < 0.30:
                        c.inst(rnd.choice(SMALL_ROCKS + ROCKS[:3]), x, y, -35, s=rnd.uniform(1.8, 3.6), shadow=False)
                    elif r < 0.62:
                        c.inst(rnd.choice(GRASS), x, y, -8, s=rnd.uniform(2.0, 3.0), shadow=False)
                    elif r < 0.80:
                        c.inst(rnd.choice(BUSHES), x, y + side * 170, -10, s=rnd.uniform(2.0, 3.0), shadow=False)
                    else:
                        c.inst(FLOWERS, x, y + side * 60, -8, s=rnd.uniform(2.0, 2.8), shadow=False)
                x += rnd.uniform(90, 210)
            x = xa + 120
            while x < xb - 120:
                yc, w, _ = self.sample(x)
                c.inst(rnd.choice(GRASS), x, yc + rnd.choice((-1, 1)) * (w / 2 - 80), BED_Z + 25, s=rnd.uniform(2.0, 3.0), shadow=False)
                x += rnd.uniform(140, 380)
            x = xa + 500
            while x < xb - 300:
                xx = x + rnd.uniform(-150, 150)
                yc, w, _ = self.sample(xx)
                c.inst(rnd.choice(SMALL_ROCKS + ROCKS[:3]), xx, yc + rnd.uniform(-w * 0.28, w * 0.28), WATER_Z - 40, s=rnd.uniform(2.0, 3.6), shadow=False)
                x += 760
            # a fallen trunk and now and then a bridge over the water, seen from the path
            xl = rnd.uniform(xa + 800, xb - 800)
            yc, w, slope = self.sample(xl)
            c.inst(LOG, xl, yc, WATER_Z + 30, yaw=90 + math.degrees(math.atan2(slope, 1.0)), s=2.6, shadow=False)
            if rnd.random() < 0.7:
                xbridge = rnd.uniform(xa + 1500, xb - 1500)
                yc, w, slope = self.sample(xbridge)
                c.single(vg("SM_BridgeLong"), xbridge, yc, -20, "RiverBridge", yaw=math.degrees(math.atan2(slope, 1.0)) * 0.0, s=max(w / 1000.0, 1.0) * 1.0)


# ---------------------------------------------------------------- landmarks
def camp(c, x, y=-330):
    """A camp beside the path: a fire with light, seats of logs, crates and barrels, a haystack and a cart."""
    rnd = c.rnd
    g = lw.ground(x, y)
    c.single(vg("SM_Bonfire"), x, y, g, "CampFire", s=2.4)
    c.light(x, y + 20, g + 100, (1.0, 0.55, 0.2), 24, 1200, 0.35, "Camp_Fire")
    for k in range(4):
        a = math.radians(k * 90 + rnd.uniform(-20, 20))
        c.inst(vg("InventoryProps/SM_Logs"), x + math.cos(a) * 180, y + math.sin(a) * 130, lw.ground(x, y), yaw=math.degrees(a) + 90, s=1.7)
    c.inst(vg("SM_Bench"), x - 260, y - 140, g, yaw=0, s=2.0)
    for i in range(3):
        c.inst(rnd.choice([vg("SM_Crate"), vg("SM_Barrel")]), x + 330 + i * 80, y - 200 - rnd.uniform(0, 90), g, s=1.9)
    c.inst(vg("InventoryProps/SM_HaystackA"), x - 520, y - 330, g, s=0.55)
    c.inst(vg("InventoryProps/SM_Cart"), x + 560, y - 180, g, yaw=20, s=2.0)


def stone_circle(c, x, y=-900, r=620):
    rnd = c.rnd
    for k in range(11):
        a = math.radians(k * 360 / 11 + rnd.uniform(-8, 8))
        sx, sy = x + math.cos(a) * r, y + math.sin(a) * r * 0.55
        c.inst(rnd.choice(ROCKS), sx, sy, lw.ground(sx, sy) - 30, s=rnd.uniform(1.7, 2.6))
    c.inst(MOSS_ROCK, x, y, lw.ground(x, y) - 60, s=rnd.uniform(0.8, 1.2))
    for _ in range(14):
        a = rnd.uniform(0, 6.28)
        c.inst(rnd.choice(PLANTS + [FLOWERS]), x + math.cos(a) * r * rnd.uniform(0.2, 0.9), y + math.sin(a) * r * 0.5 * rnd.uniform(0.2, 0.9), lw.ground(x, y), s=rnd.uniform(2.0, 3.0), shadow=False)


def fallen_giant(c, x, y=-520):
    rnd = c.rnd
    g = lw.ground(x, y)
    c.inst(LOG, x, y, g + 20, yaw=90 + rnd.uniform(-8, 8), s=rnd.uniform(4.5, 6.0))
    for _ in range(8):
        c.inst(rnd.choice(MUSH), x + rnd.uniform(-320, 320), y + rnd.uniform(-90, 90), g, s=rnd.uniform(1.8, 3.0), shadow=False)
    c.inst(STUMP, x + 560, y + 50, g, s=2.8)


def rock_garden(c, x, y=-700):
    rnd = c.rnd
    for _ in range(9):
        rx, ry = x + rnd.uniform(-700, 700), y + rnd.uniform(-420, 220)
        rock = rnd.choice(ROCKS + [MOSS_ROCK])
        sc = rnd.uniform(0.6, 1.2) if rock == MOSS_ROCK else (rnd.uniform(1.6, 3.6) if rnd.random() < 0.8 else rnd.uniform(0.5, 0.9))
        c.inst(rock, rx, ry, lw.ground(rx, ry) - 40, s=sc)
    for _ in range(14):
        rx, ry = x + rnd.uniform(-800, 800), y + rnd.uniform(-450, 300)
        c.inst(rnd.choice(PLANTS), rx, ry, lw.ground(rx, ry), s=rnd.uniform(2.0, 3.2), shadow=False)


def wayshrine(c, x, y=-230):
    g = lw.ground(x, y)
    c.single(vg("SM_WaymarkA"), x, y, g, "Waymark", s=2.0)
    c.single(vg("SM_LightColumn_FireBowl"), x + 220, y + 10, g, "Lamp", s=1.7)
    c.light(x + 220, y + 10, g + 520, (1.0, 0.6, 0.25), 14, 1000, 0.25, "Lamp_Light")


def meadow(c, x, width=2200):
    """A glade: dense flowers and plants right behind the path."""
    rnd = c.rnd
    n = int(width / 40)
    for _ in range(n):
        px, py = x + rnd.uniform(-width / 2, width / 2), rnd.uniform(-1300, -250)
        c.inst(rnd.choice([FLOWERS, FLOWERS] + PLANTS), px, py, lw.ground(px, py), s=rnd.uniform(2.0, 3.4), shadow=False)
