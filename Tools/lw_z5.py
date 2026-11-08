"""Zone 5 of the long world: the cave, 120000 cm. The floor dips from the surface and runs through rooms at different depths, tunnels and
caverns of different height. Per chunk: floor slabs and ceiling boxes (collision), the rock lumps that clad the back wall, the ceiling caps
and stalactites, mounds, stalagmites, mushrooms and plants on the floor, glowing mushrooms with lights, columns, now and then a pond or the statue.

The rules learned in the first cave (cave.py): the ceiling boxes are cut so their bottom face is exactly the ceiling line, nothing tall may reach
within 150 cm of the hero's line (decorations have no collision), nothing may stand above the floor edge at y 100..300 (it blocks the camera)."""
import math
import lw, lwb
import levelgen as lg

Z = lw.ZONE["Z5"]
CV = "/Game/MysteryCave/Meshes/"
NAMES = ["A barlang szája", "Cseppköves folyosó", "Gombakert", "Mélyedés", "A szobor csarnoka", "Tavas üreg", "Oszlopcsarnok", "Kristálytó", "Szűk átjáró", "A kijárat"]
HERO_CLEAR = 150.0


def mp(n):
    return CV + n + "." + n


def seq(prefix, nums):
    return [prefix % n for n in nums]


MESH = {
    "cap": seq("SM_stal_StalactiteCeil_%d", (1, 2, 3)),
    "lump": seq("SM_stal_StalagmiteWall_%d", (1, 2, 3, 4)),
    "mound": seq("SM_stal_StalagmiteFloor_%d", (1, 2, 3, 4)),
    "hang_big": seq("SM_stal_StalactiteBig_%d", (1, 2, 3, 4, 5)),
    "hang_small": seq("SM_stal_StalactiteSmall_%d", (1, 2, 3, 4, 5, 6)),
    "hang_strip": seq("SM_stal_StalactiteStrip_%d", (1, 2, 3, 4, 5, 6, 7)),
    "hang_w": seq("SM_stal_StalactitesW_%d", (1, 2, 3)) + ["SM_stal_StalactitesWG_1", "SM_stal_StalactitesM_3"],
    "mite_big": seq("SM_stal_StalagmiteBig_%d", (1, 3, 4, 5, 6)),
    "mite_single": seq("SM_stal_StalagmiteSingle_%d", (1, 2, 3, 4, 6, 7, 8, 9)),
    "mite_tiny": seq("SM_stal_StalagmiteTiny_%d", (1, 2, 3, 4, 5)),
    "group": seq("SM_gr_StalagmitesGroup_%d", (1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14)),
    "mushroom": seq("SM_pln_Mushroom_%s", ("11", "12", "13", "21", "22", "23", "24", "25", "26", "27", "31", "32", "33", "34", "35", "41", "42", "43", "51", "52", "61", "62", "63")),
    "bubble": seq("SM_pln_MushroomBubble_%d", (1, 2, 3, 4, 5, 6, 7)),
    "plant": ["SM_pln_Plant%d_1" % i for i in range(1, 10)],
    "pillar": seq("SM_stal_Pillar_%d", (1, 2, 3)),
    "statue": ["SM_statue"],
}
ALL = sorted({n for v in MESH.values() for n in v})
B = {}


def load_bounds():
    for n, v in lwb.load([mp(n) for n in ALL]).items():
        if v:
            B[n.split("/")[-1].split(".")[0]] = v
    for k in MESH:
        MESH[k] = [n for n in MESH[k] if n in B]


load_bounds()
WATER_MESH = "/Game/Stylized_Spruce_Forest/Meshes/SM_River.SM_River"
WATER_MAT = "/Game/Amnesia/Materials/MI_River_Water.MI_River_Water"
GLOW = [(0.25, 0.75, 1.0), (0.7, 0.35, 1.0), (0.2, 1.0, 0.65), (1.0, 0.45, 0.8)]


def cb(x):
    return lw.cave_ceiling(x)


def floor_low(x, r):
    return min(lw.h(x - r), lw.h(x), lw.h(x + r))


def floor_high(x, r):
    return max(lw.h(x - r), lw.h(x), lw.h(x + r))


def ceil_low(x, r):
    return min(cb(x - r), cb(x), cb(x + r))


def shell(c):
    """Collision: floor slabs along the profile and the ceiling boxes along the ceiling line, both from the segments that start in this chunk."""
    pts = lw.PROFILE.pts
    gaps = lw.CAVE["gaps"]
    for (xa, za), (xb, zb) in zip(pts, pts[1:]):
        if not (Z.x0 <= xa < Z.x1) or not (c.x0 <= xa < c.x1):
            continue
        c.ops.append(lg.slab(xa, za, xb, zb, 3000, -1400, 140, "LW_CaveFloor", c.folder, lg.CAVE_GROUND))
    cp = [(x, lw.h(x) + g) for x, g in gaps]
    THICK = 3500.0
    for (xa, za), (xb, zb) in zip(cp, cp[1:]):
        if not (c.x0 <= xa < c.x1):
            continue
        th = math.atan2(zb - za, xb - xa)
        s, co = math.sin(th), math.cos(th)
        ext = 300.0
        ax, az = xa - ext * co, za - ext * s
        length = math.hypot(xb - xa, zb - za) + 2 * ext
        tx, tz = ax - THICK * s, az + THICK * co
        c.ops.append(lg.slab(tx, tz, tx + length * co, tz + length * s, THICK, -700, 300, "LW_CaveCeiling", c.folder, lg.CAVE_GROUND))
    # the flat wall behind everything, from the floor to the ceiling, over the chunk
    z_lo = min(lw.h(x) for x in (c.x0, (c.x0 + c.x1) / 2, c.x1)) - 3200
    c.ops.append(lg.box(c.x0 - 200, c.x1 + 200, -760, -640, z_lo, 5200, "LW_CaveBackWall", c.folder, lg.CAVE_GROUND))


def stand(c, name, x, y, sc, tilt=0.0, rcap=190.0, shadow=True, yaw=None):
    """A piece on the floor behind the hero's line; tall pieces are shrunk and moved back until their footprint clears y -150."""
    b = B[name]
    reach = max(abs(b[0]), abs(b[1]), abs(b[3]), abs(b[4]))
    if (b[5] - b[2]) * sc > 45.0:
        sc = min(sc, rcap / reach)
        y = min(y, -HERO_CLEAR - reach * sc)
        y = max(y, -700.0 + reach * sc * 0.3)
    r = max(b[3] - b[0], b[4] - b[1]) * 0.5 * sc * 0.4
    z = floor_low(x, r) - b[2] * sc - 6
    c.inst(mp(name), x, y, z, yaw=yaw, s=sc, pitch=c.rnd.uniform(-tilt, tilt), roll=c.rnd.uniform(-tilt, tilt), shadow=shadow)


def hang(c, name, x, y, sc):
    b = B[name]
    r = max(b[3] - b[0], b[4] - b[1]) * 0.5 * sc * 0.5
    room = ceil_low(x, r) - floor_high(x, r) - 330.0
    length = (b[5] - b[2]) * sc
    if length > room:
        sc *= room / length
        if sc < 0.4:
            return
    c.inst(mp(name), x, y, ceil_low(x, r) + 40 - b[5] * sc, s=sc)


def build(c):
    rnd = c.rnd
    c.guard_y = lambda mesh, y, a, b, d: y          # the cave places everything by its own rule (stand, columns)
    theme = rnd.choice(["grotto", "hall", "forest", "pond", "statue"])
    x0, x1 = c.x0, c.x1
    shell(c)

    # ---- ceiling cladding: caps glued to the line, never over the hero's line
    x = x0 + 100
    while x < x1 - 50:
        for y in (-560, -300, 260):
            c.inst(mp(rnd.choice(MESH["cap"])), x + rnd.uniform(-120, 120), y + rnd.uniform(-60, 60), cb(x) + 12, s=rnd.uniform(1.6, 2.4), shadow=False)
        x += rnd.uniform(360, 520)

    # ---- stalactites
    x = x0 + 250
    while x < x1 - 150:
        r = rnd.random()
        if r < 0.08:
            hang(c, rnd.choice(MESH["hang_big"]), x, rnd.uniform(160, 260), rnd.uniform(1.0, 1.7))
        elif r < 0.45:
            hang(c, rnd.choice(MESH["hang_small"]), x, rnd.choice((rnd.uniform(-620, -180), rnd.uniform(200, 280))), rnd.uniform(1.4, 2.8))
        elif r < 0.7:
            hang(c, rnd.choice(MESH["hang_strip"]), x, rnd.uniform(-640, -200), rnd.uniform(1.4, 2.4))
        else:
            hang(c, rnd.choice(MESH["hang_w"]), x, rnd.uniform(-600, -100), rnd.uniform(1.0, 1.8))
        x += rnd.uniform(110, 260)

    # ---- back wall: two layers of rock lumps flattened along their bulge
    for ywall, sc_lo, sc_hi, step_x, step_z, back, bulge in ((-640, 1.9, 2.8, 480, 520, True, 0.38), (-520, 1.2, 2.0, 600, 560, False, 0.3)):
        x = x0 - 300
        while x < x1 + 300:
            zlow = floor_low(x, 300) - 200
            ztop = cb(x) + 250
            z = zlow + rnd.uniform(0, 200)
            while z < ztop:
                if back or rnd.random() < 0.6:
                    sc = rnd.uniform(sc_lo, sc_hi)
                    c.inst(mp(rnd.choice(MESH["lump"])), x + rnd.uniform(-90, 90), ywall + rnd.uniform(-40, 40), z, sx=sc * bulge, sy=sc, sz=sc,
                           yaw=90 + rnd.uniform(-25, 25), pitch=rnd.uniform(-12, 12), roll=rnd.uniform(-12, 12), shadow=False)
                z += step_z * rnd.uniform(0.8, 1.15)
            x += step_x * rnd.uniform(0.8, 1.2)

    # ---- front lip: lumps down from the floor edge into the dark, below the edge only
    x = x0 - 400
    while x < x1 + 400:
        zf = floor_low(x, 250)
        for dz in (-780, -1350, -1900):
            sc = rnd.uniform(2.2, 2.6)
            c.inst(mp(rnd.choice(MESH["lump"])), x + rnd.uniform(-60, 60), 120 + rnd.uniform(-20, 20), zf + dz, sx=sc * 0.3, sy=sc, sz=sc,
                   yaw=90 + rnd.uniform(-30, 30), pitch=rnd.uniform(-4, 4), roll=rnd.uniform(-4, 4), shadow=False)
        if int(x / 1000) % 2 == 0:
            c.light(x, 460, zf - 330, (0.35, 0.5, 1.0), 22, 1300, 0.05, "Cave_Glow")
        x += rnd.uniform(480, 640)

    # ---- floor
    for px, pz in lw.PROFILE.pts:
        if x0 + 100 < px < x1 - 100:
            for _ in range(3):
                stand(c, rnd.choice(MESH["mound"]), px + rnd.uniform(-260, 260), rnd.uniform(-540, -250), rnd.uniform(0.7, 1.2), tilt=4, rcap=260)
    weights = {"grotto": (0.14, 0.26, 0.38, 0.62, 0.9), "hall": (0.2, 0.36, 0.5, 0.7, 0.85),
               "forest": (0.42, 0.62, 0.74, 0.86, 0.95), "pond": (0.2, 0.36, 0.5, 0.7, 0.85), "statue": (0.2, 0.36, 0.5, 0.7, 0.85)}[theme]
    x = x0 + 120
    while x < x1:
        y = rnd.uniform(-640, -140)
        r = rnd.random()
        if r < weights[0]:
            stand(c, rnd.choice(MESH["mite_single"] + MESH["mite_tiny"]), x, y, rnd.uniform(1.4, 3.0))
        elif r < weights[1]:
            stand(c, rnd.choice(MESH["mite_big"]), x, y, rnd.uniform(0.6, 1.3))
        elif r < weights[2]:
            stand(c, rnd.choice(MESH["group"]), x, y, rnd.uniform(0.5, 1.0))
        elif r < weights[3]:
            stand(c, rnd.choice(MESH["mushroom"]), x, y, rnd.uniform(1.6, 3.2))
        elif r < weights[4]:
            stand(c, rnd.choice(MESH["plant"]), x, y, rnd.uniform(2.0, 3.6))
        else:
            stand(c, rnd.choice(MESH["mound"][:2]), x, y, rnd.uniform(0.8, 1.4), tilt=4)
        x += rnd.uniform(70, 170)

    # ---- columns floor to ceiling, many in the hall
    x = x0 + 800
    step = (700, 1200) if theme == "hall" else (1000, 1700)
    while x < x1 - 600:
        name = rnd.choice(MESH["pillar"])
        b = B[name]
        zf = floor_low(x, 200)
        gap = cb(x) - zf
        sz = (gap + 160) / (b[5] - b[2])
        reach = max(abs(b[0]), abs(b[1]), abs(b[3]), abs(b[4]))
        sxy = min(rnd.uniform(0.7, 1.05), 290.0 / reach)
        yp = -HERO_CLEAR - reach * sxy - rnd.uniform(0, 60)
        c.inst(mp(name), x, yp, zf - b[2] * sz - 40, sx=sxy, sy=sxy, sz=sz, shadow=False)
        x += rnd.uniform(*step)

    # ---- glowing mushrooms with lights
    x = x0 + 400
    spacing = (380, 600) if theme == "grotto" else (520, 820)
    while x < x1 - 250:
        col = rnd.choice(GLOW)
        for _ in range(rnd.randint(3, 5)):
            stand(c, rnd.choice(MESH["bubble"]), x + rnd.uniform(-130, 130), rnd.uniform(-420, -110), rnd.uniform(2.2, 4.0))
        c.light(x, -150, floor_low(x, 100) + 170, col, 16, 950, 0.12, "Cave_Glow")
        x += rnd.uniform(*spacing)

    # ---- a pond behind the path or the statue hall, on flat floor
    flats = []
    xx = x0 + 800
    while xx < x1 - 1600:
        if lw.flat_at(xx, 700):
            flats.append(xx)
        xx += 200
    if flats and theme == "pond":
        px = rnd.choice(flats)
        for k in range(6):
            wx = px - 1200 + k * 400
            c.inst(WATER_MESH, wx, -430, lw.h(wx) + 28, yaw=0, sx=0.45, sy=0.38, sz=1.0, shadow=False, material=WATER_MAT)
        c.light(px, -430, lw.h(px) + 120, (0.3, 0.8, 1.0), 40, 2200, 0.15, "Pond_Glow")
    if flats and theme == "statue":
        sx_ = rnd.choice(flats)
        bs = B["SM_statue"]
        reach_s = max(abs(bs[0]), abs(bs[1]), abs(bs[3]), abs(bs[4])) * 1.6
        sy_ = -HERO_CLEAR - reach_s - 40
        stand(c, MESH["mound"][0], sx_, sy_, 2.6, rcap=reach_s + 120)
        c.single(mp("SM_statue"), sx_, sy_, lw.h(sx_) + 60, "Statue", s=1.6)
        c.light(sx_, sy_ + 100, lw.h(sx_) + 450, (1.0, 0.8, 0.5), 30, 1500, 0.2, "Statue_Glow")

    # ---- the mouths: rocks, ivy and a torch at both ends of the cave
    if c.k == 0 or c.k == Z.n - 1:
        xe = Z.x0 if c.k == 0 else Z.x1
        side = 1 if c.k == 0 else -1
        for i in range(5):
            # big rocks, but behind the hero's line: the rock is about 674 wide at scale 1, so its centre sits more than its half width behind y 0
            s = rnd.uniform(1.0, 1.8)
            c.inst(lg.MOSS_ROCK, xe + side * rnd.uniform(-100, 500), -(337 * s * 1.05 + 120) - rnd.uniform(0, 160), (-60 if side == 1 else 0) + lw.h(xe), s=s)
        c.light(xe - side * 150, -100, 380, (1.0, 0.6, 0.25), 16, 1200, 0.3, "Mouth_Torch")

    lw.checkpoint(c, NAMES[c.k % len(NAMES)], dx=700.0)
    lw.enemies(c, ["Skeleton", "Skeleton", "Golem", "Golem"], 5)
    lw.orbs(c, 2)
