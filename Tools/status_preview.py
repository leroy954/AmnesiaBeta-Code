"""Previews M_StatusOverlay on an enemy with no rebuild: a material instance with the wanted parameters is put on the enemy as its overlay material.
Usage: python status_preview.py <out folder> <name> '<json: Color [r,g,b], Opacity, RimPower, RimBoost>' [more names and jsons ...]
Example: python status_preview.py out burn '{"Color": [0.9, 0.3, 0.05], "Opacity": 0.35, "RimPower": 2.2, "RimBoost": 3.2}'"""
import json
import os
import sys
import time

from PIL import Image

import ue
import pie_lib as P

MI = "editor_toolset.toolsets.material_instance.MaterialInstanceTools"
FOLDER = "/Game/Amnesia/Materials"
PARENT = {"refPath": FOLDER + "/M_StatusOverlay.M_StatusOverlay"}
OUT = sys.argv[1]
JOBS = list(zip(sys.argv[2::2], [json.loads(j) for j in sys.argv[3::2]]))
os.makedirs(OUT, exist_ok=True)


def instance(name, params):
    path = "%s/MI_Preview_%s" % (FOLDER, name)

    if not ue.call(ue.AS, "exists", path=path):
        ue.call(MI, "create", folder_path=FOLDER, asset_name="MI_Preview_" + name, parent=PARENT)

    inst = {"refPath": path + ".MI_Preview_" + name}
    r, g, b = params.get("Color", [1, 1, 1])
    ue.call(MI, "set_vector_parameter", instance=inst, name="Color", value={"r": r, "g": g, "b": b, "a": 1.0})

    for key in ("Opacity", "RimPower", "RimBoost"):
        if key in params:
            ue.call(MI, "set_scalar_parameter", instance=inst, name=key, value=float(params[key]))

    return path + ".MI_Preview_" + name


def shot(name):
    path = os.path.join(OUT, name + ".png")
    ue.shot(path)
    im = Image.open(path)
    w, h = im.size
    im.crop((int(w * 0.30), int(h * 0.30), int(w * 0.75), int(h * 0.92))).save(path)
    return path


paths = {name: instance(name, params) for name, params in JOBS}

P.start_pie(park_enemies=False)
time.sleep(3)
hero = P.live_hero()
enemy = None
for e in P.pie("Ent_"):
    t = ue.call(ue.A, "get_actor_transform", actor=e)["location"]
    if t["x"] > 2000:
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 420, 0, t["z"] + 120, 0, 0, 0, 1, 1, 1))
        enemy = e
        break
time.sleep(3)
mesh = [c for c in ue.call(ue.A, "get_components", actor=enemy) if c["refPath"].endswith(".CharacterMesh0")][0]
tiles = []

for name, _ in JOBS:
    ue.props(mesh, OverlayMaterial=paths[name])
    time.sleep(1.0)
    tiles.append(Image.open(shot("preview_" + name)))
    print("shot", name)

ue.call(ue.E, "StopPIE")
w, h = tiles[0].size
sheet = Image.new("RGB", (w * len(tiles), h))
for i, im in enumerate(tiles):
    sheet.paste(im.convert("RGB"), (i * w, 0))
sheet.save(os.path.join(OUT, "preview_sheet.png"))
print("sheet", sheet.size)
