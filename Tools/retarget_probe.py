"""Runs inside the editor (py command). Lists the IK Rig / retargeter Python API of this engine version and what the existing
rigs hold, into Saved/retarget_result.txt."""
import unreal

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


try:
    for name in ("IKRigController", "IKRetargeterController", "IKRetargetBatchOperation", "RetargetSourceOrTarget", "AutoMapChainType",
                 "IKRetargetFactory", "IKRigDefinitionFactory", "RetargetAutoAlignMethod"):
        c = getattr(unreal, name, None)
        log("==", name, [n for n in dir(c) if not n.startswith("_")] if c else "MISSING")

    for name in ("IKRetargetBatchOperation.duplicate_and_retarget", "IKRetargeterController.auto_map_chains", "IKRetargeterController.set_ik_rig",
                 "IKRigController.apply_auto_generated_retarget_definition", "IKRetargeterController.auto_align_all_bones"):
        cls, fn = name.split(".")
        f = getattr(getattr(unreal, cls, None), fn, None)
        log("doc", name, (f.__doc__ or "")[:900] if f else "MISSING")

    rtg = unreal.load_asset("/Game/ElfArden/Blueprint/Rig/RTG_ElfArden")
    ctrl = unreal.IKRetargeterController.get_controller(rtg)
    for side in ("SOURCE", "TARGET"):
        s = getattr(unreal.RetargetSourceOrTarget, side)
        try:
            log("RTG_ElfArden", side, ctrl.get_ik_rig(s).get_path_name(), ctrl.get_preview_mesh(s).get_path_name() if ctrl.get_preview_mesh(s) else None)
        except Exception as e:
            log("RTG_ElfArden", side, "err", e)

    rig = unreal.load_asset("/Game/ElfArden/Blueprint/Rig/IK_ElfArden")
    rc = unreal.IKRigController.get_controller(rig)
    log("IK_ElfArden mesh", rc.get_skeletal_mesh().get_path_name() if rc.get_skeletal_mesh() else None)
    log("IK_ElfArden chains", [(c.chain_name, c.start_bone.bone_name, c.end_bone.bone_name) for c in rc.get_retarget_chains()])
except Exception:
    import traceback
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
