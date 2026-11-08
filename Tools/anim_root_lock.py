"""Runs inside the editor (python editor_py.py anim_root_lock.py <folder> [off]). Sets Force Root Lock on every animation sequence under the folder
(recursive), so the Mixamo attacks that step forward play in place: the game moves the hero, not the animation. Also reports, for the first
sequence, how far its root and pelvis travel."""
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    folder = sys.argv[1] if sys.argv[1].startswith("/") else "/" + sys.argv[1]  # Git Bash rewrites a leading slash
    lock = not (len(sys.argv) > 2 and sys.argv[2] == "off")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = [a for a in registry.get_assets_by_path(folder, recursive=True) if str(a.asset_class_path.asset_name) == "AnimSequence"]
    first = True

    for a in assets:
        seq = unreal.load_asset(str(a.package_name))

        if first:
            first = False
            length = seq.get_play_length()

            for bone in ("root", "pelvis"):
                p0 = unreal.AnimationLibrary.get_bone_pose_for_time(seq, bone, 0.0, True).translation
                p1 = unreal.AnimationLibrary.get_bone_pose_for_time(seq, bone, length, True).translation
                log(seq.get_name(), bone, "start", p0, "end", p1)

        seq.set_editor_property("force_root_lock", lock)
        unreal.EditorAssetLibrary.save_loaded_asset(seq)

    log("root lock", lock, "on", len(assets))
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
