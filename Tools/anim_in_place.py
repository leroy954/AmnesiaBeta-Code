"""Runs inside the editor (python editor_py.py anim_in_place.py <folder> [bone] [name filter]). Makes the animation sequences under the folder play in
place, like Mixamo's "In Place": the pelvis (or the given bone) keeps its height and rotation, but its horizontal position stays where it was on the
first frame. The travel that was removed is reported per sequence (the game can step the hero forward instead)."""
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    folder = sys.argv[1] if sys.argv[1].startswith("/") else "/" + sys.argv[1]
    bone = sys.argv[2] if len(sys.argv) > 2 else "pelvis"
    filt = sys.argv[3].lower() if len(sys.argv) > 3 else ""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    lib = unreal.AnimationLibrary

    for a in sorted(registry.get_assets_by_path(folder, recursive=True), key=lambda a: str(a.asset_name)):
        if str(a.asset_class_path.asset_name) != "AnimSequence" or filt not in str(a.asset_name).lower():
            continue

        seq = unreal.load_asset(str(a.package_name))
        keys = lib.get_num_keys(seq)
        poses = [lib.get_bone_pose_for_frame(seq, bone, i, False) for i in range(keys)]
        x0, y0 = poses[0].translation.x, poses[0].translation.y
        far = max(((p.translation.x - x0) ** 2 + (p.translation.y - y0) ** 2) ** 0.5 for p in poses)
        end = poses[-1].translation

        positions = [unreal.Vector(x0, y0, p.translation.z) for p in poses]
        rotations = [p.rotation for p in poses]
        scales = [p.scale3d for p in poses]

        ctrl = seq.get_editor_property("controller")
        ctrl.open_bracket(unreal.Text("in place"), False)
        ctrl.set_bone_track_keys(bone, positions, rotations, scales, False)
        ctrl.close_bracket(False)
        unreal.EditorAssetLibrary.save_loaded_asset(seq)
        log(str(a.asset_name), "keys", keys, "removed: end %.0f,%.0f" % (end.x - x0, end.y - y0), "max %.0f" % far)
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
