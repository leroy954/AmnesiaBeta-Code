"""Builds the skill icons and the font of the skill screen.

The 16 icons come from the Kirosho skill icon pack (/Game/KiroshoSkIconsPk0). Each branch has its color, so a few icons get a
hue shift: the purple ones turn blue for the ice branch, the orange ones turn green for the dragon branch. The result is imported as
/Game/Amnesia/UI/Skills/T_Skill_<skill id>, which is where AmnesiaSkills.cpp looks for them.

The Cinzel font comes from the zip the user downloaded (OFL). Its static weights are imported as font faces in /Game/UI/Fonts.

Usage: python skill_icons.py [--fonts-only] [--icons-only] [--cinzel <folder with the static ttf files>]"""
import base64
import io
import os
import sys

from PIL import Image

import ue

# skill id: (icon in the pack, hue shift in degrees)
ICONS = {
    "steel_power": ("StrokeOfAnger", 0),
    "steel_body": ("UltimatePower", 0),
    "steel_blood": ("BloodyGrip", 0),
    "steel_swift": ("RapidSpear_Attack", 0),
    "fire_blade": ("Berserk", 0),
    "fire_embers": ("StealthAttack", 0),
    "fire_burst": ("StormAttack", 0),
    "fire_fury": ("FuryAttack", 0),
    "ice_blade": ("DeepCut", -70),
    "ice_chill": ("CursedCut", -65),
    "ice_nova": ("EnergyRecharge", 0),
    "ice_armor": ("Evade", 0),
    "dragon_fire": ("VindictiveFury", 90),
    "dragon_haste": ("PiercingAttack", -145),
    "dragon_lantern": ("CursedFury", 0),
    "dragon_guard": ("Regeneration", 0),
}

HERE = os.path.dirname(os.path.abspath(__file__))
STAGING = os.path.join(HERE, "staging", "skill_icons")


def shifted(img, degrees):
    """Rotates the hue of an RGBA image and keeps its alpha."""
    if degrees == 0:
        return img
    alpha = img.getchannel("A")
    h, s, v = img.convert("RGB").convert("HSV").split()
    step = round(degrees * 255.0 / 360.0)
    h = h.point(lambda value: (value + step) % 256)
    out = Image.merge("HSV", (h, s, v)).convert("RGB")
    out.putalpha(alpha)
    return out


def build_icons():
    os.makedirs(STAGING, exist_ok=True)
    files = []
    for skill, (name, degrees) in ICONS.items():
        raw = ue.call(ue.E, "CaptureAssetImage", assetPath="/Game/KiroshoSkIconsPk0/Textures/T_%s.T_%s" % (name, name))
        img = Image.open(io.BytesIO(base64.b64decode(raw["data"]))).convert("RGBA")
        path = os.path.join(STAGING, "T_Skill_%s.png" % skill).replace("\\", "/")
        shifted(img, degrees).save(path)
        files.append(path)
    print("exported", len(files), "icons to", STAGING)

    result = ue.job([{"op": "import", "dest": "/Game/Amnesia/UI/Skills", "files": files}], timeout=600)
    print("imported", len(result["imported"]), "errors", result["errors"])

    # crisp icons: no mips, the UI texture group, no compression artifacts
    props = [{"op": "set_object_props", "object": path, "props": {
        "CompressionSettings": "TC_EditorIcon", "LODGroup": "TEXTUREGROUP_UI", "MipGenSettings": "TMGS_NoMipmaps"}} for path in result["imported"]]
    props.append({"op": "save"})
    print("settings", ue.job(props, timeout=600)["modified"])


def import_fonts(folder):
    weights = ["Cinzel-Regular.ttf", "Cinzel-SemiBold.ttf", "Cinzel-Bold.ttf", "Cinzel-Black.ttf"]
    files = [os.path.join(folder, name).replace("\\", "/") for name in weights if os.path.exists(os.path.join(folder, name))]
    result = ue.job([{"op": "import", "dest": "/Game/UI/Fonts", "files": files}, {"op": "save"}], timeout=600)
    print("fonts", result["imported"], "errors", result["errors"])


if __name__ == "__main__":
    args = sys.argv[1:]
    cinzel = args[args.index("--cinzel") + 1] if "--cinzel" in args else None
    if "--fonts-only" not in args:
        build_icons()
    if "--icons-only" not in args and cinzel:
        import_fonts(cinzel)
