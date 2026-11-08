"""Kit of the long world: the six zones of the first level, each ten times longer, far beyond it (x 150000..820000).

The first level is untouched. This world lives in the same map, along the same walking line (y 0), because the hero is
constrained to that plane. Everything of it sits under the outliner folder "LW".

  zones        six, each in chunks of about 11-12 thousand cm (the length of a whole zone of the first level)
  profile      the walkable ground height along x, generated per zone (hills, ledges; the cave digs down)
  Chunk        collects what one chunk contains: instanced meshes (one actor per mesh and chunk), single actors, lights, enemies
  terrain      ground slabs where the ground leaves z 0, the walkable floor strip, the plain behind it, backdrop mountains, end walls

Pieces that repeat (trees, grass, rocks, ruin blocks) go in as instanced meshes ("ism" job op): one actor holds thousands of them.
"""
import math, random, bisect
import ue
import levelgen as lg

CUBE = lg.CUBE
GROUND = lg.GROUND
CAVE_GROUND = lg.CAVE_GROUND
DEC = ["Decoration"]

X_START = 150000.0
CHECKPOINT_CLASS = "/Script/AmnesiaBeta.AmnesiaCheckpoint"
PICKUP_CLASS = "/Script/AmnesiaBeta.AmnesiaPickup"

# key, name, length, chunks, first checkpoint index, enemy levels (first, last), moods
ZONE_DEFS = [
    ("Z1", "Erdő", 112000, 10, 100, (2, 3), ["Summer Golden", "Summer Blue"]),
    ("Z2", "Falu", 120000, 10, 120, (3, 5), ["Autumn Golden", "Summer Blue"]),
    ("Z3", "Havas erdő", 112000, 10, 140, (4, 7), ["Winter Golden", "Winter Blue"]),
    ("Z4", "Romok", 100000, 9, 160, (6, 9), ["Ruins Mist", "Ruins Rain"]),
    ("Z5", "Barlang", 120000, 10, 180, (8, 10), ["Cave"]),
    ("Z6", "Vihar", 106000, 9, 200, (10, 12), ["Storm"]),
]


class Zone:
    def __init__(self, key, name, length, n, cp, levels, moods, x0):
        self.key, self.name, self.length, self.n, self.cp, self.levels, self.moods = key, name, float(length), n, cp, levels, moods
        self.x0 = x0
        self.x1 = x0 + length

    def chunk_span(self, k):
        w = self.length / self.n
        return self.x0 + k * w, self.x0 + (k + 1) * w


ZONES = []
_x = X_START
for _d in ZONE_DEFS:
    ZONES.append(Zone(*_d, x0=_x))
    _x += _d[2]
X_END = _x
ZONE = {z.key: z for z in ZONES}


def zone_at(x):
    for z in ZONES:
        if z.x0 <= x < z.x1:
            return z
    return ZONES[0] if x < X_START else ZONES[-1]


# ---------------------------------------------------------------- the ground profile
class Profile:
    """Walkable ground height along x as a polyline. Straight lines between points, flat beyond both ends."""

    def __init__(self):
        self.pts = []
        self._xs = []

    def extend(self, pts):
        for x, z in pts:
            if self.pts and x <= self.pts[-1][0]:
                x = self.pts[-1][0] + 1.0
            self.pts.append((x, z))
        self._xs = [p[0] for p in self.pts]

    def h(self, x):
        if not self.pts:
            return 0.0
        if x <= self._xs[0]:
            return self.pts[0][1]
        i = bisect.bisect_right(self._xs, x) - 1
        if i >= len(self.pts) - 1:
            return self.pts[-1][1]
        (xa, za), (xb, zb) = self.pts[i], self.pts[i + 1]
        return za + (zb - za) * (x - xa) / (xb - xa)

    def slope_deg(self, x):
        return abs(math.degrees(math.atan2(self.h(x + 40) - self.h(x - 40), 80.0)))


PROFILE = Profile()
CAVE = {}      # filled by the cave builder: ceiling line points, rooms


def h(x):
    return PROFILE.h(x)


def flat_at(x, margin=250):
    return PROFILE.slope_deg(x) < 0.5 and PROFILE.slope_deg(x - margin) < 0.5 and PROFILE.slope_deg(x + margin) < 0.5


def ground(x, y):
    """Surface height at (x, y): the hills cover y -1400..140, everything else is z 0 (the plain)."""
    if -1400 <= y <= 140:
        return h(x)
    return 0.0


def hill_profile(rnd, x0, x1, hmin, hmax, flat=(1200, 4200), ang=(11, 19), plat=(400, 1800), margin=1500, ridge=0.2, steps=0.2):
    """Rolling ground for an open zone: flat runs, hills with plateaus, stepped hills and ridges, always back at 0 between them."""
    pts = [(x0, 0.0), (x0 + margin, 0.0)]
    x = x0 + margin
    while True:
        x += rnd.uniform(*flat)
        H = rnd.uniform(hmin, hmax)
        a = rnd.uniform(*ang)
        run = H / math.tan(math.radians(a))
        need = run * 2 + plat[1] * 2 + margin
        if x + need > x1:
            break
        pts.append((x, 0.0))
        r = rnd.random()
        if r < ridge:
            pts.append((x + run, H))
            x += run * 2
            pts.append((x, 0.0))
        elif r < ridge + steps:
            H1 = H * rnd.uniform(0.4, 0.6)
            run1 = H1 / math.tan(math.radians(a))
            run2 = (H - H1) / math.tan(math.radians(a))
            pts.append((x + run1, H1))
            x += run1 + rnd.uniform(*plat)
            pts.append((x, H1))
            x += run2
            pts.append((x, H))
            x += rnd.uniform(*plat)
            pts.append((x, H))
            x += run
            pts.append((x, 0.0))
        else:
            pts.append((x + run, H))
            x += run + rnd.uniform(*plat)
            pts.append((x, H))
            x += run * rnd.uniform(0.9, 1.1)
            pts.append((x, 0.0))
    pts.append((x1 - margin, 0.0))
    pts.append((x1, 0.0))
    return pts


def cave_profile(rnd, x0, x1):
    """The cave floor: it dips from the surface, runs through rooms at different depths and climbs back out. The ceiling line follows
    the floor plus a gap that changes room to room (tunnels low, caverns high). Returns the floor points and fills CAVE['gaps']."""
    pts = [(x0, 0.0), (x0 + 1500, 0.0)]
    gaps = [(x0, 1500.0), (x0 + 1500, 1500.0)]
    x, z = x0 + 1500, 0.0
    # the way down
    depth = -rnd.uniform(900, 1500)
    x += abs(depth - z) / math.tan(math.radians(24))
    z = depth
    pts.append((x, z))
    gaps.append((x, 1050.0))
    end_run = 8000.0
    while x < x1 - 1500 - end_run:
        x += rnd.uniform(1600, 5200)                       # a room
        gap = rnd.choice([900, 1050, 1050, 1300, 1700, 2100])
        pts.append((x, z))
        gaps.append((x, float(gap)))
        target = rnd.uniform(-1900, -250)
        if abs(target - z) < 250:
            continue
        a = rnd.uniform(9, 22)
        x += abs(target - z) / math.tan(math.radians(a))
        z = target
        pts.append((x, z))
        gaps.append((x, float(gap)))
    # the way out
    x += rnd.uniform(1200, 2400)
    pts.append((x, z))
    gaps.append((x, 1050.0))
    x += abs(z) / math.tan(math.radians(22))
    pts.append((x, 0.0))
    gaps.append((x, 1300.0))
    pts.append((x1 - 400, 0.0))
    gaps.append((x1 - 400, 1500.0))
    pts.append((x1, 0.0))
    gaps.append((x1, 1500.0))
    CAVE["gaps"] = gaps
    return pts


def build_profile():
    """The ground of the whole long world, deterministic: the same seeds give the same hills every time."""
    global PROFILE
    PROFILE = Profile()
    plan = {
        "Z1": dict(hmin=110, hmax=300, flat=(1800, 5200), plat=(500, 1600)),
        "Z2": dict(hmin=120, hmax=420, flat=(1500, 4500), plat=(500, 1800)),
        "Z3": dict(hmin=150, hmax=520, flat=(1000, 3500), plat=(400, 1800), steps=0.3),
        "Z4": dict(hmin=130, hmax=300, flat=(1500, 4000), plat=(1200, 3000), steps=0.35, ridge=0.1),
        "Z6": dict(hmin=150, hmax=420, flat=(1200, 3800), plat=(400, 1600), ridge=0.3),
    }
    for z in ZONES:
        rnd = random.Random("profile-" + z.key)
        if z.key == "Z5":
            PROFILE.extend(cave_profile(rnd, z.x0, z.x1))
        else:
            PROFILE.extend(hill_profile(rnd, z.x0, z.x1, **plan[z.key]))
    return PROFILE


build_profile()


def cave_ceiling(x):
    """Bottom of the cave ceiling at x (the cave only)."""
    gaps = CAVE.get("gaps") or []
    if not gaps:
        return h(x) + 1050.0
    xs = [g[0] for g in gaps]
    if x <= xs[0]:
        return h(x) + gaps[0][1]
    i = bisect.bisect_right(xs, x) - 1
    if i >= len(gaps) - 1:
        return h(x) + gaps[-1][1]
    (xa, ga), (xb, gb) = gaps[i], gaps[i + 1]
    return h(x) + ga + (gb - ga) * (x - xa) / (xb - xa)


# ---------------------------------------------------------------- a chunk of a zone
def r1(v):
    return round(float(v), 1)


class Chunk:
    """Everything of one chunk. Repeating meshes are collected per mesh and sent as one instanced actor each."""

    def __init__(self, zone, k):
        self.zone, self.k = zone, k
        self.x0, self.x1 = zone.chunk_span(k)
        self.rnd = random.Random("%s-%d-long" % (zone.key, k))
        self.folder = "LW/%s/C%02d" % (zone.key, k)
        self.batches = {}
        self.ops = []
        self.counts = 0

    # the ground under a point; the cave overrides this through lw.ground
    def ground(self, x, y):
        return ground(x, y)

    # thin or soft things the guard leaves alone: trunks, plants, ground cover, the rock faces that stand in front on purpose
    SOFT = ("Spruce_0", "Small_Spruce", "SM_Tree_", "Ceiba_Tree", "Dry_Tree", "Broken_Tree", "Landscape", "SM_River", "SM_Grass", "SM_Flowers", "Plant",
            "Bush", "Ivy", "Fern", "Monstera", "Heliconia", "DirtStony", "SM_RockWall", "StalagmiteWall", "SM_Cube", "Mushroom", "SM_Corn", "Cabbage",
            "Pumpkin", "TopTrim", "SM_Flower")

    def guard_y(self, mesh, y, sy_scale, sx_scale, sz_scale):
        """Decorations have no collision in the game: a tall piece whose footprint reaches the hero's line (y 0) swallows the hero.
        Such a piece is moved sideways until its footprint clears the line."""
        name = mesh.split("/")[-1]
        if any(t in name for t in self.SOFT):
            return y
        import lwb
        b = lwb.b(mesh)
        if (b[5] - b[2]) * sz_scale < 70:
            return y
        reach = max(abs(b[0]), abs(b[1]), abs(b[3]), abs(b[4])) * max(sx_scale, sy_scale) * 1.05
        clear = 90.0                                       # the hero's half width and a margin
        if abs(y) < reach + clear:
            return -(reach + clear) if y <= 0 else (reach + clear)
        return y

    def inst(self, mesh, x, y, z, yaw=None, s=1.0, sx=None, sy=None, sz=None, pitch=0.0, roll=0.0, shadow=True, cull=None, material=None, guard=True):
        if yaw is None:
            yaw = self.rnd.uniform(0, 360)
        if guard:
            y = self.guard_y(mesh, y, sy or s, sx or s, sz or s)
        key = (mesh, bool(shadow), tuple(cull) if cull else None, material)
        self.batches.setdefault(key, []).append([r1(x), r1(y), r1(z), r1(pitch), r1(yaw), r1(roll),
                                                   round(sx or s, 3), round(sy or s, 3), round(sz or s, 3)])
        self.counts += 1

    def single(self, mesh, x, y, z, label, yaw=0.0, s=1.0, sx=None, sy=None, sz=None, pitch=0.0, roll=0.0, collision=None, hidden=False, material=None, shadow=None, tags=DEC):
        self.ops.append(ue.sm(mesh, x, y, z, yaw=yaw, s=s, sx=sx, sy=sy, sz=sz, pitch=pitch, roll=roll, label=label, folder=self.folder, tags=tags,
                              collision=collision, hidden=hidden, material=material, shadow=shadow))

    def actor(self, cls, x, y, z, label, props=None, rot=(0, 0, 0)):
        op = {"op": "spawn_actor", "class": cls, "loc": [r1(x), r1(y), r1(z)], "rot": list(rot), "label": label, "folder": self.folder}
        if props:
            op["props"] = props
        self.ops.append(op)

    def light(self, x, y, z, color, intensity, radius, flicker=0.0, label="Glow"):
        lg.light(self.ops, x, y, z, color, intensity, radius, flicker, label, folder=self.folder + "/Lights")

    def flush(self):
        """The instanced actors of the batches, then the single ops. Big batches are split so one job stays small."""
        out = [{"op": "delete", "folder": self.folder}]
        n = 0
        for (mesh, shadow, cull, material), items in self.batches.items():
            short = mesh.split("/")[-1].split(".")[0].replace("SM_", "")[:28]
            for part in range(0, len(items), 6000):
                op = {"op": "ism", "mesh": mesh, "label": "%s%02d_%s_%d" % (self.zone.key, self.k, short, part // 6000), "folder": self.folder,
                      "tags": DEC, "shadow": shadow, "instances": items[part:part + 6000]}
                if cull:
                    op["cull"] = list(cull)
                if material:
                    op["material"] = material
                out.append(op)
                n += 1
        out += self.ops
        return out

    def send(self, timeout=900):
        ops = self.flush()
        res = ue.job(ops, timeout=timeout, chunk=400)
        return res


def run_chunk(zone_key, k, builder, timeout=900):
    z = ZONE[zone_key]
    c = Chunk(z, k)
    builder(c)
    res = c.send(timeout)
    print("%s chunk %d  x %.0f..%.0f  instances %d  actors %d  errors %s" % (zone_key, k, c.x0, c.x1, c.counts, res["spawned"] - c.counts, res["errors"]))
    return c


# ---------------------------------------------------------------- terrain
def terrain_ops(zones=None):
    """Ground slabs where the ground leaves z 0, the walkable floor strip, the plain behind it, end walls. Open zones only; the cave has its own."""
    ops = [{"op": "delete", "folder": "LW/Terrain"}]
    F = "LW/Terrain"
    pts = PROFILE.pts
    for (x0, z0), (x1, z1) in zip(pts, pts[1:]):
        z = zone_at((x0 + x1) / 2)
        if z.key == "Z5":
            continue
        if abs(z0) < 1 and abs(z1) < 1:
            continue
        ops.append(lg.slab(x0, z0, x1, z1, 700, -1400, 140, "LW_Slab", F, GROUND))
    return ops


def floor_ops(zone, river_gaps=None):
    """The walkable strip (collision) and the plain behind and in front of it, per zone. river_gaps: [(x0, x1)] stretches where the plain
    is built by the river instead (visual boxes there)."""
    F = "LW/Terrain"
    ops = []
    # the first and the last zone reach a little beyond the world's ends, for the cliffs and trees that close it
    x0 = zone.x0 - END_PAD if zone.key == ZONES[0].key else zone.x0
    x1 = zone.x1 + END_PAD if zone.key == ZONES[-1].key else zone.x1
    ops.append(lg.box(x0, x1, -1400, 260, -1000, 0, "LW_Strip_" + zone.key, F, GROUND))
    # the lowered ground in front of the path, under the big blurry foreground plants
    ops.append(lg.box(x0, x1, 260, 820, -1250, -250, "LW_FG_Ground_" + zone.key, F, GROUND, collision="NoCollision"))
    return ops


END_PAD = 4600.0


def end_ops():
    """Cliffs, boulders and trees closing the world at both ends, on the extra floor beyond the end walls."""
    import lwf
    c = Chunk(ZONES[0], 0)
    c.folder = "LW/Ends"
    rnd = c.rnd
    for x_edge, d in ((X_START - 100, -1), (X_END + 100, 1)):
        # the plain behind the path
        xa, xb = (x_edge - END_PAD, x_edge) if d < 0 else (x_edge, x_edge + END_PAD)
        c.inst(CUBE, xa, -9000, -1000, yaw=0, sx=(xb - xa) / 100.0, sy=76.0, sz=10.0, shadow=False, material=GROUND, guard=False)
        for layer in range(3):
            for i in range(6):
                s = rnd.uniform(1.15, 1.6) - layer * 0.1
                half = 513 * s
                x = x_edge + d * (half * 0.8 + 150 + i * 760 + rnd.uniform(-60, 60))
                c.inst(lg.ROCK_WALL, x, -(368 * s) - 80 - layer * 120, layer * 300 - 60, yaw=rnd.uniform(-8, 8), s=s, shadow=False, guard=False)
        for i in range(8):
            s = rnd.uniform(1.1, 1.9)
            c.inst(lg.MOSS_ROCK, x_edge + d * (500 * s + rnd.uniform(100, 3200)), -400 - rnd.uniform(0, 300), -50, s=s, guard=False)
        for i in range(10):
            c.inst(rnd.choice(lg.TREES), x_edge + d * rnd.uniform(400, 4200), -900 - rnd.uniform(0, 900), -30, s=rnd.uniform(0.8, 1.25))
    return c.flush()


def backdrop_ops(rnd):
    """Mountains and hills far behind, one instanced actor per mesh over the whole world."""
    b = "/Game/Stylized_Spruce_Forest/Meshes/Background/SM_Background_Landscape_0%d.SM_Background_Landscape_0%d"
    m = [b % (i, i) for i in (1, 2, 3)]
    far, near = [], []
    x = X_START - 8000
    i = 0
    while x < X_END + 8000:
        far.append([x, -14000, -300, 0, 0 if i % 3 else 180, 0, 1, 1, 1])
        near.append([x - 5000, -10500, -200, 0, 180 if i % 2 else 0, 0, 1, 1, 1])
        x += 10000
        i += 1
    ops = [{"op": "delete", "folder": "LW/Backdrop"}]
    far1 = [f for j, f in enumerate(far) if j % 2 == 0]
    far3 = [f for j, f in enumerate(far) if j % 2 == 1]
    for name, mesh, items in (("Far1", m[0], far1), ("Far3", m[2], far3), ("Near2", m[1], near)):
        ops.append({"op": "ism", "mesh": mesh, "label": "LW_Backdrop_" + name, "folder": "LW/Backdrop", "tags": DEC, "shadow": False, "instances": items})
    return ops


def wall_ops():
    """Invisible walls at both ends of the long world."""
    F = "LW/Terrain"
    ops = []
    for x in (X_START - 200, X_END + 100):
        ops.append(lg.box(x, x + 100, -1500, 1500, -3000, 8000, "LW_EndWall", F, GROUND, collision="BlockAll", hidden=True))
    return ops


# ---------------------------------------------------------------- shared pieces for the zone builders
def pick(rnd, seq):
    return seq[rnd.randrange(len(seq))]


def level_at(zone, x):
    a, b = zone.levels
    t = (x - zone.x0) / zone.length
    return int(round(a + (b - a) * t))


ENEMY_BP = {
    "Oak": "/Game/Amnesia/Blueprints/BP_AmnesiaEnemy.BP_AmnesiaEnemy_C",
    "Ash": "/Game/Amnesia/Blueprints/BP_Ent_Ash.BP_Ent_Ash_C",
    "Birch": "/Game/Amnesia/Blueprints/BP_Ent_Birch.BP_Ent_Birch_C",
    "Spruce": "/Game/Amnesia/Blueprints/BP_Ent_Spruce.BP_Ent_Spruce_C",
    "Skeleton": "/Game/Amnesia/Blueprints/BP_Skeleton_Guard.BP_Skeleton_Guard_C",
    "Golem": "/Game/Amnesia/Blueprints/BP_Armored_Golem.BP_Armored_Golem_C",
}


FIRE_IGNITE = "(Sounds=(%s),Volume=0.800,PitchMin=0.950,PitchMax=1.050,Range=4000.0)" % ",".join(
    "/Script/Engine.SoundWave'/Game/Audio/World/SW_Fire_Ignite_%02d.SW_Fire_Ignite_%02d'" % (i, i) for i in (1, 2))


def checkpoint(c, name=None, dx=500.0):
    """The rest place at the start of the chunk. Its index counts up through the zone, the first one is the teleport target."""
    x = c.x0 + dx
    idx = c.zone.cp + c.k
    c.actor(CHECKPOINT_CLASS, x, 0, h(x), "Checkpoint_L%03d" % idx,
            {"CheckpointIndex": idx, "DisplayName": name or c.zone.name, "LightSounds": FIRE_IGNITE})


def enemies(c, kinds, count, margin=1400.0, gap=900.0):
    """Enemies on flat ground, spread over the chunk, their level rising along the zone. kinds: list of ENEMY_BP keys (repeats weight)."""
    rnd = c.rnd
    xs = []
    tries = 0
    while len(xs) < count and tries < 600:
        tries += 1
        x = rnd.uniform(c.x0 + margin, c.x1 - 300)
        if any(abs(x - o) < gap for o in xs) or not flat_at(x, 240):
            continue
        xs.append(x)
    xs.sort()
    for i, x in enumerate(xs):
        kind = pick(rnd, kinds)
        lvl = max(1, level_at(c.zone, x) + rnd.choice([0, 0, 1, -1]))
        c.actor(ENEMY_BP[kind], x, 0, h(x) + 320, "Ent_L%s%02d_%02d_%s_L%d" % (c.zone.key, c.k, i, kind, lvl),
                {"EnemyLevel": lvl}, rot=(0, 180 if rnd.random() < 0.5 else 0, 0))
    return xs


def orbs(c, count=2):
    rnd = c.rnd
    for i in range(count):
        x = rnd.uniform(c.x0 + 1500, c.x1 - 500)
        c.actor(PICKUP_CLASS, x, 0, h(x) + 90, "HealOrb_L%s%02d_%d" % (c.zone.key, c.k, i))
