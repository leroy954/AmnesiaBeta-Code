"""Ruins kit of the long world (zones 4 and 6): pillars, wall runs, gates, arches, flagstone roads, obelisks, stone rings,
rubble. Pieces are instanced meshes of the AncientTempleRuins pack, set on the ground of the profile.

Reminder of the hero's line: decorations have no collision in the game, so nothing tall may stand within y -150..150.
Everything here stays behind y -200 unless it is flat."""
import math
import lw, lwb
import levelgen as lg

RU = "/Game/AncientTempleRuins/Environment/"


def m(rel):
    return RU + rel + "." + rel.split("/")[-1]


PILLAR = m("Construction/Pillar/SM_Pillar_01_128_128_512_01")      # 146x147x486, bottom centre
PILLAR_THIN = m("Construction/Pillar/SM_Pillar_01_64_64_256_D")    # 67x76x261, bottom centre
BASE = m("Construction/Pillar/SM_PillarBase_01_128_192_01")        # 137x137x199, bottom centre
BASE_LOW = m("Construction/Pillar/SM_PillarBase_01_128_96_01_A")   # 137x137x104
WALL = m("Construction/Wall/SM_Wall_01_Linear_256_256_01A")        # 256x64x256, min corner
WALL_LOW = m("Construction/Wall/SM_Wall_01_Linear_256_128_02")     # 267x58x142
WALL_OPEN = m("Construction/Wall/SM_Wall_01_Open_192_256_01_A")    # 521x199x521 an arch, min (-5,-100,-4)
FLOOR = m("Construction/Floor/SM_Floor_01_Linear_768_512_01_A")    # 768x512x64, top at pivot, min corner
ENGRAVED = m("Construction/Floor/SM_Floor_Engraved_800_800_01_A")  # 602x602, z 32..62
FACADE = m("Construction/Buildings/SM_Facade_Custom_01")           # 473x239x360, x centred, y -1..238
TRIM = m("Construction/Ceil/SM_TopTrim_01_Linear_128_256_A")       # 266x137x73, z -69..4
STAIRS = m("Construction/Stair/SM_Stairs_02_Linear_800_500_01")    # 803x501x218
TORCH = m("Prop/SM_Torch_01")                                      # 10x10x79, bottom
RUBBLE = [m("Rock/Stone02/SM_Stones_02_%s" % k) for k in "abcd"]
UPRIGHT = m("Prop/StoneHead/SM_StoneHead_01_A")                    # 1144x1106x1825, z -619..1206
LINTEL = m("Prop/StoneHead/SM_StoneHead_02")                       # 244x1095x236 (long along y), z -112..124
BLOCK = m("Prop/StoneHead/SM_StoneHead_07")                        # 553x465x298, z -150..148
IVY = [m("Foliage/Ivy_01/SM_Ivy_01_%s" % k) for k in "DEFGH"]      # hangs down from its pivot
ROOTS = m("Foliage/CeibaRoots_01/SM_Ceiba_Roots_03B")


def ground_low(x0, x1, y):
    return min(lw.ground(x0, y), lw.ground(x1, y), lw.ground((x0 + x1) / 2.0, y))


def rubble(c, x, y, spread, n, shadow=True):
    rnd = c.rnd
    for _ in range(n):
        rx, ry = x + rnd.uniform(-spread, spread), y + rnd.uniform(-spread * 0.6, spread * 0.6)
        c.inst(rnd.choice(RUBBLE), rx, ry, lw.ground(rx, ry) + 25, s=rnd.uniform(0.45, 0.9), shadow=shadow)


def pillar(c, x, y, state="whole", scale=1.0, base=True, ivy=0.0, light=False):
    """A pillar on its base: whole, broken (snapped), stump or fallen (stump plus the shaft beside it)."""
    rnd = c.rnd
    z = lw.ground(x, y) - 8
    if base:
        c.inst(BASE, x, y, z, yaw=rnd.choice([0, 90]), s=scale)
        z += 199 * scale - 6
    if state == "whole":
        c.inst(PILLAR, x, y, z, yaw=rnd.uniform(-4, 4), s=scale)
        top = z + 486 * scale
        if ivy and rnd.random() < ivy:
            c.inst(rnd.choice(IVY), x + rnd.uniform(-30, 30), y + 70 * scale, top - 10, yaw=rnd.uniform(-20, 20), s=rnd.uniform(1.0, 1.5), shadow=False)
        if light:
            c.light(x, y + 90, top - 120 * scale, (0.45, 0.7, 1.0), 22, 2400, 0.5, "Rune_Light")
        return top
    keep = {"broken": rnd.uniform(0.5, 0.7), "stump": rnd.uniform(0.2, 0.32), "fallen": rnd.uniform(0.25, 0.4)}[state]
    c.inst(PILLAR, x, y, z, yaw=rnd.uniform(-6, 6), sx=scale, sy=scale, sz=keep * scale)
    top = z + 486 * keep * scale
    rubble(c, x, y, 200, rnd.randint(1, 3))
    if state == "fallen":
        lie = rnd.choice([-1, 1])
        fx = x + lie * 140 * scale
        c.inst(PILLAR, fx, y - 40, lw.ground(fx, y) + 75 * scale, yaw=90 if lie > 0 else -90, roll=90, sx=scale, sy=scale, sz=(1.0 - keep - 0.05) * scale)
    return top


def wall_run(c, x0, y, heights, ivy=0.0, trim=False, shadow=True):
    """A wall of 256 wide stones, heights[i] rows high each, from x0 along x, its face towards the camera. A row only sits on a full row."""
    rnd = c.rnd
    for i, rows in enumerate(heights):
        cx = x0 + i * 256
        base = ground_low(cx, cx + 256, y) - 15
        for r in range(rows):
            c.inst(WALL, cx, y, base + r * 256, yaw=0, shadow=shadow)
        top = base + rows * 256
        if rows == 0:
            rubble(c, cx + 128, y + 100, 150, 3)
            continue
        if trim and rows == max(heights) and i > 0 and heights[i - 1] == rows:
            c.inst(TRIM, cx - 5, y - 30, top + 69, yaw=0, shadow=shadow)
        if ivy and rnd.random() < ivy:
            c.inst(rnd.choice(IVY), cx + rnd.uniform(40, 216), y + 70, top - 5, yaw=rnd.uniform(-15, 15), s=rnd.uniform(1.1, 1.8) * (1 + 0.3 * (rows - 1)), shadow=False)
        elif rows < max(heights) and rnd.random() < 0.5:
            rubble(c, cx + 128, y + 110, 130, 1)


def side_run(c, x, y0, heights):
    """A wall running away from the camera (along -y), so the ruins are also seen from the side."""
    for i, rows in enumerate(heights):
        py = y0 - i * 256
        base = ground_low(x, x + 64, py - 128) - 15
        for r in range(rows):
            c.inst(WALL, x, py, base + r * 256, yaw=-90)
        if rows == 0:
            rubble(c, x + 30, py - 128, 150, 3)


def arch(c, x, y, scale=1.3, flank=True):
    g = ground_low(x, x + 521 * scale, y)
    c.inst(WALL_OPEN, x, y + 100 * scale, g - 8, yaw=0, s=scale)
    if flank:
        wall_run(c, x - 2 * 256, y, [1, 2])
        wall_run(c, x + 521 * scale + 60, y, [2, 1])


def gate(c, x, y=-520, scale=1.15):
    """Two tall pillars, one with a lintel stub, the other broken, fallen pieces between."""
    rnd = c.rnd
    t1 = pillar(c, x, y, "whole", scale, ivy=0.8)
    pillar(c, x + 520 * scale, y, rnd.choice(["broken", "fallen"]), scale)
    c.inst(LINTEL, x + 90 * scale, y, t1 - 20, yaw=90, sx=0.3 * scale, sy=0.3 * scale, sz=0.3 * scale)
    rubble(c, x + 280 * scale, y - 60, 280, 5)


def colonnade(c, x0, n, y=-470, spacing=620, scale=1.0, ivy=0.5):
    """Pillars along x at y: mostly whole, some broken, a stump or a fallen one now and then."""
    rnd = c.rnd
    for i in range(n):
        st = rnd.choice(["whole", "whole", "whole", "broken", "broken", "stump", "fallen"])
        pillar(c, x0 + i * spacing + rnd.uniform(-40, 40), y + rnd.uniform(-20, 20), st, scale, ivy=ivy)


def paving(c, x0, x1, y_back, depth=2, rows_missing=0.2):
    """Flagstone paving behind the path (flat, set into the ground), with missing slabs."""
    rnd = c.rnd
    x = x0
    while x < x1:
        for r in range(depth):
            y = y_back + r * 512
            if rnd.random() < rows_missing:
                continue
            g = ground_low(x, x + 768, y + 256)
            c.inst(FLOOR, x + rnd.uniform(-8, 8), y, g + 4 + rnd.uniform(-6, 2), yaw=rnd.uniform(-1.5, 1.5), pitch=rnd.uniform(-1, 1), roll=rnd.uniform(-1, 1), shadow=False)
        x += 768


def road(c, cx, y_from, y_to):
    """A broken flagstone road along y (towards the back), centred on x = cx."""
    rnd = c.rnd
    y = y_from
    while y > y_to:
        y0 = y - 768
        if rnd.random() > 0.22:
            c.inst(FLOOR, cx + 256 + rnd.uniform(-14, 14), y0, lw.ground(cx, y0) + 6 + rnd.uniform(-8, 3), yaw=90 + rnd.uniform(-2.5, 2.5),
                   pitch=rnd.uniform(-1.5, 1.5), roll=rnd.uniform(-1.5, 1.5), sy=rnd.uniform(0.97, 1.0), shadow=False)
        else:
            rubble(c, cx, y0 + 384, 220, 3)
        y -= 768


def road_pillars(c, cx, y_from=-900, step=980, n=6, glow=6):
    rnd = c.rnd
    runes = 0
    for i in range(n):
        py = y_from - step * i
        for side in (-1, 1):
            x = cx + side * (470 + rnd.uniform(-50, 50))
            yy = py + rnd.uniform(-90, 90)
            sc = 1.15 + 0.07 * i
            state = rnd.choice(["whole", "broken", "broken", "stump", "fallen"] if i > 0 else ["broken", "stump"])
            pillar(c, x, yy, state, sc, light=(runes < glow))
            if state == "whole":
                runes += 1


def obelisk(c, x, y, light=True):
    rnd = c.rnd
    sz = rnd.uniform(0.3, 0.4)
    c.inst(UPRIGHT, x, y, lw.ground(x, y) - 40 + 619 * sz, yaw=rnd.uniform(-20, 20), roll=rnd.uniform(-4, 4), sx=0.16, sy=0.16, sz=sz)
    if light:
        c.light(x, y + 160, lw.ground(x, y) + 1825 * sz * 0.45, (0.45, 0.7, 1.0), 28, 2800, 0.5, "Rune_Light")


def standing_ring(c, cx, cy, r=5600.0, ss=2.6, ey=0.34, a0=165, a1=380, da=18):
    """The ring of trilithons around a giant: uprights with lintels on the back half, a few fallen, rune blocks inside."""
    rnd = c.rnd

    def upright(x, y, hs, yaw):
        sz = 0.26 * ss * hs
        c.inst(UPRIGHT, x, y, lw.ground(x, y) - 60 + 619 * sz, yaw=yaw, sx=0.15 * ss, sy=0.15 * ss, sz=sz)
        return lw.ground(x, y) - 60 + 1825 * sz

    for i, angle in enumerate(range(a0, a1, da)):
        a = math.radians(angle)
        x, y = cx + r * math.cos(a), cy + r * math.sin(a) * ey
        tangent = math.degrees(a) + 90
        tx, ty = math.cos(a + math.pi / 2) * 140 * ss, math.sin(a + math.pi / 2) * 140 * ss * ey
        if i % 4 == 3:
            c.inst(UPRIGHT, x, y, lw.ground(x, y) + 60 * ss, yaw=tangent, pitch=84, sx=0.15 * ss, sy=0.15 * ss, sz=0.26 * ss)
            continue
        ta = upright(x - tx, y - ty, rnd.uniform(0.9, 1.15), tangent + rnd.uniform(-6, 6))
        tb = upright(x + tx, y + ty, rnd.uniform(0.9, 1.15), tangent + rnd.uniform(-6, 6))
        if i % 2 == 0:
            c.inst(LINTEL, x, y, min(ta, tb) + 112 * 0.3 * ss - 10 * ss, yaw=tangent + 90, s=0.3 * ss)
    for angle in range(0, 360, 30):
        a = math.radians(angle + 15)
        x, y = cx + r * 0.6 * math.cos(a), cy + r * 0.6 * math.sin(a) * ey
        if y > cy + 600:
            continue
        c.inst(BLOCK, x, y, lw.ground(x, y) - 40 * ss, yaw=rnd.uniform(0, 360), roll=rnd.uniform(-8, 8), s=rnd.uniform(0.35, 0.55) * ss)
        c.light(x, y, lw.ground(x, y) + 160 * ss, (0.45, 0.7, 1.0), 25, 2600, 0.5, "Rune_Light")
