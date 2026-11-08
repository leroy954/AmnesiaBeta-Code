"""Runs inside the editor (python editor_py.py mixamo_import.py). Imports the Mixamo downloads from Import/Mixamo (the zips extracted there, one folder
per pack): X Bot as a skeletal mesh into /Game/Mixamo/Character, then every animation FBX with the X Bot skeleton into /Game/Mixamo/<pack>.
The per-pack copies of X Bot are skipped. Existing assets are kept unless the script is run with "replace"."""
import glob
import os
import sys
import traceback

import unreal

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Import", "Mixamo")
DEST = "/Game/Mixamo"
REPLACE = "replace" in sys.argv[1:]
lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


def asset_name(path):
    return "".join(c if c.isalnum() else "_" for c in os.path.splitext(os.path.basename(path))[0]).strip("_")


def task(file, dest, ui):
    t = unreal.AssetImportTask()
    t.filename = file
    t.destination_path = dest
    t.automated = True
    t.save = True
    t.replace_existing = True
    t.options = ui
    return t


try:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mesh_path = f"{DEST}/Character/X_Bot"

    if REPLACE or not unreal.EditorAssetLibrary.does_asset_exist(mesh_path):
        ui = unreal.FbxImportUI()
        ui.import_mesh = True
        ui.import_as_skeletal = True
        ui.import_animations = False
        ui.import_materials = True
        ui.import_textures = False
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        ui.create_physics_asset = False
        t = task(os.path.join(SRC, "X Bot.fbx"), f"{DEST}/Character", ui)
        tools.import_asset_tasks([t])
        log("mesh", list(t.imported_object_paths))

    mesh = unreal.load_asset(mesh_path)
    skeleton = mesh.skeleton
    log("skeleton", skeleton.get_path_name())

    tasks = []

    for file in sorted(glob.glob(os.path.join(SRC, "**", "*.fbx"), recursive=True)):
        if os.path.basename(file).lower() == "x bot.fbx":
            continue

        folder = os.path.basename(os.path.dirname(file))
        pack = "Single" if folder == "Mixamo" else asset_name(folder)
        dest = f"{DEST}/{pack}"

        if not REPLACE and unreal.EditorAssetLibrary.does_asset_exist(f"{dest}/{asset_name(file)}"):
            continue

        ui = unreal.FbxImportUI()
        ui.import_mesh = False
        ui.import_animations = True
        ui.import_materials = False
        ui.import_textures = False
        ui.skeleton = skeleton
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
        tasks.append(task(file, dest, ui))

    log("animation files", len(tasks))
    tools.import_asset_tasks(tasks)
    made = [p for t in tasks for p in t.imported_object_paths]
    log("imported", len(made))

    for p in made[:400]:
        log(" ", p)
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
