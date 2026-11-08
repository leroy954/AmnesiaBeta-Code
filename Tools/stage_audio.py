"""Pulls the chosen sounds out of the downloaded zips and prepares them for Unreal.
Usage: python -I stage_audio.py <downloads dir> <output dir>

Everything becomes 16 bit PCM wav with a clean name:
  one   - one shot: leading silence trimmed, capped in length, peak normalized
  slice - a recording of several steps cut into single steps
  loop  - an ambience capped in length and crossfaded end into start, so it loops without a seam
Thunder is synthesized, since none of the packs has any."""
import sys, os, io, zipfile, wave, struct, random, math, array

DL, OUT = sys.argv[1], sys.argv[2]
# optional: only stage this output folder, like Companion
ONLY = sys.argv[3] if len(sys.argv) > 3 else None

ZIPS = {
    "dragon": "dragon-20261021T062131Z-1-001.zip",
    "amb": "AMBIENCE-20261021T062127Z-1-001.zip",
    "battle": "Battle-20261021T062129Z-1-001.zip",
    "steps": "Footsteps-20261021T062132Z-1-001.zip",
    "hits": "Hits-20261021T062133Z-1-001.zip",
    "loc": "Locations-20261021T062139Z-1-001.zip",
    "magic": "MAGIC WEPONES-20261021T062141Z-1-001.zip",
    "mon": "monsters_creatures_sfx_1_wav-20261021T062142Z-1-001.zip",
    "rpg": "rpg_sound_pack.zip",
}

W = "MAGIC WEPONES/Weapons/0009_combat_"
M = "monsters_creatures_sfx_1_wav/"
R = "RPG Sound Pack/"

# (zip, member, folder, base name, mode, options)
PLAN = [
    # hero
    *[("magic", W + "sword_swing_%s.wav" % k, "Hero", "Hero_Swing", "one", {}) for k in "cde"],
    *[("rpg", R + "battle/%s.wav" % k, "Hero", "Hero_Swing", "one", {}) for k in ("swing", "swing2", "swing3")],
    *[("hits", "Hits/Hit_0%d.wav" % i, "Hero", "Hero_HitThud", "one", {"max": 0.6}) for i in (1, 2, 3, 4)],
    *[("battle", "Battle/%s.wav" % k, "Hero", "Hero_Hurt", "one", {"max": 0.7}) for k in ("Punch Thud 2", "Punch Thud 3", "Punch Thud 5", "Punch Thud 6 ", "Sharp Punch 9")],
    ("battle", "Battle/Soldier Head and Body Fall to Ground.wav", "Hero", "Hero_Death", "one", {"max": 3.0}),
    ("battle", "Battle/Body Fall 23.wav", "Hero", "Hero_Death", "one", {"max": 2.0}),
    ("steps", "Footsteps/Jump_Footsteps Dirt 3.wav", "Hero", "Hero_Jump", "one", {"peak": -8}),
    ("steps", "Footsteps/Landing_Jump_Footsteps Gravel 1.wav", "Hero", "Hero_Land", "one", {"peak": -6}),
    ("steps", "Footsteps/Landing_Jump_Footsteps Gravel 9.wav", "Hero", "Hero_Land", "one", {"peak": -6}),
    ("rpg", R + "battle/spell.wav", "Hero", "Hero_LevelUp", "one", {"max": 3.2}),
    ("rpg", R + "battle/magic1.wav", "Hero", "Hero_Heal", "one", {"peak": -6}),
    *[("rpg", R + "inventory/%s.wav" % k, "Hero", "Orb_Pickup", "one", {"peak": -6}) for k in ("bubble", "bubble2", "bubble3")],

    # footsteps, cut into single steps
    ("steps", "Footsteps/Footsteps Grass 2.wav", "Footsteps", "Step_Grass", "slice", {}),
    ("steps", "Footsteps/Footsteps Grass 1.wav", "Footsteps", "Step_Grass", "slice", {}),
    ("steps", "Footsteps/Footsteps Dirt 8.wav", "Footsteps", "Step_Dirt", "slice", {}),
    ("steps", "Footsteps/Footsteps Dirt 7.wav", "Footsteps", "Step_Dirt", "slice", {}),
    ("steps", "Footsteps/Footsteps Snow 3.wav", "Footsteps", "Step_Snow", "slice", {}),
    ("steps", "Footsteps/Footsteps Rocks 6.wav", "Footsteps", "Step_Stone", "slice", {}),
    ("steps", "Footsteps/Footsteps Gravel 4.wav", "Footsteps", "Step_Stone", "slice", {}),
    ("steps", "Footsteps/Footsteps Mud 4.wav", "Footsteps", "Step_Mud", "slice", {}),
    ("steps", "Footsteps/Footsteps Mud 5.wav", "Footsteps", "Step_Mud", "slice", {}),
    *[("steps", "Footsteps/Monster Footsteps %s.wav" % k, "Footsteps", "Step_Monster", "one", {"max": 0.9}) for k in ("1", "2 ", "3", "4", "5", "6")],

    # sword impacts on each kind of enemy
    *[("magic", W + "sword_hit_wood_%s.wav" % k, "Enemy", "Impact_Wood", "one", {}) for k in "cde"],
    *[("magic", W + "dagger_hit_wood_%s.wav" % k, "Enemy", "Impact_Wood", "one", {}) for k in "cde"],
    *[("magic", W + "staff_hit_%s.wav" % k, "Enemy", "Impact_Bone", "one", {}) for k in "cde"],
    *[("magic", W + "punch_hit_%s.wav" % k, "Enemy", "Impact_Bone", "one", {}) for k in "cde"],
    *[("magic", W + "sword_hit_metal_%s.wav" % k, "Enemy", "Impact_Metal", "one", {}) for k in "cde"],
    *[("hits", "Hits/Metal_Hit_0%d.wav" % i, "Enemy", "Impact_Metal", "one", {"max": 1.0}) for i in (5, 6, 8)],

    # ents
    *[("rpg", R + "NPC/giant/giant%d.wav" % i, "Enemy", "Ent_Grunt", "one", {}) for i in range(1, 6)],
    ("mon", M + "Earth Elemental/0014_monster_elemental_earth_vocal_a.wav", "Enemy", "Ent_Attack", "one", {}),
    ("mon", M + "Earth Elemental/0014_monster_elemental_earth_vocal_b.wav", "Enemy", "Ent_Attack", "one", {}),
    ("mon", M + "Troll/0014_monster_troll_vocal_b.wav", "Enemy", "Ent_Attack", "one", {}),
    ("mon", M + "Earth Elemental/0014_monster_elemental_earth_death.wav", "Enemy", "Ent_Death", "one", {}),
    ("mon", M + "Troll/0014_monster_troll_death.wav", "Enemy", "Ent_Death", "one", {}),
    ("battle", "Battle/Body Fall Heavy 5.wav", "Enemy", "BodyFall_Heavy", "one", {"max": 2.0}),
    ("battle", "Battle/Body Fall Heavy 7.wav", "Enemy", "BodyFall_Heavy", "one", {"max": 2.0}),

    # skeletons
    ("mon", M + "Ghoul/0014_monster_ghoul_vocal_a.wav", "Enemy", "Skeleton_Attack", "one", {}),
    ("mon", M + "Ghoul/0014_monster_ghoul_bite.wav", "Enemy", "Skeleton_Attack", "one", {}),
    ("mon", M + "Ghoul/0014_monster_ghoul_claw.wav", "Enemy", "Skeleton_Attack", "one", {}),
    ("mon", M + "Ghoul/Enemy_ Walker_Bug_Detect.wav", "Enemy", "Skeleton_Attack", "one", {}),
    *[("rpg", R + "NPC/shade/shade%d.wav" % i, "Enemy", "Skeleton_Hurt", "one", {"peak": -5}) for i in (1, 2, 3, 14, 15)],
    ("mon", M + "Ghoul/0014_monster_ghoul_death.wav", "Enemy", "Skeleton_Death", "one", {}),
    ("mon", M + "Ghost/0014_monster_ghost_death.wav", "Enemy", "Skeleton_Death", "one", {}),
    ("battle", "Battle/Skull Crush Body Falls.wav", "Enemy", "Skeleton_Fall", "one", {}),
    ("battle", "Battle/Impact Body Fall 2.wav", "Enemy", "Skeleton_Fall", "one", {}),

    # golems
    ("mon", M + "Giant/0014_monster_giant_attack_a.wav", "Enemy", "Golem_Attack", "one", {}),
    ("mon", M + "Giant/0014_monster_giant_attack_b.wav", "Enemy", "Golem_Attack", "one", {}),
    ("mon", M + "Giant/Enemy_Walker_Rock_Detect.wav", "Enemy", "Golem_Attack", "one", {}),
    ("mon", M + "Giant/Enemy_Walker_Rock_Grunts_01.wav", "Enemy", "Golem_Hurt", "one", {}),
    ("mon", M + "Giant/Enemy_Walker_Rock_Grunts_02.wav", "Enemy", "Golem_Hurt", "one", {}),
    ("mon", M + "Minotaur/0014_monster_minotaur_vocal_b.wav", "Enemy", "Golem_Hurt", "one", {}),
    ("mon", M + "Minotaur/0014_monster_minotaur_death.wav", "Enemy", "Golem_Death", "one", {}),
    ("mon", M + "Giant/Enemy_Walker_Rock_LandRock.wav", "Enemy", "Golem_Fall", "one", {}),

    # world and menu
    ("mon", M + "Demon/Disparo flama_Arma.wav", "World", "Fire_Ignite", "one", {}),
    ("mon", M + "Fire Elemental/0014_monster_elemental_fire_attack_a.wav", "World", "Fire_Ignite", "one", {}),
    ("rpg", R + "interface/interface1.wav", "UI", "UI_Click", "one", {"peak": -6}),
    ("rpg", R + "interface/interface3.wav", "UI", "UI_Hover", "one", {"peak": -12}),
    ("rpg", R + "battle/sword-unsheathe2.wav", "UI", "UI_SwordDraw", "one", {"peak": -4}),

    # the little dragon companion: short calls of a big dragon, pitched up in the game, and wing flaps
    *[("dragon", "dragon/Dragon_%02d.wav" % i, "Companion", "Dragon_Chirp", "one", {"max": 1.6, "peak": -6, "fade_out": 450}) for i in (9, 17, 18, 20, 23, 25, 27)],
    ("mon", M + "Earth Elemental/Alas pesadas_01.wav", "Companion", "Dragon_Flap", "one", {"peak": -8}),
    ("mon", M + "Earth Elemental/Alas pesadas_02.wav", "Companion", "Dragon_Flap", "one", {"peak": -8}),
    ("mon", M + "Fire Elemental/0014_monster_elemental_fire_attack_a.wav", "Companion", "Fireball_Launch", "one", {"peak": -5}),
    ("mon", M + "Fire Elemental/0014_monster_elemental_fire_attack_b.wav", "Companion", "Fireball_Launch", "one", {"peak": -5}),
    ("mon", M + "Demon/Disparo flama_Arma.wav", "Companion", "Fireball_Launch", "one", {"peak": -5, "max": 1.0, "fade_out": 250}),
    ("mon", M + "Fire Elemental/0014_monster_elemental_fire_death.wav", "Companion", "Fireball_Hit", "one", {"peak": -4, "max": 1.2, "fade_out": 350}),
    ("mon", M + "Demon/Enemy_WormBoss_Generic_Damage.wav", "Companion", "Fireball_Hit", "one", {"peak": -4}),

    # ambience loops
    ("amb", "AMBIENCE/Forest 2.wav", "Ambience", "Amb_Forest_Day", "loop", {}),
    ("amb", "AMBIENCE/Calm River.wav", "Ambience", "Amb_River", "loop", {}),
    ("loc", "Locations/Countryside Village Ambience.wav", "Ambience", "Amb_Village", "loop", {}),
    ("amb", "AMBIENCE/Forest 4b.wav", "Ambience", "Amb_Forest_Quiet", "loop", {}),
    ("amb", "AMBIENCE/Forest 3.wav", "Ambience", "Amb_Forest_Deep", "loop", {}),
    ("amb", "AMBIENCE/Crickets Night 3.wav", "Ambience", "Amb_Crickets", "loop", {}),
    ("amb", "AMBIENCE/Cave Ambience Drips.wav", "Ambience", "Amb_Cave_Drips", "loop", {}),
    ("amb", "AMBIENCE/Cave Ambience Wind.wav", "Ambience", "Amb_Wind", "loop", {}),
    ("amb", "AMBIENCE/Forest Rain 3.wav", "Ambience", "Amb_Rain_Forest", "loop", {}),
    ("amb", "AMBIENCE/Rain Downpour 1.wav", "Ambience", "Amb_Rain_Heavy", "loop", {}),
]


def read_wav(data):
    """Returns (rate, channels, array of 16 bit samples interleaved). Handles 16, 24 and 32 bit int and 32 bit float."""
    pos, fmt, raw = 12, None, None
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            tag, ch, rate = struct.unpack("<HHI", body[:8])
            bits = struct.unpack("<H", body[14:16])[0]
            if tag == 0xFFFE:
                tag = struct.unpack("<H", body[24:26])[0]
            fmt = (tag, ch, rate, bits)
        elif cid == b"data":
            raw = body
        pos += 8 + size + (size & 1)
    tag, ch, rate, bits = fmt
    out = array.array("h")
    if tag == 3 and bits == 32:
        f = array.array("f")
        f.frombytes(raw[:len(raw) // 4 * 4])
        out.extend(max(-32768, min(32767, int(v * 32767))) for v in f)
    elif bits == 16:
        out.frombytes(raw[:len(raw) // 2 * 2])
    elif bits == 24:
        n = len(raw) // 3
        b = bytearray(n * 2)
        b[0::2] = raw[1:n * 3:3]
        b[1::2] = raw[2:n * 3:3]
        out.frombytes(bytes(b))
    elif bits == 32:
        n = len(raw) // 4
        b = bytearray(n * 2)
        b[0::2] = raw[2:n * 4:4]
        b[1::2] = raw[3:n * 4:4]
        out.frombytes(bytes(b))
    elif bits == 8:
        out.extend((v - 128) << 8 for v in raw)
    else:
        raise ValueError("unsupported wav %s" % (fmt,))
    return rate, ch, out


def write_wav(path, rate, ch, samples):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "wb") as w:
        w.setnchannels(ch)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(samples.tobytes())


def frame_peaks(s, ch, rate, block_ms=10):
    """Peak level of each block of frames, as a list."""
    block = max(1, int(rate * block_ms / 1000)) * ch
    return [max(abs(v) for v in s[i:i + block]) if len(s[i:i + block]) else 0 for i in range(0, len(s), block)], block


def normalize(s, peak_db):
    top = max((abs(v) for v in s), default=0)
    if top == 0:
        return s
    gain = (32767 * 10 ** (peak_db / 20.0)) / top
    return array.array("h", (max(-32768, min(32767, int(v * gain))) for v in s))


def fade(s, ch, rate, fade_in_ms=2, fade_out_ms=30):
    s = array.array("h", s)
    frames = len(s) // ch
    fi, fo = int(rate * fade_in_ms / 1000), int(rate * fade_out_ms / 1000)
    for f in range(min(fi, frames)):
        for c in range(ch):
            s[f * ch + c] = int(s[f * ch + c] * f / fi)
    for f in range(min(fo, frames)):
        k = frames - 1 - f
        for c in range(ch):
            s[k * ch + c] = int(s[k * ch + c] * f / fo)
    return s


def one_shot(rate, ch, s, opts):
    peaks, block = frame_peaks(s, ch, rate)
    loud = max(peaks) if peaks else 0
    start = next((i for i, p in enumerate(peaks) if p > loud * 0.06), 0)
    # keep a few ms before the attack
    begin = max(0, start * block - int(rate * 0.004) * ch)
    s = s[begin:]
    cap = int(rate * opts.get("max", 2.5)) * ch
    s = s[:cap]
    # cut the silent tail
    peaks, block = frame_peaks(s, ch, rate)
    end = len(peaks)
    while end > 1 and peaks[end - 1] < loud * 0.01:
        end -= 1
    s = s[:end * block]
    return fade(normalize(s, opts.get("peak", -3)), ch, rate, 2, opts.get("fade_out", 30))


def slice_steps(rate, ch, s, limit=6):
    """Finds the separate steps in a footstep recording and returns each one as a short sound."""
    peaks, block = frame_peaks(s, ch, rate)
    loud = max(peaks)
    on, quiet, onsets = loud * 0.22, loud * 0.08, []
    armed = True
    for i, p in enumerate(peaks):
        if armed and p > on:
            # step back to where the step begins
            j = i
            while j > 0 and peaks[j - 1] > quiet and i - j < 6:
                j -= 1
            if not onsets or j - onsets[-1] > 18:
                onsets.append(j)
            armed = False
        elif p < quiet:
            armed = True
    steps = []
    for k, o in enumerate(onsets):
        end = onsets[k + 1] if k + 1 < len(onsets) else len(peaks)
        end = min(end, o + 32)  # at most 320 ms
        # stop once the step has died away for 50 ms, before the next scuff
        top = max(peaks[o:end])
        quiet_run = 0
        for b in range(o + 3, end):
            quiet_run = quiet_run + 1 if peaks[b] < top * 0.06 else 0
            if quiet_run >= 5:
                end = b
                break
        piece = s[o * block:end * block]
        if len(piece) > rate * 0.06 * ch:
            steps.append(fade(normalize(piece, -6), ch, rate, 1, 40))
    # keep the strongest takes, spread over the recording
    return steps[:limit]


def make_loop(rate, ch, s, max_seconds=75.0, cross_seconds=3.0):
    cross = int(rate * cross_seconds)
    frames = min(len(s) // ch, int(rate * max_seconds) + cross)
    s = s[:frames * ch]
    body = array.array("h", s[:(frames - cross) * ch])
    tail = s[(frames - cross) * ch:]
    # blend the tail into the start so the end flows into the beginning
    for f in range(cross):
        a = f / cross
        for c in range(ch):
            i = f * ch + c
            body[i] = int(body[i] * a + tail[i] * (1.0 - a))
    return body


def thunder(rate, seed, seconds=6.5):
    rnd = random.Random(seed)
    n = int(rate * seconds)
    out = array.array("h", bytes(n * 4))
    bumps = [(rnd.uniform(0.0, 0.25), 1.0, rnd.uniform(0.35, 0.6))] + [(rnd.uniform(0.3, 2.8), rnd.uniform(0.35, 0.8), rnd.uniform(0.5, 1.4)) for _ in range(5)]
    crack_t = bumps[0][0]
    for c in range(2):
        lo1 = lo2 = lo3 = hi = 0.0
        a1, a2, a3, ah = 1 - math.exp(-2 * math.pi * 60 / rate), 1 - math.exp(-2 * math.pi * 140 / rate), 1 - math.exp(-2 * math.pi * 320 / rate), 1 - math.exp(-2 * math.pi * 2500 / rate)
        r = random.Random(seed * 10 + c)
        buf = []
        for i in range(n):
            t = i / rate
            w = r.uniform(-1.0, 1.0)
            lo1 += a1 * (w - lo1)
            lo2 += a2 * (lo1 - lo2)
            lo3 += a3 * (w - lo3)
            hi += ah * (w - hi)
            env = 0.0
            for t0, amp, tau in bumps:
                if t >= t0:
                    env += amp * math.exp(-(t - t0) / tau) * min(1.0, (t - t0) / 0.06)
            env *= min(1.0, (seconds - t) / 1.5)
            crack = 0.0
            if t >= crack_t:
                crack = math.exp(-(t - crack_t) / 0.09) * min(1.0, (t - crack_t) / 0.004)
            buf.append(lo2 * 9.0 * env + lo3 * 1.6 * env * 0.5 + hi * crack * 0.9)
        top = max(abs(v) for v in buf) or 1.0
        for i, v in enumerate(buf):
            out[i * 2 + c] = int(v / top * 32767 * 0.7)
    return out


def main():
    zips = {k: zipfile.ZipFile(os.path.join(DL, v)) for k, v in ZIPS.items()}
    counters = {}
    report = []
    for key, member, folder, base, mode, opts in PLAN:
        if ONLY and folder != ONLY:
            continue
        try:
            data = zips[key].read(member)
        except KeyError:
            report.append("MISSING %s %s" % (key, member))
            continue
        rate, ch, s = read_wav(data)
        if mode == "one":
            pieces = [one_shot(rate, ch, s, opts)]
        elif mode == "slice":
            pieces = slice_steps(rate, ch, s)
        else:
            pieces = [make_loop(rate, ch, s)]
        for p in pieces:
            if mode == "loop":
                name = "SW_%s" % base
            else:
                counters[base] = counters.get(base, 0) + 1
                name = "SW_%s_%02d" % (base, counters[base])
            path = os.path.join(OUT, folder, name + ".wav")
            write_wav(path, rate, ch, p)
            report.append("%s/%s %.2fs %dch %dHz" % (folder, name, len(p) / ch / rate, ch, rate))
    for i in range(3 if not ONLY or ONLY == "World" else 0):
        p = thunder(48000, 7 + i)
        name = "SW_Thunder_%02d" % (i + 1)
        write_wav(os.path.join(OUT, "World", name + ".wav"), 48000, 2, p)
        report.append("World/%s %.2fs synthesized" % (name, len(p) / 2 / 48000))
    print("\n".join(report))
    print("files:", len(report))


main()
