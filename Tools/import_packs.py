"""Imports the staged sounds and UI textures into the project and sets their import settings."""
import os, sys, time
import ue

ST = os.path.join(os.path.dirname(os.path.abspath(__file__)), "staging")
AUDIO = os.path.join(ST, "audio")
UI = os.path.join(ST, "ui_import")


def files_in(folder, ext):
    return [os.path.join(folder, f).replace("\\", "/") for f in sorted(os.listdir(folder)) if f.lower().endswith(ext)]


ops = []
for sub in sorted(os.listdir(AUDIO)):
    ops.append({"op": "import", "dest": "/Game/Audio/" + sub, "files": files_in(os.path.join(AUDIO, sub), ".wav")})
for sub in ("FantasyBorders", "RPG"):
    ops.append({"op": "import", "dest": "/Game/UI/" + sub, "files": files_in(os.path.join(UI, sub), ".png")})

t0 = time.time()
r = ue.job(ops, timeout=1800)
print("import:", len(r["imported"]), "assets in %.0fs" % (time.time() - t0), "errors:", r["errors"])

# loops loop, UI art stays crisp and unmipped
props = []
for path in r["imported"]:
    name = path.split(".")[-1]
    if path.startswith("/Game/Audio/Ambience/"):
        props.append({"op": "set_object_props", "object": path, "props": {"bLooping": True}})
    elif path.startswith("/Game/UI/FantasyBorders/"):
        props.append({"op": "set_object_props", "object": path, "props": {
            "CompressionSettings": "TC_EditorIcon", "LODGroup": "TEXTUREGROUP_Pixels2D", "MipGenSettings": "TMGS_NoMipmaps", "Filter": "TF_Nearest"}})
    elif path.startswith("/Game/UI/RPG/"):
        props.append({"op": "set_object_props", "object": path, "props": {
            "CompressionSettings": "TC_EditorIcon", "LODGroup": "TEXTUREGROUP_UI", "MipGenSettings": "TMGS_NoMipmaps"}})
props.append({"op": "save"})
r2 = ue.job(props, timeout=1800)
print("settings:", r2["modified"], "errors:", r2["errors"])
open(os.path.join(ST, "imported.txt"), "w").write("\n".join(r["imported"]))
