"""Zone 2 of the long world: the autumn village country, 120000 cm. Chunks alternate between hamlets, farmland with a barn and mill,
and a market place. Buildings stand behind the path, trees keep out of their footprints."""
import math
import lw, lwf, lwb
from lwf import vg, GRASS, FLOWERS, PLANTS, TREES, SMALL_SPRUCE, BUSHES, ROCKS, SMALL_ROCKS, MUSH
import levelgen as lg

Z = lw.ZONE["Z2"]
NAMES = ["Falu széle", "Kovácsműhely", "Régi kút", "Malom", "Piactér", "Pásztortanya", "Fogadó", "Gyümölcsös", "Hídfő", "Szélmalom"]

HOUSES = [vg("SM_HouseC"), vg("SM_HouseA"), vg("SM_HouseB"), vg("SM_Barn")]
VILLAGE_TREES = TREES + [vg("SM_Tree_B"), vg("SM_Tree_C")] * 2


class Site:
    """Footprints already taken in a chunk, so trees, plants and props stay out of them."""

    def __init__(self):
        self.rects = []

    def add(self, x, y, w, d, pad=120):
        self.rects.append((x - w / 2 - pad, x + w / 2 + pad, y - d / 2 - pad, y + d / 2 + pad))

    def hit(self, x, y):
        return any(a < x < b and c < y < d for a, b, c, d in self.rects)


def building(c, site, mesh, x, y, s, label="Building"):
    mn = lwb.b(mesh)
    w, d = (mn[3] - mn[0]) * s, (mn[4] - mn[1]) * s
    # stand on the lowest ground under the footprint, a little sunk
    g = min(lw.ground(x - w / 2, y), lw.ground(x + w / 2, y), lw.ground(x, y - d / 2), lw.ground(x, y + d / 2), lw.ground(x, y)) - 8
    c.single(mesh, x, y, g, label, s=s)
    site.add(x + (mn[3] + mn[0]) * s / 2, y + (mn[4] + mn[1]) * s / 2, w, d)
    return w


def street_props(c, site, x0, x1):
    """Crates, barrels, haystacks, fences and lamps along the path on the flats."""
    rnd = c.rnd
    x = x0
    while x < x1:
        if lw.flat_at(x, 150):
            r = rnd.random()
            if r < 0.45:
                for i in range(rnd.randint(1, 3)):
                    c.inst(rnd.choice([vg("SM_Crate"), vg("SM_Barrel")]), x + i * 75, rnd.uniform(-330, -210), lw.ground(x, -250), s=rnd.uniform(1.7, 2.2))
            elif r < 0.65:
                c.inst(vg("InventoryProps/SM_HaystackA"), x, rnd.uniform(-480, -330), lw.ground(x, -400), s=rnd.uniform(0.7, 1.0))
            elif r < 0.9:
                # a short run of low fence, the houses behind it stay in view
                for i in range(rnd.randint(1, 2)):
                    c.inst(vg("SM_FenceBig_4m"), x + i * 330, -225, lw.ground(x + i * 330, -225), yaw=0, s=0.82)
                x += 500
        x += rnd.uniform(420, 900)


def lamps(c, xs):
    for x in xs:
        g = lw.ground(x, -260)
        c.single(vg("SM_LightColumn_FireBowl"), x, -260, g, "Lamp", s=1.7)
        c.light(x, -240, g + 520, (1.0, 0.6, 0.25), 14, 1000, 0.25, "Lamp_Light")


def hamlet(c, site, xc, count):
    """Buildings in two rows behind the path around xc, a well, a bonfire with light and waymarks."""
    rnd = c.rnd
    x = xc - 2600
    placed = 0
    while placed < count and x < xc + 2600:
        near = rnd.random() < 0.5
        mesh = rnd.choice(HOUSES)
        s = rnd.uniform(1.4, 2.1)
        y = rnd.uniform(-900, -600) if near else rnd.uniform(-1900, -1650)
        if near and mesh == HOUSES[3]:
            mesh = HOUSES[2]
        w = building(c, site, mesh, x, y, s)
        x += w * 0.6 + rnd.uniform(250, 700)
        placed += 1
    # the well and the bonfire in the open place between the rows
    wx = xc + rnd.uniform(-300, 300)
    c.single(vg("SM_Well"), wx, -260, lw.ground(wx, -260), "Well", s=1.7)
    site.add(wx, -260, 380, 340)
    bx = xc + rnd.uniform(500, 1100) * rnd.choice((-1, 1))
    c.single(vg("SM_Bonfire"), bx, -130 - 90, lw.ground(bx, -220), "Bonfire", s=2.4)
    c.light(bx, -240, lw.ground(bx, -220) + 100, (1.0, 0.55, 0.2), 22, 1100, 0.35, "Bonfire_Light")
    c.single(vg("SM_WaymarkA"), xc - 2800, -190, lw.ground(xc - 2800, -190), "Waymark", s=2.0)
    c.single(vg("SM_WaymarkB"), xc + 2800, -190, lw.ground(xc + 2800, -190), "Waymark", yaw=180, s=2.0)
    lamps(c, [xc - 1500, xc + 1500, xc + 200])
    c.inst(vg("InventoryProps/SM_Cart"), xc + rnd.uniform(-900, 900), -300, lw.ground(xc, -300), yaw=rnd.uniform(-30, 30), s=2.0)


def farm(c, site, xc):
    """Fields with crops, a barn, haystacks and a mill far behind."""
    rnd = c.rnd
    building(c, site, vg("SM_Barn"), xc - 900, -900, 1.8)
    building(c, site, vg("SM_HouseB"), xc + 1100, -750, 1.6)
    building(c, site, vg("SM_Mill"), xc + 300, -2400, 2.3, label="Mill")
    for k in range(8):
        fx = xc - 2200 + k * 560
        if site.hit(fx, -900):
            continue
        fy = rnd.choice([-1000, -1500])
        if not lw.flat_at(fx, 300):
            continue
        c.inst(vg("SM_GardenBed_6x4m"), fx, fy, lw.ground(fx, fy), yaw=90, s=1.6)
        site.add(fx, fy, 640, 960, 40)
        for _ in range(12):
            px, py = fx + rnd.uniform(-300, 300), fy + rnd.uniform(-450, 450)
            c.inst(rnd.choice([vg("SM_Corn"), vg("SM_Cabbage"), vg("SM_Pumpkin")]), px, py, lw.ground(px, py) + 10, s=rnd.uniform(1.4, 2.0), shadow=False)
    for k in range(3):
        hx = xc + rnd.uniform(-2000, 2000)
        if not site.hit(hx, -500):
            c.inst(vg("InventoryProps/SM_HaystackB"), hx, rnd.uniform(-700, -450), lw.ground(hx, -500), s=rnd.uniform(0.6, 0.9))


def market(c, site, xc):
    """A market place beside the path: stalls with sunshades, crates, barrels, benches and a bonfire."""
    rnd = c.rnd
    for k in range(5):
        sx = xc - 1600 + k * 800 + rnd.uniform(-100, 100)
        if not lw.flat_at(sx, 300):
            continue
        g = lw.ground(sx, -420)
        c.single(rnd.choice([vg("SM_ShopA"), vg("SM_ShopB"), vg("SM_ShopC")]), sx, -420, g, "Stall", s=2.4)
        for _ in range(3):
            c.inst(rnd.choice([vg("SM_Crate"), vg("SM_Barrel"), vg("SM_BoxA")]), sx + rnd.uniform(-200, 200), rnd.uniform(-360, -300), g, s=rnd.uniform(1.7, 2.2))
        site.add(sx, -420, 330, 380)
    c.single(vg("SM_Bonfire"), xc, -300, lw.ground(xc, -300), "Bonfire", s=2.4)
    c.light(xc, -300, lw.ground(xc, -300) + 100, (1.0, 0.55, 0.2), 22, 1100, 0.35, "Bonfire_Light")
    building(c, site, vg("SM_HouseC"), xc - 1800, -1500, 1.9)
    building(c, site, vg("SM_HouseA"), xc + 1800, -1550, 1.9)
    lamps(c, [xc - 2200, xc, xc + 2200])


def build(c):
    rnd = c.rnd
    site = Site()
    kind = ["hamlet", "farm", "hamlet", "market", "hamlet", "farm", "hamlet", "market", "farm", "hamlet"][c.k]
    xc = rnd.uniform(c.x0 + 4000, c.x1 - 4000)
    if kind == "hamlet":
        hamlet(c, site, xc, rnd.randint(5, 8))
    elif kind == "farm":
        farm(c, site, xc)
    else:
        market(c, site, xc)
    street_props(c, site, c.x0 + 800, c.x1 - 400)

    def tree_skip(x, y, row):
        return site.hit(x, y)

    dens = rnd.uniform(0.9, 1.25)
    lwf.tree_rows(c, VILLAGE_TREES, dens=dens, small=SMALL_SPRUCE, skip=tree_skip)
    big_bush = [vg("SM_BushBig_A"), vg("SM_BushBig_B"), vg("SM_BushBig_C")]
    lwf.scatter(c, BUSHES, 220, (-2800, -350), (3.0, 4.6), z_off=-20, skip=site.hit)
    lwf.scatter(c, big_bush, 600, (-2800, -1300), (1.6, 2.6), z_off=-20, skip=site.hit)
    lwf.scatter(c, PLANTS + [FLOWERS, vg("SM_Flower_A"), vg("SM_Flower_B")], 150, (-1500, -200), (2.0, 3.4), skip=site.hit, shadow=False)
    lwf.foreground(c, spacing=420, trunk_every=2600)
    lwf.path_grass(c)
    lwf.back_grass(c, (-330, -520, -720, -930, -1150), 115, extra=[(0.05, [FLOWERS]), (0.06, PLANTS)], skip=site.hit)
    lwf.plain(c)
    lwf.hill_faces(c)
    lw.checkpoint(c, NAMES[c.k % len(NAMES)])
    lw.enemies(c, ["Oak", "Ash", "Birch", "Spruce", "Skeleton", "Skeleton"], 6)
    lw.orbs(c, 2)
