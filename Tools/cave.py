"""Rebuilds the cave (zone 5, x 42000..54000) so it reads as rock instead of boxes.

What was wrong: the ceiling was a chain of tilted boxes whose bottoms did not follow the intended line (sagging up to 3.6 m on the
slopes, black gaps at the joints), the stalactites hung from a line that was not the ceiling, the back wall was a flat plane, the floor
ended in a bare diagonal edge over a black void and there was little decoration.

What this does:
  ceiling   boxes re-cut so their bottom face is exactly the ceiling line, cladded with the pack's ceiling caps, stalactites hang from that line
  back wall rock lumps (StalagmiteWall) in two layers over the flat wall
  front lip rock lumps hanging from the floor edge down into the dark
  floor     mounds over the floor kinks, stalagmites, groups, mushrooms, plants, glowing mushrooms with lights, pillars, the statue
Usage: python cave.py"""
import math, random
import ue
import levelgen as lg

CV = "/Game/MysteryCave/Meshes/"
rnd = random.Random(5150)
F = "Gen/Zone5"
DEC = ["Decoration"]
X0, X1 = lg.CAVE_X0, lg.CAVE_X1
GAP = lg.CEIL_GAP


def mp(n):
    return CV + n + "." + n


def seq(prefix, nums):
    return [prefix % n for n in nums]


NAMES = {
    "cap": seq("SM_stal_StalactiteCeil_%d", (1, 2, 3)),
    "lump": seq("SM_stal_StalagmiteWall_%d", (1, 2, 3, 4)),
    "mound": seq("SM_stal_StalagmiteFloor_%d", (1, 2, 3, 4)),
    "hang_big": seq("SM_stal_StalactiteBig_%d", (1, 2, 3, 4, 5)),
    "hang_small": seq("SM_stal_StalactiteSmall_%d", (1, 2, 3, 4, 5, 6)),
    "hang_strip": seq("SM_stal_StalactiteStrip_%d", (1, 2, 3, 4, 5, 6, 7)),
    "hang_w": seq("SM_stal_StalactitesW_%d", (1, 2, 3)) + ["SM_stal_StalactitesWG_1"] + seq("SM_stal_StalactitesM_%d", (3,)),
    "mite_big": seq("SM_stal_StalagmiteBig_%d", (1, 3, 4, 5, 6)),
    "mite_single": seq("SM_stal_StalagmiteSingle_%d", (1, 2, 3, 4, 6, 7, 8, 9)),
    "mite_tiny": seq("SM_stal_StalagmiteTiny_%d", (1, 2, 3, 4, 5)),
    "group": seq("SM_gr_StalagmitesGroup_%d", tuple(range(1, 15))),
    "mushroom": seq("SM_pln_Mushroom_%s", ("11", "12", "13", "21", "22", "23", "24", "25", "26", "27", "31", "32", "33", "34", "35", "41", "42", "43", "51", "52", "61", "62", "63")),
    "bubble": seq("SM_pln_MushroomBubble_%d", (1, 2, 3, 4, 5, 6, 7)),
    "plant": ["SM_pln_Plant%d_1" % i for i in range(1, 10)],
    "pillar": seq("SM_stal_Pillar_%d", (1, 2, 3)),
    "statue": ["SM_statue"],
}
ALL = sorted({n for v in NAMES.values() for n in v})

# ---------------------------------------------------------------- mesh sizes
B = {}
rep = ue.job([{"op": "mesh_bounds", "meshes": [mp(n) for n in ALL]}])["report"]
for line in rep:
    parts = line.split()
    if len(parts) == 7:
        B[parts[0].split("/")[-1].split(".")[0]] = tuple(float(v) for v in parts[1:])
missing = [n for n in ALL if n not in B]
print("meshes with bounds: %d, missing: %s" % (len(B), missing))
for k in NAMES:
    NAMES[k] = [n for n in NAMES[k] if n in B]

ops = [{"op": "delete", "folder": F + "/Ceiling"}, {"op": "delete", "folder": F + "/Floor"}, {"op": "delete", "folder": F + "/Back"},
       {"op": "delete", "folder": F + "/Rock"}, {"op": "delete", "label_prefix": "Cave_Ceiling"}, {"op": "delete", "label_prefix": "Cave_Glow"},
       {"op": "delete", "label_prefix": "Statue_Glow"}]


def put(name, x, y, z, label, folder, s=1.0, yaw=0.0, pitch=0.0, roll=0.0, sx=None, sy=None, sz=None, shadow=None):
    ops.append(ue.sm(mp(name), x, y, z, yaw=yaw, pitch=pitch, roll=roll, s=s, sx=sx, sy=sy, sz=sz, label=label, folder=folder, tags=DEC, shadow=shadow))


# ---------------------------------------------------------------- the ceiling line: bottom face of the ceiling boxes
h = lg.h
CP = [(X0, 1500), (43500, h(43500) + GAP)] + [(x, z + GAP) for (x, z) in lg.PROFILE if 45000 <= x <= 51000] + [(52500, h(52500) + GAP), (X1, 1500)]


def cb(x):
    if x <= CP[0][0]:
        return CP[0][1]
    for (xa, za), (xb, zb) in zip(CP, CP[1:]):
        if xa <= x <= xb:
            return za + (zb - za) * (x - xa) / (xb - xa)
    return CP[-1][1]


def ceil_angle(x):
    return math.degrees(math.atan2(cb(x + 60) - cb(x - 60), 120.0))


def floor_low(x, r):
    return min(h(x - r), h(x), h(x + r))


def ceil_low(x, r):
    return min(cb(x - r), cb(x), cb(x + r))


# re-cut ceiling boxes: the face towards the cave is exactly the line, overlapping at the joints
THICK = 3500.0
for (xa, za), (xb, zb) in zip(CP, CP[1:]):
    th = math.atan2(zb - za, xb - xa)
    s, c = math.sin(th), math.cos(th)
    ext = 300.0
    ax, az = xa - ext * c, za - ext * s
    length = math.hypot(xb - xa, zb - za) + 2 * ext
    tx, tz = ax - THICK * s, az + THICK * c      # the top face line = the bottom line moved up along the normal
    ops.append(lg.slab(tx, tz, tx + length * c, tz + length * s, THICK, -700, 300, "Cave_Ceiling", F + "/Cave", lg.CAVE_GROUND))

# ---------------------------------------------------------------- ceiling cladding: caps glued to the line, y over the whole width
x = X0 + 100
while x < X1 - 50:
    # not over the hero's line (y 0): even a double jump must not push through the ceiling rock
    for y in (-560, -300, 260):
        yy = y + rnd.uniform(-60, 60)
        sc = rnd.uniform(1.6, 2.4)
        name = rnd.choice(NAMES["cap"])
        put(name, x + rnd.uniform(-120, 120), yy, cb(x) + 12, "CeilCap", F + "/Ceiling", s=sc, yaw=rnd.uniform(0, 360), pitch=ceil_angle(x) * 0.0, shadow=False)
    x += rnd.uniform(360, 520)


def hang(name, x, y, sc, label="Stalactite", folder=F + "/Ceiling"):
    b = B[name]
    r = max(b[3] - b[0], b[4] - b[1]) * 0.5 * sc * 0.5
    # the tip must stay 3.3 m above the floor: the hero and its jumps pass under without touching
    room = ceil_low(x, r) - max(h(x - r), h(x), h(x + r)) - 330.0
    length = (b[5] - b[2]) * sc
    if length > room:
        sc *= room / length
        if sc < 0.4:
            return
    z = ceil_low(x, r) + 40 - b[5] * sc            # the top of the mesh just inside the ceiling
    put(name, x, y, z, label, folder, s=sc, yaw=rnd.uniform(0, 360))


# stalactites: big dark ones near the camera side as silhouettes, many small ones, curtains and clusters at the back
x = X0 + 250
while x < X1 - 150:
    r = rnd.random()
    if r < 0.08:
        # a few dark silhouettes on the camera side, small enough not to block the view
        hang(rnd.choice(NAMES["hang_big"]), x, rnd.uniform(160, 260), rnd.uniform(1.0, 1.7))
    elif r < 0.45:
        hang(rnd.choice(NAMES["hang_small"]), x, rnd.choice((rnd.uniform(-620, -180), rnd.uniform(200, 280))), rnd.uniform(1.4, 2.8))
    elif r < 0.7:
        hang(rnd.choice(NAMES["hang_strip"]), x, rnd.uniform(-640, -200), rnd.uniform(1.4, 2.4))
    else:
        hang(rnd.choice(NAMES["hang_w"]), x, rnd.uniform(-600, -100), rnd.uniform(1.0, 1.8))
    x += rnd.uniform(110, 260)

# ---------------------------------------------------------------- back wall: two layers of rock lumps over the flat wall
for layer, (ywall, sc_lo, sc_hi, step_x, step_z, back, bulge) in enumerate(((-640, 1.9, 2.8, 480, 520, True, 0.38), (-520, 1.2, 2.0, 600, 560, False, 0.3))):
    x = X0 - 300
    while x < X1 + 300:
        zlow = floor_low(x, 300) - 200
        ztop = cb(x) + 250
        z = zlow + rnd.uniform(0, 200)
        while z < ztop:
            if back or rnd.random() < 0.6:
                name = rnd.choice(NAMES["lump"])
                sc = rnd.uniform(sc_lo, sc_hi)
                # the bulge of the lump is its +x side: turn it towards the camera (+y)
                # flattened along the bulge: the wall is rough, but never comes near the hero at y 0 or the camera
                put(name, x + rnd.uniform(-90, 90), ywall + rnd.uniform(-40, 40), z, "WallLump", F + "/Rock", sx=sc * bulge, sy=sc, sz=sc, yaw=90 + rnd.uniform(-25, 25),
                    pitch=rnd.uniform(-12, 12), roll=rnd.uniform(-12, 12), shadow=False)
            z += step_z * rnd.uniform(0.8, 1.15)
        x += step_x * rnd.uniform(0.8, 1.2)

# ---------------------------------------------------------------- front lip: lumps down from the floor edge into the dark
x = X0 - 400
while x < X1 + 400:
    zf = floor_low(x, 250)
    for k, dz in enumerate((-780, -1350, -1900)):
        name = rnd.choice(NAMES["lump"])
        sc = rnd.uniform(2.2, 2.6)       # the top of a lump is 300 * scale above its pivot: it must end below the floor edge
        # the front face of the floor slab is at y 140; the lumps stand on it and bulge only a little further towards the camera
        put(name, x + rnd.uniform(-60, 60), 120 + rnd.uniform(-20, 20), zf + dz, "FrontLump", F + "/Rock", sx=sc * 0.3, sy=sc, sz=sc, yaw=90 + rnd.uniform(-30, 30),
            pitch=rnd.uniform(-4, 4), roll=rnd.uniform(-4, 4), shadow=False)
    # a faint cold light under the edge, or the front of the cave floor is a black void
    lg.light(ops, x, 460, zf - 330, (0.35, 0.5, 1.0), 22, 1300, 0.05, "Cave_Glow", folder=F + "/Lights")
    x += rnd.uniform(480, 640)

# ---------------------------------------------------------------- floor
FL = F + "/Floor"


HERO_CLEAR = 150.0    # the hero walks at y 0 and decorations have no collision: nothing tall may reach this close


def stand(name, x, y, sc, label, folder=FL, tilt=0.0, rcap=190.0, **kw):
    """Puts a piece on the floor, behind the hero's line. A piece taller than 45 cm is shrunk to a footprint radius of rcap and moved back
    until its whole footprint is behind y -HERO_CLEAR, so the hero never walks into it and disappears."""
    b = B[name]
    reach = max(abs(b[0]), abs(b[1]), abs(b[3]), abs(b[4]))          # the widest the mesh gets around its pivot, at scale 1
    if (b[5] - b[2]) * sc > 45.0:
        sc = min(sc, rcap / reach)
        y = min(y, -HERO_CLEAR - reach * sc)
        y = max(y, -700.0 + reach * sc * 0.3)
    r = max(b[3] - b[0], b[4] - b[1]) * 0.5 * sc * 0.4
    z = floor_low(x, r) - b[2] * sc - 6
    put(name, x, y, z, label, folder, s=sc, yaw=rnd.uniform(0, 360), pitch=rnd.uniform(-tilt, tilt), roll=rnd.uniform(-tilt, tilt), **kw)


# rounded mounds over every kink of the floor profile, so no straight line shows
for px, pz in lg.PROFILE:
    if X0 + 100 < px < X1 - 100:
        for _ in range(3):
            stand(rnd.choice(NAMES["mound"]), px + rnd.uniform(-260, 260), rnd.uniform(-540, -250), rnd.uniform(0.7, 1.2), "KinkMound", tilt=4, rcap=260)

x = X0 + 120
while x < X1:
    y = rnd.uniform(-640, -140)
    r = rnd.random()
    if r < 0.2:
        stand(rnd.choice(NAMES["mite_single"] + NAMES["mite_tiny"]), x, y, rnd.uniform(1.4, 3.0), "CaveFloor")
    elif r < 0.36:
        stand(rnd.choice(NAMES["mite_big"]), x, y, rnd.uniform(0.6, 1.3), "CaveFloor")
    elif r < 0.5:
        stand(rnd.choice(NAMES["group"]), x, y, rnd.uniform(0.5, 1.0), "CaveFloor")
    elif r < 0.7:
        stand(rnd.choice(NAMES["mushroom"]), x, y, rnd.uniform(1.6, 3.2), "CaveFloor")
    elif r < 0.85:
        stand(rnd.choice(NAMES["plant"]), x, y, rnd.uniform(2.0, 3.6), "CaveFloor")
    else:
        stand(rnd.choice(NAMES["mound"][:2]), x, y, rnd.uniform(0.8, 1.4), "CaveFloor", tilt=4)
    x += rnd.uniform(70, 170)

# columns from floor to ceiling at the back
x = X0 + 1300
while x < X1 - 700:
    name = rnd.choice(NAMES["pillar"])
    b = B[name]
    zf = floor_low(x, 200)
    sz = (cb(x) - zf + 160) / (b[5] - b[2])
    reach = max(abs(b[0]), abs(b[1]), abs(b[3]), abs(b[4]))
    sxy = min(rnd.uniform(0.7, 1.05), 290.0 / reach)                 # the footprint stays behind the hero's line
    yp = -HERO_CLEAR - reach * sxy - rnd.uniform(0, 60)
    put(name, x, yp, zf - b[2] * sz - 40, "Pillar", F + "/Back", sx=sxy, sy=sxy, sz=sz, yaw=rnd.uniform(0, 360))
    x += rnd.uniform(1000, 1700)

# glowing mushroom clusters, each with a light
GLOW = [(0.25, 0.75, 1.0), (0.7, 0.35, 1.0), (0.2, 1.0, 0.65), (1.0, 0.45, 0.8)]
x = X0 + 400
while x < X1 - 250:
    col = rnd.choice(GLOW)
    for _ in range(rnd.randint(3, 5)):
        stand(rnd.choice(NAMES["bubble"]), x + rnd.uniform(-130, 130), rnd.uniform(-420, -110), rnd.uniform(2.2, 4.0), "GlowMushroom")
    lg.light(ops, x, -150, floor_low(x, 100) + 170, col, 16, 950, 0.12, "Cave_Glow", folder=F + "/Lights")
    x += rnd.uniform(520, 820)

# the statue in the middle, on a mound
SX = 48500
bs = B["SM_statue"]
reach_s = max(abs(bs[0]), abs(bs[1]), abs(bs[3]), abs(bs[4])) * 1.6
SY = -HERO_CLEAR - reach_s - 40
print("statue footprint radius %.0f, placed at y %.0f" % (reach_s, SY))
stand(NAMES["mound"][0], SX, SY, 2.6, "StatueMound", folder=F + "/Back", rcap=reach_s + 120)
put("SM_statue", SX, SY, h(SX) + 60, "Statue", F + "/Back", s=1.6)
lg.light(ops, SX, SY + 100, h(SX) + 450, (1.0, 0.8, 0.5), 30, 1500, 0.2, "Statue_Glow", folder=F + "/Lights")

print("pieces:", len(ops))
print(ue.job(ops + [{"op": "save"}], timeout=900))
