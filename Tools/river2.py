"""A winding river behind the first forest section: curved and straight stretches, a carved bed, banks, trees back in.
Run: python river2.py   (PIE must be stopped)"""
import math, random
import ue
import levelgen as lg

X0, X1 = -2800.0, 8800.0
STEP = 55.0
BED_Z, WATER_Z = -150, -38
MUD, GROUND = lg.MUD_GROUND, lg.GROUND
WATER_MESH = "/Game/Stylized_Spruce_Forest/Meshes/SM_River.SM_River"
WATER_MAT = "/Game/Amnesia/Materials/MI_River_Water.MI_River_Water"
SMALL = lg.SMALL_ROCKS + lg.SPRUCE_ROCKS[:3]

# centre line control points (x, y, width). Repeated y values give straight stretches
CTRL = [
    (-2800, -5400, 700), (-2100, -4700, 760), (-1300, -3700, 820), (-600, -2900, 860),
    (200, -2450, 880), (1300, -2400, 880), (2500, -2400, 860),          # straight
    (3300, -2650, 800), (4000, -3250, 760), (4800, -3650, 760), (5500, -3400, 800),
    (6200, -2800, 860), (6900, -2500, 880), (7800, -2500, 860),         # straight again
    (8300, -3100, 800), (8800, -4300, 720), (9300, -5400, 700),
]


def catmull(p0, p1, p2, p3, t):
    return 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t)


def sample(x):
    """Returns (y centre, width, slope dy/dx) of the river at x."""
    pts = CTRL
    i = 0
    while i < len(pts) - 2 and pts[i + 1][0] <= x:
        i += 1
    p0 = pts[max(i - 1, 0)]
    p1, p2 = pts[i], pts[i + 1]
    p3 = pts[min(i + 2, len(pts) - 1)]
    t = (x - p1[0]) / float(p2[0] - p1[0])
    t = min(max(t, 0.0), 1.0)
    y = catmull(p0[1], p1[1], p2[1], p3[1], t)
    w = catmull(p0[2], p1[2], p2[2], p3[2], t)
    t2 = min(t + 0.01, 1.0)
    y2 = catmull(p0[1], p1[1], p2[1], p3[1], t2)
    dx = (t2 - t) * (p2[0] - p1[0])
    return y, w, (y2 - y) / dx if dx else 0.0


def exclude(x, y):
    if X0 - 150 <= x <= X1 + 150:
        yc, w, _ = sample(min(max(x, X0), X1))
        # keep trees off the water and a bit of the bank
        return abs(y - yc) < w / 2 + 130
    return False


def build():
    F = "Gen/River"
    ops = []
    # floor pieces: everything left of the river start is one box, then thin slabs follow the channel
    ops.append(lg.box(-8000, X0, -9000, 260, -1000, 0, "RiverFloor_Left", F, GROUND))
    x = X0
    n = 0
    while x < X1 - 1:
        xa, xb = x, min(x + STEP, X1)
        yc, w, _ = sample((xa + xb) / 2)
        ya, yb = yc - w / 2, yc + w / 2
        ops.append(lg.box(xa, xb + 1, yb, 260, -1000, 0, "RiverFloor_Front", F, GROUND))
        ops.append(lg.box(xa, xb + 1, -9000, ya, -1000, 0, "RiverFloor_Back", F, GROUND))
        ops.append(lg.box(xa, xb + 1, ya, yb, -1000, BED_Z, "RiverBed", F, MUD))
        x = xb
        n += 1
    # water ribbon: tiles turned along the flow
    x = X0 + STEP / 2
    while x < X1:
        yc, w, slope = sample(x)
        yaw = math.degrees(math.atan2(slope, 1.0))
        length = STEP * 1.9 * math.sqrt(1 + slope * slope)
        ops.append(ue.sm(WATER_MESH, x, yc, WATER_Z, yaw=yaw, sx=length / 1000.0, sy=(w * 0.96) / 1000.0, sz=1.0,
                         label="RiverWater", folder=F, tags=["Decoration"], collision="NoCollision", shadow=False, material=WATER_MAT))
        x += STEP
    # banks
    rnd = random.Random(77)
    x = X0 + 60
    while x < X1 - 60:
        yc, w, slope = sample(x)
        for side in (-1, 1):
            edge = yc + side * w / 2
            y = edge + side * rnd.uniform(-10, 130)
            r = rnd.random()
            if r < 0.30:
                ops.append(ue.sm(rnd.choice(SMALL), x, y, -35, yaw=rnd.uniform(0, 360), s=rnd.uniform(1.8, 3.6), label="RiverBank_Rock", folder=F, tags=["Decoration"]))
            elif r < 0.62:
                ops.append(ue.sm(rnd.choice(lg.GRASS), x, y, -8, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.6, 3.8), label="RiverBank_Grass", folder=F, tags=["Decoration"]))
            elif r < 0.80:
                ops.append(ue.sm(rnd.choice([lg.BUSH, lg.BUSH2]), x, y + side * 170, -10, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.0, 3.0), label="RiverBank_Bush", folder=F, tags=["Decoration"]))
            else:
                ops.append(ue.sm(lg.FLOWERS, x, y + side * 60, -8, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.4, 3.2), label="RiverBank_Flowers", folder=F, tags=["Decoration"]))
        x += rnd.uniform(90, 210)
    # reeds in the shallows, stones in the stream, a fallen trunk
    x = X0 + 120
    while x < X1 - 120:
        yc, w, _ = sample(x)
        ops.append(ue.sm(rnd.choice(lg.GRASS), x, yc + rnd.choice((-1, 1)) * (w / 2 - 80), BED_Z + 25, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.4, 3.4), label="RiverReed", folder=F, tags=["Decoration"]))
        x += rnd.uniform(140, 380)
    for x in range(int(X0) + 500, int(X1) - 300, 760):
        xx = x + rnd.uniform(-150, 150)
        yc, w, _ = sample(xx)
        ops.append(ue.sm(rnd.choice(SMALL), xx, yc + rnd.uniform(-w * 0.28, w * 0.28), WATER_Z - 40, yaw=rnd.uniform(0, 360), s=rnd.uniform(2.0, 3.6), label="RiverStone", folder=F, tags=["Decoration"]))
    yc, w, slope = sample(3900)
    ops.append(ue.sm(lg.LOG, 3900, yc, WATER_Z + 30, yaw=90 + math.degrees(math.atan2(slope, 1.0)), s=2.6, label="RiverLog", folder=F, tags=["Decoration"]))
    print("floor slabs", n, "ops", len(ops))
    print(ue.job(ops))


def main():
    print(ue.job([{"op": "delete", "folder": "Gen/River"}, {"op": "delete", "folder": "Gen/Zone1"}]))
    build()
    lg.EXCLUDE = exclude
    lg.step_zone1()
    # a denser belt of trees on both sides of the river, away from the water
    ops = []
    rnd = random.Random(5150)
    F = "Gen/Zone1/RiverTrees"
    for xs in range(-3000, 9000, 175):
        for _ in range(2):
            x = xs + rnd.uniform(0, 175)
            yc, w, _ = sample(min(max(x, X0), X1))
            side = rnd.choice((-1, 1))
            y = yc + side * (w / 2 + rnd.uniform(220, 1500))
            if y > -950 or exclude(x, y):
                continue
            mesh = rnd.choice(lg.TREES + [lg.SMALL_SPRUCE])
            ops.append(ue.sm(mesh, x, y, -30, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.75, 1.25), label="Tree_River", folder=F, tags=["Decoration"]))
    print("river trees", len(ops), ue.job(ops))


if __name__ == "__main__":
    main()
