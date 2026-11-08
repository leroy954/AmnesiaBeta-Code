"""Zone 4 of the long world: the overgrown ruins, 100000 cm. Behind the path three rows of broken buildings in the mist, in front of
them temple pieces on the flat stretches: gates, terraces with colonnades and altars, courtyards, shrines. Jungle plants over everything."""
import math, random
import lw, lwf, lwr
from lwr import m, PILLAR, WALL, WALL_OPEN, TRIM, FLOOR, ENGRAVED, FACADE, BLOCK, RUBBLE, IVY, ROOTS, TORCH, BASE_LOW
import levelgen as lg

Z = lw.ZONE["Z4"]
NAMES = ["Az összeomlott kapu", "Szentély terasz", "Elnyelt udvar", "Moháslépcső", "Az oltár", "Folyosók", "Kriptapitvar", "A régi főtér", "Utolsó terasz"]

FERNS = [m("Foliage/Fern_01/SM_Fern_01_%s" % k) for k in "ABCDEFGH"]
MONSTERA = [m("Foliage/Monstera_01/SM_Monstera_01_Cluster_0%d" % i) for i in (1, 2, 3, 4)]
HELICONIA = m("Foliage/Heliconia_01/SM_Heliconia_01")
TILES = [m("Rock/StoneTiles02/SM_StoneTiles_01_%s" % k) for k in "ABCDEFGH"]
DIRT = [m("GroundScatter/SM_GS_DirtStony_01%s" % k) for k in "cdefg"]
CEIBA = [m("Foliage/Ceiba_Tree_01/SM_Ceiba_Tree_01_%s" % k) for k in "ABC"]
CEIBA2 = [m("Foliage/Ceiba_Tree_02/SM_Tree_02_%s" % k) for k in "ABC"]
POTS = [m("Prop/Pots/SM_Pot_01_A"), m("Prop/Pots/SM_Pot_01_Broken_A_01"), m("Prop/Pots/SM_Pot_03_Broken_A_01"), m("Prop/Pots/SM_Pot_02_Broken_A_01"), m("Prop/Pots/SM_Pot_02_A")]
PILLAR_BIG = m("Construction/Pillar/SM_Pillar_02_256_512_01")      # 236x287x617, y -2..285
ROOF = m("Construction/Ceil/SM_Roof_02_Linear_256_512_01_A")       # 415x284x221, min (-16,-12,-13)
DOOR = m("Construction/Door/SM_Door_01_A")


def flat_spans(c, min_len, step=100.0):
    """Stretches of the chunk where the ground is level for at least min_len."""
    spans = []
    x = c.x0 + 300
    start = None
    while x < c.x1 - 300:
        ok = lw.PROFILE.slope_deg(x) < 0.5
        if ok and start is None:
            start = x
        if (not ok or x + step >= c.x1 - 300) and start is not None:
            if x - start >= min_len:
                spans.append((start, x))
            start = None
        x += step
    return spans


def torch(c, x, y):
    g = lw.ground(x, y)
    c.inst(BASE_LOW, x, y, g - 6, s=0.7)
    c.inst(TORCH, x, y, g - 6 + 104 * 0.7 - 4, s=2.0)
    c.light(x, y + 20, g + 104 * 0.7 + 150, (1.0, 0.58, 0.22), 14, 900, 0.35, "Torch_Light")


def pots(c, x, y, n):
    rnd = c.rnd
    for _ in range(n):
        px, py = x + rnd.uniform(-90, 90), y + rnd.uniform(-60, 60)
        c.inst(rnd.choice(POTS), px, py, lw.ground(px, py) - 3, yaw=rnd.uniform(0, 360), s=rnd.uniform(0.9, 1.25), roll=rnd.choice([0, 0, 0, 80]))


def broken_heights(rnd, columns, tall):
    hs, level = [], tall
    for i in range(columns):
        level = max(0, min(tall, level + rnd.choice([-1, 0, 0, 1])))
        if i in (0, columns - 1):
            level = max(1, level - 1)
        hs.append(level)
    return hs


def gate_site(c, x):
    rnd = c.rnd
    lwr.gate(c, x, -520, 1.05)
    for dx in (-120, 220, 520):
        c.inst(rnd.choice(DIRT), x + dx, -350, lw.ground(x, -350) - 8, s=rnd.uniform(0.8, 1.2))
    lwr.wall_run(c, x - 1300, -1000, broken_heights(rnd, 5, 2), ivy=0.5)
    lwr.wall_run(c, x + 1100, -1000, broken_heights(rnd, 5, 2), ivy=0.5)


def terrace_site(c, x0, x1):
    """Paving, a broken back wall with a doorway and roots, a colonnade, an altar and pots."""
    rnd = c.rnd
    n = max(3, int((x1 - x0 - 200) // 256))
    lwr.paving(c, x0, x1, -800, depth=2, rows_missing=0.28)
    hs = broken_heights(rnd, n, 3)
    lwr.wall_run(c, x0 - 100, -900, hs, ivy=0.4, trim=True)
    mid = x0 + (x1 - x0) * rnd.uniform(0.3, 0.7)
    c.inst(DOOR, mid - 40, -860, lw.ground(mid, -860) - 5, yaw=0)
    c.inst(ROOTS, mid + 500, -1030, lw.ground(mid, -1030) + 918 * 0.45 - 40, yaw=180, s=0.45)
    cols = int((x1 - x0 - 400) // 620)
    lwr.colonnade(c, x0 + 250, cols, -470, 620, 1.0)
    pots(c, x0 + (x1 - x0) * 0.4, -560, 3)
    c.inst(ENGRAVED, mid, -330, lw.ground(mid, -330) - 62 + 2, s=0.9)


def court_site(c, x):
    rnd = c.rnd
    c.inst(ENGRAVED, x, -360, lw.ground(x, -360) - 62 + 2, s=1.0)
    for dx, dy, st in ((-420, -260, "stump"), (420, -260, "broken"), (550, 30, "stump")):
        if dy > -150:
            continue
        lwr.pillar(c, x + dx, dy - 100, st, 0.85)
    torch(c, x - 350, -330)
    torch(c, x + 350, -330)
    lwr.wall_run(c, x - 800, -1250, broken_heights(rnd, 6, 2), ivy=0.5)
    pots(c, x + 100, -620, 3)


def shrine_site(c, x):
    rnd = c.rnd
    g = lw.ground(x, -880)
    lwr.paving(c, x - 600, x + 600, -760, depth=1, rows_missing=0.15)
    c.inst(FACADE, x, -880, g - 4, s=1.15)
    lwr.pillar(c, x - 370, -560, "whole", 0.95, ivy=0.6)
    lwr.pillar(c, x + 370, -560, "broken", 0.95)
    torch(c, x - 220, -430)
    torch(c, x + 220, -430)
    lwr.wall_run(c, x - 1000, -1000, [2, 1], ivy=0.6)
    lwr.wall_run(c, x + 500, -1000, [1, 2, 0], ivy=0.6)


def city_row(c, y, scale, seed_off, sparse=False):
    """A row of ruined buildings: broken wall shells with side returns, arches, corner pillars, roofs on the parts still standing.
    Every building lies completely inside the chunk, so neighbouring chunks never overlap."""
    local = random.Random("city-%s-%d-%d" % (c.zone.key, c.k, seed_off))
    unit = 256.0 * scale
    x = c.x0 + local.uniform(200, unit)
    while x < c.x1:
        cols = local.randint(3, 6)
        if x + cols * unit + 60 * scale > c.x1 - 100:
            break
        tall = local.randint(2, 4)
        hs, level = [], tall
        for i in range(cols):
            level = max(1, min(tall, level + local.choice([-1, 0, 0, 1])))
            hs.append(level)
        hs[0] = max(1, hs[0] - 1)
        g = -20.0 * scale
        i = 0
        while i < cols:
            cx = x + i * unit
            if i + 1 < cols and hs[i] >= 2 and hs[i + 1] >= 2 and local.random() < 0.3:
                c.inst(WALL_OPEN, cx + 5 * scale, y + 100 * scale, g, yaw=0, s=scale * 0.98, shadow=False)
                for r in range(2, min(hs[i], hs[i + 1])):
                    for k in (0, 1):
                        c.inst(WALL, cx + k * unit, y, g + r * unit, yaw=0, s=scale, shadow=False)
                i += 2
                continue
            for r in range(hs[i]):
                c.inst(WALL, cx, y, g + r * unit, yaw=0, s=scale, shadow=False)
            i += 1
        depth = local.randint(2, 3)
        for side, hx in ((0, x), (1, x + cols * unit - 64 * scale)):
            rows = max(1, hs[0 if side == 0 else -1] - 1)
            for d in range(depth):
                for r in range(max(1, rows - d)):
                    c.inst(WALL, hx, y - d * unit, g + r * unit, yaw=-90, s=scale, shadow=False)
        for px in (x - 30 * scale, x + cols * unit + 30 * scale):
            c.inst(PILLAR_BIG, px, y - 140 * scale, g, yaw=0, sx=scale * local.uniform(0.85, 1.0), sy=scale * local.uniform(0.85, 1.0), sz=scale * local.uniform(0.6, 1.05), shadow=False)
        top_full = max(hs)
        for i in range(cols):
            if hs[i] == top_full and local.random() < 0.55:
                c.inst(ROOF, x + i * unit + 16 * scale, y - 200 * scale, g + hs[i] * unit + 13 * scale, yaw=0, s=scale, shadow=False)
            if local.random() < 0.4:
                c.inst(local.choice(IVY), x + i * unit + local.uniform(40, 216) * scale, y + 70 * scale, g + hs[i] * unit - 5,
                       yaw=local.uniform(-20, 20), s=local.uniform(1.2, 2.0) * scale * (1 + 0.25 * (hs[i] - 1)), shadow=False)
        if local.random() < (0.25 if sparse else 0.45):
            c.inst(local.choice(CEIBA), x + local.uniform(0, cols * unit), y - local.uniform(200, 900) * scale, -30, s=local.uniform(0.35, 0.55) * max(1.0, scale * 0.7), shadow=False)
        gap = local.uniform(0.6, 2.2) * unit if not sparse else local.uniform(1.5, 3.5) * unit
        x += cols * unit + gap


def retaining_walls(c):
    """Stone facing on the front of every raised stretch of ground, so no earth face shows from the camera."""
    rnd = c.rnd
    x = c.x0
    while x < c.x1:
        top = max(lw.h(x), lw.h(x + 256))
        if top > 8:
            c.inst(WALL, x, 140, -40, yaw=0, sx=1.0, sy=1.0, sz=(top + 40) / 256.0, shadow=False, guard=False)
            if abs(lw.h(x) - lw.h(x + 256)) < 2:
                c.inst(TRIM, x - 5, 170, top + 69 - 4, yaw=0, shadow=False, guard=False)
            elif rnd.random() < 0.5:
                c.inst(rnd.choice(IVY), x + rnd.uniform(30, 220), 210, top - 5, s=rnd.uniform(0.8, 1.3), shadow=False, guard=False)
        x += 256


def build(c):
    rnd = c.rnd
    city_row(c, -1900, 1.0, 1)
    city_row(c, -3400, 2.1, 2)
    city_row(c, -6200, 3.6, 3, sparse=True)
    retaining_walls(c)

    # temple pieces on the flat stretches
    spans = flat_spans(c, 1900)
    rnd.shuffle(spans)
    kinds = ["terrace", "gate", "court", "shrine", "terrace", "court"]
    rnd.shuffle(kinds)
    used = []
    for (xa, xb), kind in zip(spans, kinds):
        if kind == "terrace":
            x1 = min(xb, xa + rnd.uniform(3000, 4800))
            lw_x = (xa + 100, x1 - 100)
            terrace_site(c, *lw_x)
        elif kind == "gate":
            gate_site(c, xa + 1500)
        elif kind == "court":
            court_site(c, (xa + xb) / 2)
        else:
            shrine_site(c, (xa + xb) / 2)
        used.append((xa, xb))

    # jungle: Ceiba trees behind, plants everywhere the stone left room, stony dirt and loose tiles on the path
    x = c.x0 + rnd.uniform(0, 900)
    while x < c.x1:
        c.inst(rnd.choice(CEIBA + CEIBA2), x, rnd.uniform(-2700, -1700), -30, s=rnd.uniform(0.55, 0.8), shadow=False)
        x += rnd.uniform(1100, 1700)
    for _ in range(420):
        x = rnd.uniform(c.x0, c.x1)
        y = rnd.choice([rnd.uniform(-1350, -260), rnd.uniform(-1350, -260), rnd.uniform(160, 250)])
        if any(xa - 120 < x < xb + 120 and -1000 < y < -250 for xa, xb in used):
            continue
        z = lw.ground(x, y)
        k = rnd.random()
        near = y > -1000
        if k < 0.55:
            c.inst(rnd.choice(FERNS), x, y, z - 3, s=rnd.uniform(0.9, 1.5) if near else rnd.uniform(1.4, 2.2), shadow=False)
        elif k < 0.8:
            c.inst(rnd.choice(MONSTERA), x, y, z - 3, s=rnd.uniform(0.8, 1.3) if near else rnd.uniform(1.2, 1.8), shadow=False)
        elif k < 0.9:
            c.inst(HELICONIA, x, y, z - 2, s=rnd.uniform(2.5, 4.0), shadow=False)
        else:
            c.inst(rnd.choice(DIRT), x, y, z - 10, s=rnd.uniform(0.6, 1.0), shadow=False)
    x = c.x0
    while x < c.x1:
        y = rnd.uniform(-260, 120)
        c.inst(rnd.choice(DIRT), x, y, lw.ground(x, y) - 10, s=rnd.uniform(0.7, 1.1), shadow=False)
        if rnd.random() < 0.5:
            tx = x + rnd.uniform(-150, 150)
            c.inst(rnd.choice(TILES), tx, rnd.uniform(-200, 100), lw.ground(tx, 0) + 3, s=rnd.uniform(1.3, 2.0), shadow=False)
        x += rnd.uniform(220, 380)
    lwf.path_grass(c, scale=(1.3, 1.9), flowers=0.04)
    lwf.back_grass(c, (-330, -520, -720), 170, scale=(1.5, 2.2))
    lwf.plain(c)

    lw.checkpoint(c, NAMES[c.k % len(NAMES)])
    lw.enemies(c, ["Skeleton", "Skeleton", "Golem", "Golem", "Ash"], 6)
    lw.orbs(c, 2)
