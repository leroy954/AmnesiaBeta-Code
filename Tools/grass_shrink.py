"""Runs inside the editor (python editor_py.py grass_shrink.py [max height cm] [apply]). Finds every grass blade of the level, as static mesh actors
and as instances of instanced mesh components (foliage too), by mesh names holding "grass". Without "apply" it only reports, per mesh, how many
there are and how tall they stand now. With "apply", every blade taller than the limit is scaled down, keeping its shape and its foot on the ground,
to 75-100% of the limit (a fixed pick per blade, so the grass is not all one height); shorter ones stay. The level must be saved afterwards (ue.job save)."""
import collections
import sys
import traceback

import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    limit = float(sys.argv[1]) if len(sys.argv) > 1 else 40.0
    apply = "apply" in sys.argv[2:]
    stats = collections.defaultdict(lambda: [0, 1e9, 0.0, 0])
    heights = {}

    def target(key, h):
        """75-100% of the limit, the same for the same blade every run"""
        pick = (zlib_crc(key) % 1000) / 1000.0
        return limit * (0.75 + 0.25 * pick) / h

    def zlib_crc(key):
        import zlib
        return zlib.crc32(key.encode())

    def mesh_height(mesh):
        if mesh not in heights:
            box = mesh.get_bounding_box()
            heights[mesh] = box.max.z - box.min.z
        return heights[mesh]

    for actor in unreal.EditorActorSubsystem().get_all_level_actors():
        if isinstance(actor, unreal.StaticMeshActor):
            comp = actor.static_mesh_component
            mesh = comp.static_mesh

            if not mesh or "grass" not in mesh.get_name().lower():
                continue

            scale = actor.get_actor_scale3d()
            h = mesh_height(mesh) * scale.z
            s = stats[mesh.get_name()]
            s[0] += 1
            s[1] = min(s[1], h)
            s[2] = max(s[2], h)

            if apply and h > limit:
                f = target(actor.get_name(), h)
                actor.modify()
                actor.set_actor_scale3d(unreal.Vector(scale.x * f, scale.y * f, scale.z * f))
                s[3] += 1

        for comp in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh = comp.static_mesh

            if not mesh or "grass" not in mesh.get_name().lower():
                continue

            count = comp.get_instance_count()
            s = stats["ISM " + mesh.get_name()]
            changed = False

            for i in range(count):
                t = comp.get_instance_transform(i, True)
                h = mesh_height(mesh) * t.scale3d.z
                s[0] += 1
                s[1] = min(s[1], h)
                s[2] = max(s[2], h)

                if apply and h > limit:
                    f = target("%s_%d" % (comp.get_name(), i), h)
                    t.scale3d = unreal.Vector(t.scale3d.x * f, t.scale3d.y * f, t.scale3d.z * f)

                    if not changed:
                        comp.modify()
                        changed = True

                    comp.update_instance_transform(i, t, True, False, True)
                    s[3] += 1

            if changed:
                comp.mark_render_state_dirty()

    for name, (n, lo, hi, done) in sorted(stats.items()):
        log("%-34s %5d  height %.0f-%.0f cm  shrunk %d" % (name, n, lo, hi, done))

    log("limit", limit, "applied" if apply else "report only")
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
