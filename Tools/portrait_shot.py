"""Takes the picture of the little dragon for the shop: the dragon mesh with its orange toon material and two lights, placed in front of the
editor camera, captured from a closer pose. The temporary actors are removed afterwards (run with `clean` to only remove them).
Usage: python portrait_shot.py <out png> [dragon yaw offset] [camera distance] [light intensity] [sideways shift]   or   python portrait_shot.py clean"""
import base64
import io
import math
import sys

from PIL import Image

import ue

E = ue.E
DRAGON = "/Game/LittleDragons/Meshes/SK_Little_Dragon_Tiger_Toon.SK_Little_Dragon_Tiger_Toon"
ORANGE = "/Game/LittleDragons/Materials/Little_Dragon_Tiger/Toon/MI_TD_Toon_Orange.MI_TD_Toon_Orange"
FOLDER = "Temp/Portrait"


def clean():
    print(ue.job([{"op": "delete", "folder": FOLDER}], timeout=120))


if sys.argv[1] == "clean":
    clean()
    sys.exit()

OUT = sys.argv[1]
YAW = float(sys.argv[2]) if len(sys.argv) > 2 else 35.0
DIST = float(sys.argv[3]) if len(sys.argv) > 3 else 135.0
LIGHT = float(sys.argv[4]) if len(sys.argv) > 4 else 120.0
SHIFT = float(sys.argv[5]) if len(sys.argv) > 5 else 0.0

clean()
cam = ue.call(E, "GetCameraTransform")
loc, rot = cam["location"], cam["rotation"]
yaw = math.radians(rot["yaw"])
fwd = (math.cos(yaw), math.sin(yaw))
right = (-math.sin(yaw), math.cos(yaw))
base = (loc["x"] + fwd[0] * 260 + right[0] * SHIFT, loc["y"] + fwd[1] * 260 + right[1] * SHIFT, loc["z"] - 40)

actor = ue.spawn(DRAGON, "PortraitDragon", base[0], base[1], base[2], yaw=rot["yaw"] + 90 + YAW, folder=FOLDER)
comps = ue.call(ue.A, "get_components", actor=actor)
mesh = [c for c in comps if "SkeletalMeshComponent" in c["refPath"]][0]
ue.props(mesh, OverrideMaterials=[ORANGE])

# the camera is on the dragon's front side: the key light is up and to the left of it, the fill low on the right
def at(r, f, z):
    return (base[0] + right[0] * r - fwd[0] * f, base[1] + right[1] * r - fwd[1] * f, base[2] + z)

ops = []
for name, pos, color, intensity in (("Key", at(-130, 110, 130), (1.0, 0.82, 0.6), LIGHT), ("Fill", at(150, 90, 20), (0.6, 0.75, 1.0), LIGHT * 0.35)):
    ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaGlowLight", "loc": list(pos), "label": "Portrait_" + name, "folder": FOLDER,
                "props": {"LightColor": "(R=%.3f,G=%.3f,B=%.3f,A=1.0)" % color, "Intensity": intensity, "Radius": 600.0, "FlickerAmount": 0.0}})
print(ue.job(ops, timeout=120)["spawned"])

# the capture camera: closer, level with the dragon's head, looking at it
target = (base[0], base[1], base[2] + 38)
pose = {"location": {"x": base[0] - fwd[0] * DIST, "y": base[1] - fwd[1] * DIST, "z": target[2] + 8},
        "rotation": {"pitch": -3.0, "yaw": rot["yaw"], "roll": 0.0}, "scale": {"x": 1, "y": 1, "z": 1}}
res = ue.call(E, "CaptureViewport", captureTransform=pose,
              annotations={"gridSpacing": 0, "gridExtent": 0, "gridHeight": 0, "maxLabelDistance": 0, "classFilter": None, "maxLabels": 0}, bShowUI=False)
img = Image.open(io.BytesIO(base64.b64decode(res["image"]["data"])))
img.save(OUT)
print("saved", OUT, img.size)
