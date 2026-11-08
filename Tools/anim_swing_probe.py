"""Runs inside the editor (python editor_py.py anim_swing_probe.py <folder> [bone] [name filter]). Finds the swings in animation sequences without a
running game: the bone (default hand_r) is followed in component space at 60 Hz, and the fast parts of its motion are listed: start, end, peak speed,
and the moment it reaches farthest forward (+Y is the mesh's forward) with that reach. Also how upright the body stays (lowest pelvis height) and how
far the chest turns away from the side view (largest yaw of spine_03 from the first frame)."""
import math
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    folder = sys.argv[1] if sys.argv[1].startswith("/") else "/" + sys.argv[1]
    bone = sys.argv[2] if len(sys.argv) > 2 else "hand_r"
    filt = sys.argv[3].lower() if len(sys.argv) > 3 else ""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    P = unreal.AnimPoseExtensions
    opts = unreal.AnimPoseEvaluationOptions()
    space = unreal.AnimPoseSpaces.WORLD

    for a in sorted(registry.get_assets_by_path(folder, recursive=True), key=lambda a: str(a.asset_name)):
        if str(a.asset_class_path.asset_name) != "AnimSequence" or filt not in str(a.asset_name).lower():
            continue

        seq = unreal.load_asset(str(a.package_name))
        length = seq.get_play_length()
        n = max(2, int(length * 60))
        pts, low, turn, yaw0 = [], 1e9, 0.0, None

        for i in range(n + 1):
            t = length * i / n
            pose = P.get_anim_pose_at_time(seq, t, opts)
            p = P.get_bone_pose(pose, bone, space).translation
            pts.append((t, p.x, p.y, p.z))
            low = min(low, P.get_bone_pose(pose, "pelvis", space).translation.z)
            fwd = P.get_bone_pose(pose, "spine_03", space).rotation.get_axis_x()
            yaw = math.degrees(math.atan2(fwd.y, fwd.x))
            yaw0 = yaw if yaw0 is None else yaw0
            turn = max(turn, abs((yaw - yaw0 + 180) % 360 - 180))

        speeds = []

        for i in range(1, len(pts)):
            dt = pts[i][0] - pts[i - 1][0]
            d = math.dist(pts[i][1:], pts[i - 1][1:])
            speeds.append(d / dt if dt > 0 else 0.0)

        segs, cur = [], None

        for i, v in enumerate(speeds):
            t, y = pts[i + 1][0], pts[i + 1][2]

            if v > 300:
                if cur is None:
                    cur = {"s": t, "e": t, "peak": v, "reach": y, "at": t}

                cur["e"] = t
                cur["peak"] = max(cur["peak"], v)

                if y > cur["reach"]:
                    cur["reach"], cur["at"] = y, t
            elif cur is not None and t - cur["e"] > 0.1:
                segs.append(cur)
                cur = None

        if cur:
            segs.append(cur)

        desc = "  ".join("[%.2f-%.2f peak %.0f reach %.0f@%.2f]" % (s["s"], s["e"], s["peak"], s["reach"], s["at"]) for s in segs if s["e"] - s["s"] >= 0.05)
        log("%-36s %.2fs pelvis>=%.0f turn %3.0f  %s" % (a.asset_name, length, low, turn, desc))
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
