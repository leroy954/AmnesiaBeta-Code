"""Runs inside the editor (python editor_py.py retarget_anims.py <set> [target]). Copies the animations of a pack set onto another skeleton
with an IK retargeter: an IK Rig for the source mesh (chains made by the engine's skeleton templates), a retargeter source -> target with fuzzy
chain mapping and auto aligned bones, then the batch retarget into /Game/Amnesia/Animation/Combat/<Set>/<target>.
The rigs and retargeters live in /Game/Amnesia/Animation/Rigs and are reused when they exist. Results go to Saved/retarget_result.txt.
The packs are not in git: after adding them again from Fab, the retargeted copies in Amnesia/Animation/Combat are already there.
Mixamo moves the hips forward in its attacks (no root bone): after retargeting such a set, run anim_in_place.py on the output folder so it plays in place."""
import re
import sys
import traceback

import unreal

SEV = "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/"
GRUX = "/Game/ParagonGrux/Characters/Heroes/Grux/Animations/"

SETS = {
    # heavy two-handed swings: the axe
    "Sevarog": {
        "mesh": "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog",
        "anims": [SEV + n for n in ("Swing1_Medium", "Swing2_Medium", "Swing3_Medium", "Swing1_FAST_v2", "Swing2_FAST_v2", "Swing3_FAST_v2",
                                     "Swing_In_Air", "Ultimate_Swing_120fps")],
    },
    # brute strikes with both arms, hit reaction and death
    "Grux": {
        "mesh": "/Game/ParagonGrux/Characters/Heroes/Grux/Meshes/Grux",
        "anims": [GRUX + n for n in ("PrimaryAttack_LA", "PrimaryAttack_RA", "PrimaryAttack_LB", "PrimaryAttack_RB", "PrimaryAttack_FourStrikes",
                                      "Attack_Melee_Air", "HitReact_Front", "Death_A")],
    },
    # Mixamo mocap (Tools/mixamo_import.py brings it in): the combat animations of every pack folder, each folder into its own output folder
    "Mixamo": {
        "mesh": "/Game/Mixamo/Character/X_Bot",
        "folders": ["/Game/Mixamo/Sword_and_Shield_Pack", "/Game/Mixamo/Pro_Melee_Axe_Pack", "/Game/Mixamo/Great_Sword_Pack",
                    "/Game/Mixamo/Pro_Magic_Pack", "/Game/Mixamo/Single"],
        "in_place": True,
        "include": r"attack|slash|kick|impact|react|death|dying|block|cast|spell|power|throw|punch|swiping|roar|combo|spin|taunt",
    },
}

MIX = "/Game/Mixamo/"
SETS["MixamoEnemy"] = {
    # the enemies' share of the Mixamo mocap: axe and sword blows, a punch and a claw swipe, hit reactions and deaths
    "mesh": "/Game/Mixamo/Character/X_Bot",
    "in_place": True,
    "anims": [MIX + "Pro_Melee_Axe_Pack/" + n for n in ("standing_melee_attack_horizontal", "standing_melee_attack_downward", "standing_melee_attack_backhand",
                                                          "standing_melee_attack_360_low", "standing_react_large_gut", "standing_react_large_from_left",
                                                          "standing_react_large_from_right", "standing_taunt_battlecry")]
             + [MIX + "Sword_and_Shield_Pack/" + n for n in ("sword_and_shield_attack__4_", "sword_and_shield_slash__3_", "sword_and_shield_attack__2_",
                                                             "sword_and_shield_kick", "sword_and_shield_impact", "sword_and_shield_impact__2_",
                                                             "sword_and_shield_death", "sword_and_shield_death__2_")]
             + [MIX + "Pro_Magic_Pack/" + n for n in ("Standing_React_Death_Backward", "Standing_React_Death_Forward", "Standing_React_Small_From_Front")]
             + [MIX + "Single/" + n for n in ("Mutant_Punch", "Mutant_Swiping", "Mutant_Roaring", "Mutant_Dying")],
}

# target mesh and its IK Rig; None makes a rig from the engine's skeleton templates
TARGETS = {
    "Hero": ("/Game/ElfArden/BaseMesh/SK_ElfArden", "/Game/ElfArden/Blueprint/Rig/IK_ElfArden"),
    "Orc": ("/Game/Low_Poly_Orc_Warrior/Meshes/Orc_Warrior/Combined/SK_Orc_Warrior_Combined1", "/Game/Low_Poly_Orc_Warrior/Rig/Orc_Warrior/IK_Orc_Warrior"),
    "SkeletonGuard": ("/Game/Skeleton_Guard/Mesh_UE4/Full/SKM_Skeleton_Guard", None),
}

RIGS = "/Game/Amnesia/Animation/Rigs"
OUT = "/Game/Amnesia/Animation/Combat"

lines = []


def log(*a):
    lines.append(" ".join(str(x) for x in a))


tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary


def source_rig(name, mesh):
    path = f"{RIGS}/IK_{name}"

    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)

    rig = tools.create_asset(f"IK_{name}", RIGS, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(mesh)
    log("template found for", name, ctrl.apply_auto_generated_retarget_definition())
    log("chains", [str(c.get_editor_property("chain_name")) for c in ctrl.get_retarget_chains()], "root", ctrl.get_retarget_root())
    EAL.save_loaded_asset(rig)
    return rig


def root_from_pelvis(rtg):
    """The source has no root bone of its own (Mixamo): the root motion is made from the pelvis, so a root lock keeps the animation in place"""
    ctrl = unreal.IKRetargeterController.get_controller(rtg)
    log("root motion sources", [n for n in dir(unreal.RootMotionSource) if n.isupper()])

    for i in range(ctrl.get_num_retarget_ops()):
        if str(ctrl.get_op_name(i)) == "Root Motion":
            op = ctrl.get_op_controller(i)
            settings = op.get_settings()
            settings.set_editor_property("root_motion_source", unreal.RootMotionSource.GENERATE_FROM_TARGET_PELVIS)
            op.set_settings(settings)
            log("root motion from", op.get_settings().get_editor_property("root_motion_source"))

    EAL.save_loaded_asset(rtg)


def retargeter(name, target, src_rig, tgt_rig):
    path = f"{RIGS}/RTG_{name}_{target}"

    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)

    rtg = tools.create_asset(f"RTG_{name}_{target}", RIGS, unreal.IKRetargeter, unreal.IKRetargetFactory())
    ctrl = unreal.IKRetargeterController.get_controller(rtg)

    if ctrl.get_num_retarget_ops() == 0:
        ctrl.add_default_ops()

    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)

    try:
        ctrl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
        ctrl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    except Exception as e:
        log("assign_ik_rig_to_all_ops:", e)

    ctrl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    ctrl.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    log("ops", [str(ctrl.get_op_name(i)) for i in range(ctrl.get_num_retarget_ops())])
    EAL.save_loaded_asset(rtg)
    return rtg


try:
    name = sys.argv[1]
    target = sys.argv[2] if len(sys.argv) > 2 else "Hero"
    spec = SETS[name]
    tgt_mesh_path, tgt_rig_path = TARGETS[target]

    src_mesh = unreal.load_asset(spec["mesh"])
    tgt_mesh = unreal.load_asset(tgt_mesh_path)
    src_rig = source_rig(name, src_mesh)
    tgt_rig = unreal.load_asset(tgt_rig_path) if tgt_rig_path else source_rig(target, tgt_mesh)
    rtg = retargeter(name, target, src_rig, tgt_rig)

    if spec.get("in_place"):
        root_from_pelvis(rtg)

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    batches = []

    if "anims" in spec:
        assets = []

        for p in spec["anims"]:
            data = registry.get_asset_by_object_path(p + "." + p.split("/")[-1])

            if data.is_valid():
                assets.append(data)
            else:
                log("missing", p)

        batches.append((f"{OUT}/{name}/{target}", assets))

    for folder in spec.get("folders", []):
        pattern = re.compile(spec.get("include", "."), re.IGNORECASE)
        assets = [a for a in registry.get_assets_by_path(folder, recursive=False)
                  if str(a.asset_class_path.asset_name) == "AnimSequence" and pattern.search(str(a.asset_name))]
        batches.append((f"{OUT}/{name}/{folder.split('/')[-1]}/{target}", assets))

    for out_dir, assets in batches:
        made = unreal.IKRetargetBatchOperation.duplicate_and_retarget(assets, src_mesh, tgt_mesh, rtg, "", "", "", "", target_path=out_dir,
                                                                      use_source_path=False, include_referenced_assets=False,
                                                                      overwrite_existing_files=True)
        log("made", len(made), "in", out_dir)

        if spec.get("in_place"):
            for a in made:
                seq = unreal.load_asset(str(a.package_name))
                seq.set_editor_property("force_root_lock", True)

        for a in made:
            log(" ", str(a.package_name))

        EAL.save_directory(out_dir, only_if_is_dirty=False, recursive=True)
except Exception:
    log(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + "retarget_result.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
