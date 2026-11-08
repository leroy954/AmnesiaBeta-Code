"""Zone 3 of the long world: the snowy spruce forest, 112000 cm. The densest forest, rock fields, hunters' cabins and camps,
fallen trunks. The winter mood turns grass and trees white by itself."""
import math
import lw, lwf, lwb
from lwf import vg, GRASS, FLOWERS, PLANTS, TREES, SMALL_SPRUCE, BUSHES, ROCKS, SMALL_ROCKS, MOSS_ROCK, DRY_BUSH, LOG, STUMP
import levelgen as lg

Z = lw.ZONE["Z3"]
NAMES = ["Havas szegély", "Vadászkunyhó", "Jeges patak", "Farkasles", "Fagyott fenyves", "Hóvihar menedék", "Sziklamező", "Régi tábor", "Zúzmarás tisztás", "Csendes erdő"]


def cabin(c, x, y=-750):
    rnd = c.rnd
    mesh = vg("SM_HouseB")
    s = rnd.uniform(1.3, 1.7)
    g = min(lw.ground(x - 300, y), lw.ground(x + 300, y)) - 8
    c.single(mesh, x, y, g, "Cabin", s=s)
    c.light(x, y + 260, g + 280, (1.0, 0.6, 0.3), 20, 1000, 0.2, "Cabin_Light")
    c.single(vg("SM_Bonfire"), x + 520, -330, lw.ground(x + 520, -330), "CabinFire", s=2.4)
    c.light(x + 520, -330, lw.ground(x + 520, -330) + 100, (1.0, 0.55, 0.2), 22, 1100, 0.35, "Cabin_Fire")
    for i in range(4):
        c.inst(vg("InventoryProps/SM_Logs"), x - 380 + i * 90, -330 - rnd.uniform(0, 80), g, yaw=rnd.uniform(0, 360), s=1.6)
    return x


def cairn(c, x, y=-420):
    """A stack of stones as a trail marker."""
    rnd = c.rnd
    g = lw.ground(x, y)
    z = g
    for i in range(4):
        sc = 1.6 - i * 0.28
        c.inst(rnd.choice(SMALL_ROCKS), x + rnd.uniform(-10, 10), y + rnd.uniform(-10, 10), z - 5, s=sc * 1.6)
        z += 45 * sc
    c.light(x, y, g + 120, (0.6, 0.8, 1.0), 6, 500, 0.0, "Cairn_Glow")


def boulder_field(c, x, y=-800):
    rnd = c.rnd
    for _ in range(18):
        rx, ry = x + rnd.uniform(-1300, 1300), y + rnd.uniform(-600, 350)
        rock = rnd.choice(ROCKS + [MOSS_ROCK, lg.STONES])
        c.inst(rock, rx, ry, lw.ground(rx, ry) - 40, s=rnd.uniform(0.7, 1.3) if rock == MOSS_ROCK else rnd.uniform(0.9, 3.0))


def build(c):
    rnd = c.rnd
    dens = rnd.uniform(1.3, 1.7)
    clearing = rnd.uniform(c.x0 + 3500, c.x1 - 3500) if rnd.random() < 0.4 else None

    def tree_skip(x, y, row):
        return clearing is not None and row in ("R0", "R1") and abs(x - clearing) < 1200

    lwf.tree_rows(c, TREES, dens=dens, small=SMALL_SPRUCE, skip=tree_skip)
    lwf.scatter(c, BUSHES + [SMALL_SPRUCE], 170, (-2800, -300), (3.0, 4.8), z_off=-20)
    lwf.scatter(c, DRY_BUSH, 260, (-1800, -250), (2.0, 3.2), shadow=False)
    lwf.scatter(c, [LOG, STUMP], 520, (-700, -200), (1.3, 2.0))
    lwf.scatter(c, ROCKS + [MOSS_ROCK, lg.STONES], 380, (-1200, -250), (0.9, 2.0), z_off=-30)
    lwf.foreground(c, spacing=380, trunk_every=1500)
    lwf.path_grass(c)
    lwf.back_grass(c, (-330, -520, -720, -930, -1150), 120, extra=[(0.12, DRY_BUSH)])
    lwf.plain(c)
    lwf.hill_faces(c)

    options = ["cabin", "camp", "boulders", "giant", "cairn", "shrine", "boulders", "cairn"]
    rnd.shuffle(options)
    xs = sorted(rnd.uniform(c.x0 + 2500, c.x1 - 1500) for _ in range(3))
    for kind, x in zip(options[:3], xs):
        if kind in ("cabin", "camp", "shrine") and not lw.flat_at(x, 700):
            continue
        {"cabin": lambda: cabin(c, x), "camp": lambda: lwf.camp(c, x), "boulders": lambda: boulder_field(c, x),
         "giant": lambda: lwf.fallen_giant(c, x), "cairn": lambda: cairn(c, x), "shrine": lambda: lwf.wayshrine(c, x)}[kind]()

    lw.checkpoint(c, NAMES[c.k % len(NAMES)])
    lw.enemies(c, ["Ash", "Birch", "Spruce", "Skeleton", "Skeleton", "Golem"], 6)
    lw.orbs(c, 2)
