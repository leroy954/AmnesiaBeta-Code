"""Zone 1 of the long world: the summer forest, 112000 cm. Per chunk: tree rows, undergrowth, grass, a river in some stretches,
two or three landmarks, a rest place, enemies and heal orbs."""
import random
import lw, lwf
from lwf import (GRASS, FLOWERS, PLANTS, TREES, SMALL_SPRUCE, BUSHES, ROCKS, SMALL_ROCKS, MUSH)
import levelgen as lg

Z = lw.ZONE["Z1"]
RIVERS = [lwf.River(Z.x0 + 14000, Z.x0 + 36500, 11), lwf.River(Z.x0 + 55000, Z.x0 + 71000, 12), lwf.River(Z.x0 + 86000, Z.x0 + 99000, 13)]

NAMES = ["Erdei pihenő", "Öreg tölgy", "Gombás tisztás", "Patakparti tábor", "Mohos kövek", "Vadászles", "Szarvasút", "Páfrányos szurdok", "Tisztás", "Fenyves szél"]


def build(c):
    rnd = c.rnd
    rivers = [r for r in RIVERS if r.spans(c.x0, c.x1)]

    def in_river(x, y):
        return any(r.covers(x, y) for r in rivers)

    glade = rnd.uniform(c.x0 + 3500, c.x1 - 3500) if rnd.random() < 0.55 else None

    def tree_skip(x, y, row):
        if in_river(x, y):
            return True
        return glade is not None and row in ("R0", "R1") and abs(x - glade) < 1100

    dens = rnd.uniform(0.85, 1.2)
    far_rows = [r for r in lg.ROWS if r[0] in ("R1", "R2", "R3", "R4")]
    lwf.tree_rows(c, TREES, rows=far_rows, dens=0.85 * dens, skip=tree_skip)
    lwf.tree_rows(c, TREES, rows=[lg.ROWS[0]], dens=dens, small=SMALL_SPRUCE, skip=tree_skip)

    lwf.scatter(c, BUSHES, 240, (-2800, -350), (3.0, 4.6), z_off=-20, skip=in_river)
    lwf.scatter(c, PLANTS + [FLOWERS], 160, (-1500, -200), (2.0, 3.4), skip=in_river, shadow=False)
    lwf.scatter(c, MUSH, 700, (-700, -150), (1.6, 2.6), shadow=False)
    lwf.scatter(c, ROCKS + SMALL_ROCKS, 800, (-900, -250), (0.9, 1.8), z_off=-30, skip=in_river)
    lwf.foreground(c, spacing=520, trunk_every=3000)

    lwf.path_grass(c)
    lwf.back_grass(c, (-330, -500, -680, -860, -1000), 105, extra=[(0.1, [FLOWERS]), (0.07, PLANTS)])
    lwf.back_grass(c, (-1180, -1380, -1580, -1780, -1980, -2180), 150, extra=[(0.08, [FLOWERS]), (0.06, PLANTS)], skip=in_river)

    for r in rivers:
        r.build(c)
        # grass along both banks, at fixed distances from the edge of the water
        for side in (-1, 1):
            for d in (60, 230, 460, 800):
                x = max(c.x0, r.xa)
                while x < min(c.x1, r.xb):
                    yc, w, _ = r.sample(x)
                    y = yc + side * (w / 2 + d + rnd.uniform(-50, 50))
                    if y < -120 and not r.covers(x, y, 30):
                        c.inst(FLOWERS if rnd.random() < 0.08 else rnd.choice(GRASS), x, y, -4, s=rnd.uniform(1.7, 2.5) * (1 + abs(y) * 0.0002), shadow=False)
                    x += 125 * rnd.uniform(0.7, 1.3)
    lwf.plain(c, river=rivers[0] if rivers else None)
    lwf.hill_faces(c)

    # two or three landmarks on their own stretch of the chunk
    options = ["camp", "stone", "giant", "garden", "shrine", "meadow", "camp", "garden"]
    rnd.shuffle(options)
    xs = sorted(rnd.uniform(c.x0 + 2500, c.x1 - 1500) for _ in range(3))
    for kind, x in zip(options[:3], xs):
        if in_river(x, -300) and kind in ("camp", "shrine"):
            continue
        if kind in ("camp", "shrine") and not lw.flat_at(x, 700):
            continue
        {"camp": lambda: lwf.camp(c, x), "stone": lambda: lwf.stone_circle(c, x), "giant": lambda: lwf.fallen_giant(c, x),
         "garden": lambda: lwf.rock_garden(c, x), "shrine": lambda: lwf.wayshrine(c, x), "meadow": lambda: lwf.meadow(c, x)}[kind]()

    lw.checkpoint(c, NAMES[c.k % len(NAMES)])
    lw.enemies(c, ["Oak", "Ash", "Birch", "Spruce"], 5 + (c.k % 2))
    lw.orbs(c, 2)
