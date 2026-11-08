"""Makes BP_Orc_Warrior (and BP_Orc_Warrior_2, _3 with the other meshes and weapons of the pack): a copy of BP_Skeleton_Guard (capsule, light, AI, creature anim instance) with the Orc Warrior mesh, its animations, the axe and
the shield. Safe to run again: it only sets the properties. Usage: python orc_bp.py [variant 1..3, default 1]"""
import sys

import ue

VARIANT = int(sys.argv[1]) if len(sys.argv) > 1 else 1
# the first orc is made from the skeleton guard, the other looks from the first orc (so they keep its sounds)
SRC = "/Game/Amnesia/Blueprints/BP_Skeleton_Guard" if VARIANT == 1 else "/Game/Amnesia/Blueprints/BP_Orc_Warrior"
DST = "/Game/Amnesia/Blueprints/BP_Orc_Warrior" + ("" if VARIANT == 1 else "_%d" % VARIANT)
PACK = "/Game/Low_Poly_Orc_Warrior/"
ANIMS = PACK + "Animations/Orc_Warrior/"


def obj(path):
    name = path.rsplit("/", 1)[1]
    return path + "." + name


def anim(n):
    return obj(ANIMS + n)


if not ue.call(ue.AS, "exists", path=DST):
    ue.call(ue.AS, "duplicate", path=SRC, new_path=DST)
    print("duplicated", SRC, "->", DST)

cdo = DST + ".Default__" + DST.rsplit("/", 1)[1] + "_C"

# the body: a 2 m orc, the capsule around it, the mesh standing on the capsule floor and facing along the walking line
ue.props({"refPath": cdo + ":CollisionCylinder"}, CapsuleHalfHeight=100, CapsuleRadius=45)
ue.props({"refPath": cdo + ":CharacterMesh0"},
         SkeletalMeshAsset=obj(PACK + "Meshes/Orc_Warrior/Combined/SK_Orc_Warrior_Combined%d" % VARIANT),
         RelativeLocation={"x": 0, "y": 0, "z": -100},
         RelativeRotation={"pitch": 0, "yaw": -90, "roll": 0})

# the axe in the right hand and the shield on the left, placed like the demo Blueprint of the pack
props = [
    {"mesh": obj(PACK + "Meshes/Weapons/SM_Orc_Warrior_Axe%d" % VARIANT), "socket": "hand_r",
     "offset": {"location": {"x": -19, "y": 0, "z": 4}, "rotation": {"pitch": 0, "yaw": 180, "roll": 0}, "scale": {"x": 1, "y": 1, "z": 1}}},
    {"mesh": obj(PACK + "Meshes/Weapons/SM_Orc_Warrior_Shield%d" % VARIANT), "socket": "hand_l",
     "offset": {"location": {"x": 15.9, "y": 9.9, "z": 0}, "rotation": {"pitch": 0, "yaw": 170, "roll": 180}, "scale": {"x": 1, "y": 1, "z": 1}}},
]

ue.props({"refPath": cdo},
         IdleAnimation=anim("AS_01_Orc_Warrior_Idle1"),
         WalkAnimation=anim("AS_03_Orc_Warrior_Walk"),
         RunAnimation=anim("AS_04_Orc_Warrior_Run"),
         DeathAnimation=anim("AS_10_Orc_Warrior_Death"),
         HitAnimations=[anim("AS_09_Orc_Warrior_Hit")],
         Attacks=[{"animation": anim("AS_07_Orc_Warrior_Attack1"), "hitTime": 0.45, "sourceBone": "hand_r", "playRate": 1.0},
                  {"animation": anim("AS_08_Orc_Warrior_Attack2"), "hitTime": 0.5, "sourceBone": "hand_r", "playRate": 1.0}],
         AttachedProps=props,
         LungeDuration=0.0,
         WalkAnimationSpeed=160.0,
         RunAnimationSpeed=420.0,
         PatrolSpeed=110.0,
         ChaseSpeed=320.0,
         AttackRange=190.0,
         SizeScale=1.0)

ue.call("editor_toolset.toolsets.blueprint.BlueprintTools", "compile_blueprint", blueprint={"refPath": DST + "." + DST.rsplit("/", 1)[1]})
ue.call(ue.AS, "save_assets", asset_paths=[DST])

print(ue.get_props({"refPath": cdo}, "IdleAnimation", "DeathAnimation", "HitAnimations", "Attacks", "AttachedProps", "AttackRange", "SizeScale"))
print(ue.get_props({"refPath": cdo + ":CharacterMesh0"}, "SkeletalMeshAsset", "RelativeLocation", "RelativeRotation"))
