"""Runs inside the editor (python editor_py.py anim_travel.py <folder> [name filter]). For every animation sequence in the folder: length, root motion
flags, and how far the root and the pelvis bones travel (start and end position, and the largest distance from the start)."""
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    folder = sys.argv[1] if sys.argv[1].startswith("/") else "/" + sys.argv[1]
    filt = sys.argv[2].lower() if len(sys.argv) > 2 else ""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    for a in sorted(registry.get_assets_by_path(folder, recursive=True), key=lambda a: str(a.asset_name)):
        if str(a.asset_class_path.asset_name) != "AnimSequence" or filt not in str(a.asset_name).lower():
            continue

        seq = unreal.load_asset(str(a.package_name))
        length = seq.get_play_length()
        row = [str(a.asset_name), "%.2fs" % length, "rm", seq.get_editor_property("enable_root_motion"), "lock", seq.get_editor_property("force_root_lock")]

        for bone in ("root", "pelvis"):
            p0 = unreal.AnimationLibrary.get_bone_pose_for_time(seq, bone, 0.0, True).translation
            far = 0.0

            for i in range(1, 21):
                p = unreal.AnimationLibrary.get_bone_pose_for_time(seq, bone, length * i / 20, True).translation
                far = max(far, ((p.x - p0.x) ** 2 + (p.y - p0.y) ** 2) ** 0.5)

            row += [bone, "end %.0f,%.0f" % (p.x - p0.x, p.y - p0.y), "max %.0f" % far]

        log(*row)
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
