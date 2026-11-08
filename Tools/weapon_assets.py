"""Builds the art and the sounds of the weapons and the new skill branches, and imports them:
- the shop pictures of the weapons, /Game/Amnesia/UI/Weapons/T_Weapon_<id>: the thumbnail of the model with the background taken away, turned to lean in the picture
- the 12 skill icons of the weapon branches, /Game/Amnesia/UI/Skills/T_Skill_<id>: icons of the rogue icon pack (/Game/RPGClassIcons) with the background taken away and a hue shift
- two sounds of the ice, /Game/Audio/World/SW_Ice_Freeze_01 and SW_Ice_Shatter_01, made from noise and sine waves (no pack)
Usage: python weapon_assets.py [--weapons] [--skills] [--sounds] [--only <weapon id>]   (no flag: all)"""
import base64
import io
import math
import os
import random
import struct
import sys
import time
import wave

from PIL import Image, ImageChops, ImageEnhance, ImageFilter

import ue

HERE = os.path.dirname(os.path.abspath(__file__))
STAGING = os.path.join(HERE, "staging", "weapon_assets")

PUREPOLY = "/Game/PurePoly/FreeLowPolyFantasyRPGWeapons/Meshes/%s.%s"
WEAPONS = {"chakram": "SM_PP_Theme_05_Chakram_001", "axe": "SM_PP_Theme_11_Axe_One_Handed_002", "wand": "SM_PP_Theme_08_Wand_001"}

# skill id: (the base icon of the rogue pack, the hue its main colour is turned to in degrees or None). The icons are violet: ice blue for the chakram, red
# for the axe, and the wand keeps the violet
ICE, RED = 200.0, 4.0
ICONS = {
    "chakram_power": ("Garrote_Slash", ICE), "chakram_bounce": ("Distraction_Coin", ICE), "chakram_multi": ("Combo_Chain", ICE), "chakram_whirl": ("Evasive_Roll", ICE),
    "axe_power": ("Toxic_Blade", RED), "axe_cleave": ("Crossed_Daggers", RED), "axe_bleed": ("Hemorrhage", RED), "axe_quake": ("Vaulting_Kick", RED),
    "wand_power": ("Umbral_Decoy", None), "wand_haste": ("Shadowstep", None), "wand_chain": ("Spider_Poison", None), "wand_blast": ("Sudden_Appearance", None),
}


def capture(path):
    raw = ue.call(ue.E, "CaptureAssetImage", assetPath=path)
    return Image.open(io.BytesIO(base64.b64decode(raw["data"]))).convert("RGBA")


def shifted(img, degrees):
    if degrees == 0:
        return img
    alpha = img.getchannel("A")
    h, s, v = img.convert("RGB").convert("HSV").split()
    step = round(degrees * 255.0 / 360.0)
    h = h.point(lambda value: (value + step) % 256)
    out = Image.merge("HSV", (h, s, v)).convert("RGB")
    out.putalpha(alpha)
    return out


def dominant_hue(img):
    """The hue (degrees) most of the colour of the icon has, and how much colour it has at all."""
    h, sat, val = img.convert("RGB").convert("HSV").split()
    hist = [0.0] * 256
    for hp, sp, vp, ap in zip(h.tobytes(), sat.tobytes(), val.tobytes(), img.getchannel("A").tobytes()):
        if ap > 128 and sp > 60:
            hist[hp] += sp * vp
    smooth = [sum(hist[(i + d) % 256] for d in range(-8, 9)) for i in range(256)]
    return smooth.index(max(smooth)) * 360.0 / 256.0, sum(hist)


def retinted(img, target):
    """The icon turned so its main colour gets the hue `target`; an icon with hardly any colour (a grey blade) stays as it is."""
    if target is None:
        return img
    hue, weight = dominant_hue(img)
    return img if weight < 8.0e6 else shifted(img, target - hue)


def without_background(img, color, near=12, full=60):
    """Alpha from how far a pixel is from the flat background color: nothing near it, everything far from it."""
    px = img.load()
    out = img.copy()
    po = out.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, _ = px[x, y]
            d = max(abs(r - color[0]), abs(g - color[1]), abs(b - color[2]))
            a = 0 if d <= near else min(255, int(255 * (d - near) / float(full - near)))
            po[x, y] = (r, g, b, a)
    return out


def import_textures(files, dest):
    # a picture that is already there (an earlier run) goes first, so the import does not ask what to do with it
    for file in files:
        asset = dest + "/" + os.path.splitext(os.path.basename(file))[0]

        if ue.call(ue.AS, "exists", path=asset):
            ue.call(ue.AS, "delete", path=asset)

    result = ue.job([{"op": "import", "dest": dest, "files": files}], timeout=600)
    print("imported", len(result["imported"]), "errors", result["errors"])
    props = [{"op": "set_object_props", "object": path, "props": {
        "CompressionSettings": "TC_EditorIcon", "LODGroup": "TEXTUREGROUP_UI", "MipGenSettings": "TMGS_NoMipmaps"}} for path in result["imported"]]
    props.append({"op": "save"})
    print("settings", ue.job(props, timeout=600)["modified"])


# ---------------------------------------------------------------- the pictures of the weapons, rendered by the editor
KEY_PATH = "/Game/Gen/IconKey/M_Key"
KEY_GREEN = 0.7
LIGHT = 120.0
STAGE = (54000.0, 0.0, 20000.0)
STAGE_FOLDER = "Temp/IconRender"
# weapon: (height of the model in cm, how far the middle of the model is above its pivot, turn of the model around its long axis, tilt of the picture)
WEAPON_VIEWS = {"chakram": (48.0, 0.0, 0.0, 0.0), "axe": (120.0, 14.0, 0.0, -42.0), "wand": (99.0, 11.5, 0.0, -42.0)}


def make_key_material():
    """A flat unlit green material for the wall behind the weapon (a copy of the weather material, like the cat probe)."""
    tools = "editor_toolset.toolsets.material.MaterialTools"
    mat = {"refPath": KEY_PATH + ".M_Key"}
    if ue.call(ue.AS, "exists", path=KEY_PATH):
        ue.call(ue.AS, "delete", path=KEY_PATH)
    ue.call(ue.AS, "duplicate", path="/Game/Amnesia/Materials/M_Weather", new_path=KEY_PATH)
    ue.props(mat, BlendMode="BLEND_Opaque")
    color = [e for e in ue.call(tools, "get_expressions", material_or_function=mat) if "VectorParameter" in e["refPath"]][0]
    ue.props(color, DefaultValue={"R": 0.0, "G": KEY_GREEN, "B": 0.0, "A": 1.0})
    ue.call(tools, "recompile", material_or_function=mat)
    ue.call(ue.AS, "save_assets", asset_paths=[KEY_PATH])


def render_weapon(weapon, yaw=None, light=LIGHT):
    """The weapon stands upright high above the level in front of a flat green wall, lit by a key and a fill light; the picture of the viewport camera."""
    height, middle, turn, _ = WEAPON_VIEWS[weapon]
    x, y, z = STAGE
    mesh = PUREPOLY % (WEAPONS[weapon], WEAPONS[weapon])
    ops = [{"op": "delete", "folder": STAGE_FOLDER},
           ue.sm(mesh, x, y, z - middle, yaw=turn if yaw is None else yaw, label="IconWeapon", folder=STAGE_FOLDER),
           ue.sm("/Engine/BasicShapes/Plane.Plane", x + 250.0, y, z, pitch=90, s=12.0, label="IconWall", folder=STAGE_FOLDER,
                 material=KEY_PATH + ".M_Key", shadow=False)]
    # the camera looks along +X from the front: the key light is up on the left of it, the fill low on the right
    k = height / 120.0
    for name, (dx, dy, dz), color, power in (("Key", (-110.0, -120.0, 130.0), (1.0, 0.92, 0.8), light), ("Fill", (-60.0, 150.0, -10.0), (0.7, 0.8, 1.0), light * 0.35)):
        ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaGlowLight", "loc": [x + dx * k, y + dy * k, z + dz * k], "label": "Icon" + name,
                    "folder": STAGE_FOLDER, "props": {"LightColor": "(R=%.3f,G=%.3f,B=%.3f,A=1.0)" % color, "Intensity": power * k * k, "Radius": 900.0, "FlickerAmount": 0.0}})
    ue.job(ops)
    camera = {"location": {"x": x - height * 1.1, "y": y, "z": z}, "rotation": {"pitch": 0.0, "yaw": 0.0, "roll": 0.0}, "scale": {"x": 1.0, "y": 1.0, "z": 1.0}}
    time.sleep(2.0)
    raw = ue.call(ue.E, "CaptureViewport", captureTransform=camera,
                  annotations={"gridSpacing": 0, "gridExtent": 0, "gridHeight": 0, "maxLabelDistance": 0, "classFilter": None, "maxLabels": 0}, bShowUI=False)
    image = raw["image"] if "image" in raw else raw
    return Image.open(io.BytesIO(base64.b64decode(image["data"]))).convert("RGB")


def clear_stage():
    ue.job([{"op": "delete", "folder": STAGE_FOLDER}, {"op": "save"}])


def key_green(img):
    """Takes the green wall away. The wall around the weapon is darker toward the edges of the picture, so each row takes its wall colour from
    the pixels left and right of the weapon; the alpha of a pixel is how much less green than its wall it is, and the wall's share is taken out of
    the colour of the edge pixels so they do not keep a green rim."""
    corner = img.getpixel((4, 4))
    corner_excess = max(1, corner[1] - max(corner[0], corner[2]))
    r, g, b = img.split()
    excess = ImageChops.subtract(g, ImageChops.lighter(r, b))
    # the weapon is in the middle of the picture; the corners hold the viewport's axis widget
    region = (int(img.width * 0.2), int(img.height * 0.02), int(img.width * 0.8), int(img.height * 0.9))
    found = excess.crop(region).point(lambda v: 255 if v < corner_excess * 0.5 else 0).getbbox()
    if not found:
        raise RuntimeError("no weapon in the picture")
    box = (found[0] + region[0], found[1] + region[1], found[2] + region[0], found[3] + region[1])
    crop = img.crop((max(0, box[0] - 10), max(0, box[1] - 4), min(img.width, box[2] + 10), min(img.height, box[3] + 4)))
    out = Image.new("RGBA", crop.size)
    src, dst = crop.load(), out.load()

    def byte(v):
        return max(0, min(255, int(round(v))))

    for yy in range(crop.height):
        left, right = src[2, yy], src[crop.width - 3, yy]
        wall = tuple((left[i] + right[i]) / 2.0 for i in range(3))
        wall_excess = max(1.0, wall[1] - max(wall[0], wall[2]))

        for xx in range(crop.width):
            pr, pg, pb = src[xx, yy]
            alpha = 1.0 - max(0.0, min(1.0, (pg - max(pr, pb)) / wall_excess))

            if alpha < 0.02:
                dst[xx, yy] = (0, 0, 0, 0)
                continue

            share = 1.0 - alpha
            dst[xx, yy] = (byte((pr - share * wall[0]) / alpha), byte((pg - share * wall[1]) / alpha), byte((pb - share * wall[2]) / alpha), byte(alpha * 255))

    return out


def to_icon(img, tilt, size=256, fill=236.0):
    """The weapon leaning in the square, as large as the square lets it be."""
    img = img.rotate(tilt, resample=Image.BICUBIC, expand=True)
    img = img.crop(img.getchannel("A").getbbox())
    scale = fill / max(img.size)
    img = img.resize((max(1, int(round(img.width * scale))), max(1, int(round(img.height * scale)))), Image.LANCZOS)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(img, ((size - img.width) // 2, (size - img.height) // 2), img)

    # a soft light halo, so a dark weapon (the blade of the axe) does not vanish on the dark panel of the shop
    halo = Image.new("RGBA", canvas.size, (200, 220, 255, 0))
    halo.putalpha(canvas.getchannel("A").filter(ImageFilter.GaussianBlur(6)).point(lambda v: min(255, int(v * 0.9))))
    return Image.alpha_composite(halo, canvas)


def build_weapon_icons():
    os.makedirs(STAGING, exist_ok=True)
    make_key_material()
    files = []
    try:
        for weapon, view in WEAPON_VIEWS.items():
            icon = to_icon(key_green(render_weapon(weapon)), view[3])
            path = os.path.join(STAGING, "T_Weapon_%s.png" % weapon).replace("\\", "/")
            icon.save(path)
            files.append(path)
    finally:
        clear_stage()
        ue.call(ue.AS, "delete", path=KEY_PATH)
    print("weapon pictures:", len(files))
    import_textures(files, "/Game/Amnesia/UI/Weapons")


def rogue_icon_paths():
    root = os.path.join(HERE, "..", "Content", "RPGClassIcons")
    found = {}
    for dp, _, fn in os.walk(root):
        for f in sorted(fn):
            if f.endswith(".uasset") and f.startswith("ROG_"):
                base = f[:-7].split("_", 3)[3]
                if base not in found:
                    rel = os.path.relpath(os.path.join(dp, f[:-7]), os.path.join(HERE, "..", "Content")).replace(os.sep, "/")
                    found[base] = "/Game/%s.%s" % (rel, f[:-7])
    return found


BRANCH_GLOW = {"chakram": (158, 219, 255), "axe": (250, 102, 82), "wand": (199, 148, 255)}


def with_glow(img, branch, lift=1.55, saturation=1.25):
    """The dark art of the rogue icons gets brighter and a soft glow in the color of its branch behind it, so it does not vanish on the dark slot of the
    skill screen next to the vivid icons of the other branches."""
    alpha = img.getchannel("A")
    art = ImageEnhance.Color(ImageEnhance.Brightness(img.convert("RGB")).enhance(lift)).enhance(saturation)
    art.putalpha(alpha)

    size = img.width
    glow = Image.new("RGBA", img.size, BRANCH_GLOW[branch] + (0,))
    mask = Image.new("L", img.size, 0)
    pixels = mask.load()
    center = (size - 1) / 2.0
    radius = size * 0.52

    for y in range(size):
        for x in range(size):
            d = ((x - center) ** 2 + (y - center) ** 2) ** 0.5 / radius
            pixels[x, y] = int(max(0.0, 1.0 - d) ** 1.6 * 150)

    glow.putalpha(mask)
    return Image.alpha_composite(glow, art)


def build_skill_icons():
    os.makedirs(STAGING, exist_ok=True)
    paths = rogue_icon_paths()
    files = []
    for skill, (base, target) in ICONS.items():
        img = capture(paths[base])
        img = without_background(img, img.getpixel((2, 2)))
        path = os.path.join(STAGING, "T_Skill_%s.png" % skill).replace("\\", "/")
        with_glow(retinted(img, target), skill.split("_")[0]).save(path)
        files.append(path)
    print("skill icons:", len(files))
    import_textures(files, "/Game/Amnesia/UI/Skills")


# ---------------------------------------------------------------- the sounds of the ice
RATE = 44100


def noise(n, rnd):
    return [rnd.uniform(-1.0, 1.0) for _ in range(n)]


def highpass(samples, amount=0.92):
    """A rough high pass: what is left when the slow part is taken off."""
    out, low = [], 0.0
    for s in samples:
        low = amount * low + (1.0 - amount) * s
        out.append(s - low)
    return out


def lowpass(samples, amount=0.9):
    out, low = [], 0.0
    for s in samples:
        low = amount * low + (1.0 - amount) * s
        out.append(low)
    return out


def add(track, start, samples, gain=1.0):
    for i, s in enumerate(samples):
        j = start + i
        if 0 <= j < len(track):
            track[j] += s * gain


def ring(freq, length, decay, rnd, vibrato=0.0):
    n = int(length * RATE)
    phase = rnd.uniform(0, 6.28)
    out = []
    for i in range(n):
        t = i / RATE
        f = freq * (1.0 + vibrato * math.sin(t * 30.0))
        out.append(math.sin(phase + 2 * math.pi * f * t) * math.exp(-t * decay))
    return out


def write_wav(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    data = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s / peak * 0.85)) * 32767)) for s in samples)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data)


def ice_freeze(rnd):
    """Ice forming: a cold swell, crackles that get denser, and a crystal ring."""
    length = 1.3
    track = [0.0] * int(length * RATE)
    swell = lowpass(noise(int(0.5 * RATE), rnd), 0.93)
    add(track, 0, [s * math.sin(math.pi * i / len(swell)) ** 2 * 5.0 for i, s in enumerate(swell)], 0.5)
    t = 0.02
    while t < 0.75:
        click = highpass(noise(int(rnd.uniform(0.004, 0.02) * RATE), rnd))
        add(track, int(t * RATE), [s * math.exp(-i / (0.004 * RATE)) for i, s in enumerate(click)], rnd.uniform(0.5, 1.2) * (0.4 + t))
        t += rnd.uniform(0.012, 0.07) * (1.1 - t)
    for freq, delay, gain in ((2093.0, 0.05, 0.5), (3136.0, 0.12, 0.4), (4699.0, 0.2, 0.3), (6272.0, 0.3, 0.2)):
        add(track, int(delay * RATE), ring(freq, 1.0, 5.0, rnd, vibrato=0.004), gain)
    return track


def ice_shatter(rnd):
    """Ice breaking: a thump and a crack, then glass pieces falling."""
    length = 1.2
    track = [0.0] * int(length * RATE)
    add(track, 0, [math.sin(2 * math.pi * 90.0 * i / RATE) * math.exp(-i / (0.03 * RATE)) for i in range(int(0.12 * RATE))], 1.2)
    crack = highpass(noise(int(0.06 * RATE), rnd), 0.85)
    add(track, 0, [s * math.exp(-i / (0.012 * RATE)) for i, s in enumerate(crack)], 1.6)
    for _ in range(46):
        t = rnd.uniform(0.01, 0.95) ** 1.4
        freq = rnd.uniform(2500.0, 9000.0)
        add(track, int(t * RATE), ring(freq, 0.18, rnd.uniform(25.0, 70.0), rnd), rnd.uniform(0.15, 0.6) * (1.2 - t))
    return track


def build_sounds():
    os.makedirs(STAGING, exist_ok=True)
    rnd = random.Random(7)
    files = []
    for name, maker in (("SW_Ice_Freeze_01", ice_freeze), ("SW_Ice_Shatter_01", ice_shatter)):
        path = os.path.join(STAGING, name + ".wav").replace("\\", "/")
        write_wav(path, maker(rnd))
        files.append(path)
    result = ue.job([{"op": "import", "dest": "/Game/Audio/World", "files": files}, {"op": "save"}], timeout=600)
    print("sounds imported", result["imported"], result["errors"])


if __name__ == "__main__":
    # --only <id> limits the pictures and icons to one weapon (its skills start with the id)
    if "--only" in sys.argv:
        only = sys.argv[sys.argv.index("--only") + 1]
        sys.argv.remove(only)
        sys.argv.remove("--only")
        WEAPON_VIEWS = {k: v for k, v in WEAPON_VIEWS.items() if k == only}
        ICONS = {k: v for k, v in ICONS.items() if k.startswith(only + "_")}
    flags = set(sys.argv[1:]) or {"--weapons", "--skills", "--sounds"}
    if "--weapons" in flags:
        build_weapon_icons()
    if "--skills" in flags:
        build_skill_icons()
    if "--sounds" in flags:
        build_sounds()
