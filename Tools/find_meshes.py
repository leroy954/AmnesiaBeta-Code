"""Runs inside the editor (python editor_py.py find_meshes.py <mesh name part> [x min] [x max] [y min] [y max]). Lists the static mesh actors of the
level whose mesh name holds the text (case does not matter): label, mesh, location, scale, bounds height and top, collision profile and tags.
Ends with a count per mesh."""
import collections
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    part = sys.argv[1].lower()
    lim = [float(v) for v in sys.argv[2:6]] + [None] * (4 - len(sys.argv[2:6]))
    x0, x1, y0, y1 = [v if v is not None else d for v, d in zip(lim, (-1e9, 1e9, -1e9, 1e9))]
    counts = collections.Counter()

    for actor in unreal.EditorActorSubsystem().get_all_level_actors() if hasattr(unreal, "EditorActorSubsystem") else []:
        if not isinstance(actor, unreal.StaticMeshActor):
            continue

        comp = actor.static_mesh_component
        mesh = comp.static_mesh

        if not mesh or part not in mesh.get_name().lower():
            continue

        loc = actor.get_actor_location()

        if not (x0 <= loc.x <= x1 and y0 <= loc.y <= y1):
            continue

        counts[mesh.get_name()] += 1
        origin, extent = actor.get_actor_bounds(False)
        log("%-28s %-26s loc %.0f,%.0f,%.0f scale %.2f,%.2f,%.2f h %.0f top %.0f coll %s tags %s" % (
            actor.get_actor_label()[:28], mesh.get_name()[:26], loc.x, loc.y, loc.z, actor.get_actor_scale3d().x, actor.get_actor_scale3d().y,
            actor.get_actor_scale3d().z, extent.z * 2, origin.z + extent.z, comp.get_collision_profile_name(), [str(t) for t in actor.tags]))

    log("== counts", dict(counts))
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
