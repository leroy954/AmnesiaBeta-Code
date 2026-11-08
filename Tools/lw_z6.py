"""Zone 6 of the long world: the storm plain, 106000 cm. Dead trees and dry grass, ruined walls, arches and obelisks, boulders, and three
colossi: each stands inside a stone ring far behind the path, with a broken flagstone road and pillars leading to it."""
import math
import lw, lwf, lwr
from lwf import GRASS, FLOWERS, PLANTS, TREES, SMALL_SPRUCE, BUSHES, ROCKS, SMALL_ROCKS, MOSS_ROCK, DRY_BUSH, DRY_TREE, LOG, STUMP
import levelgen as lg

Z = lw.ZONE["Z6"]
NAMES = ["Viharos mező", "Kőút", "Az óriások kapuja", "Romos fal", "Villámos domb", "Árnyékliget", "Kőkör", "Szélfútta puszta", "Az utolsó tábor"]
GIANT_CHUNKS = (1, 4, 7)
GIANT_CLASS = "/Script/AmnesiaBeta.AmnesiaGiant"
RING_Y = -9000.0


def giant_site(c):
    """The giant, its ring, the road and the pillars. Returns the x of the road, which other pieces keep clear of."""
    cx = (c.x0 + c.x1) / 2.0
    c.actor(GIANT_CLASS, cx, RING_Y, -160, "GiantGolem_%s%d" % (c.zone.key, c.k), {"WatchRange": 30000.0, "ShakeStrength": 3.0}, rot=(0, 90, 0))
    # the actor takes its scale from the op's scale field: set it with a second op
    c.ops[-1]["scale"] = [35.0, 35.0, 35.0]
    c.light(cx + 1500, RING_Y + 4000, 3500, (0.55, 0.7, 1.0), 900, 12000, 0.15, "Giant_Light")
    c.light(cx - 1200, RING_Y + 3000, 6500, (0.5, 0.65, 1.0), 600, 9000, 0.2, "Giant_Light")
    lwr.standing_ring(c, cx, RING_Y)
    lwr.road(c, cx, -480.0, -7000.0)
    lwr.road_pillars(c, cx)
    for k, (ox, oy) in enumerate(((-1500, -2300), (1700, -3000), (-2300, -4000), (2600, -4500), (-900, -5000), (1000, -5300))):
        lwr.obelisk(c, cx + ox, oy)
    return cx


def build(c):
    rnd = c.rnd
    road_x = giant_site(c) if c.k in GIANT_CHUNKS else None

    def clear_of_road(x, y=0):
        return road_x is None or abs(x - road_x) > 650 or y > -400

    # an open view: only low dead trees close to the path, so the storm sky shows above them
    low_rows = [("R1", -1100, 300, (0.35, 0.55), 330), ("R2", -2100, 400, (0.4, 0.6), 380)]
    lwf.tree_rows(c, [SMALL_SPRUCE] + DRY_TREE, rows=low_rows, skip=lambda x, y, row: not clear_of_road(x, y))
    lwf.scatter(c, DRY_TREE, 520, (-1600, -300), (0.9, 1.5), z_off=-20, skip=lambda x, y: not clear_of_road(x, y), shadow=False)
    lwf.scatter(c, DRY_BUSH + [lg.BUSH], 210, (-2500, -300), (2.6, 4.2), z_off=-20, skip=lambda x, y: not clear_of_road(x, y), shadow=False)
    lwf.scatter(c, [LOG, STUMP], 600, (-800, -200), (1.3, 2.2))
    lwf.scatter(c, ROCKS + [MOSS_ROCK], 380, (-1400, -250), (1.0, 2.4), z_off=-40, skip=lambda x, y: not clear_of_road(x, y))
    lwf.foreground(c, spacing=420, trunk_every=1400)
    lwf.path_grass(c)
    lwf.back_grass(c, (-300, -480, -680, -900, -1150, -1400, -1750, -2200, -2700), 105, extra=[(0.2, DRY_BUSH)], skip=lambda x, y: not clear_of_road(x, y), scale=(2.0, 2.9))
    lwf.plain(c)
    c.inst(lw.CUBE, c.x0, -12500, -1000, yaw=0, sx=(c.x1 - c.x0) / 100.0, sy=35.0, sz=10.0, shadow=False, material=lw.GROUND)    # more plain behind, for the colossi
    lwf.hill_faces(c)

    # ruined walls, arches, side walls on both sides of the plain, never across the road
    for _ in range(6):
        x = rnd.uniform(c.x0 + 300, c.x1 - 1800)
        y = rnd.uniform(-5000, -800)
        if clear_of_road(x, y) and clear_of_road(x + 1500, y):
            lwr.wall_run(c, x, y, [rnd.choice([0, 1, 1, 2]) for _ in range(rnd.randint(4, 7))], ivy=0.0, shadow=False)
    for _ in range(2):
        x, y = rnd.uniform(c.x0 + 800, c.x1 - 1500), rnd.uniform(-4500, -2400)
        if clear_of_road(x) and clear_of_road(x + 700):
            lwr.arch(c, x, y, rnd.uniform(1.0, 1.4))
    for _ in range(3):
        x, y = rnd.uniform(c.x0 + 300, c.x1 - 300), rnd.uniform(-4500, -1300)
        if clear_of_road(x, y):
            lwr.side_run(c, x, y, [rnd.choice([0, 1, 1, 2]) for _ in range(rnd.randint(3, 5))])
    for _ in range(4):
        x, y = rnd.uniform(c.x0 + 300, c.x1 - 300), rnd.uniform(-5300, -1500)
        if clear_of_road(x):
            lwr.obelisk(c, x, y, light=rnd.random() < 0.5)

    # dead groves, boulders
    for _ in range(70):
        x, y = rnd.uniform(c.x0, c.x1), rnd.uniform(-5300, -600)
        if clear_of_road(x, y):
            c.inst(rnd.choice(DRY_TREE), x, y, lw.ground(x, y) - 20, s=rnd.uniform(1.1, 2.3) * (1 + abs(y) * 0.00012), shadow=False)
    for _ in range(40):
        x, y = rnd.uniform(c.x0, c.x1), rnd.uniform(-5200, -700)
        if clear_of_road(x, y):
            if rnd.random() < 0.35:
                c.inst(MOSS_ROCK, x, y, lw.ground(x, y) - 60, s=rnd.uniform(0.8, 1.7))
            else:
                c.inst(rnd.choice(ROCKS), x, y, lw.ground(x, y) - 25, s=rnd.uniform(1.4, 3.0))

    lw.checkpoint(c, NAMES[c.k % len(NAMES)])
    lw.enemies(c, ["Skeleton", "Skeleton", "Golem", "Spruce", "Ash"], 6)
    lw.orbs(c, 2)
