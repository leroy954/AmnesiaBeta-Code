"""Gives the orcs and the skeleton guards the Mixamo mocap attacks, hit reactions and deaths (retargeted by retarget_anims.py MixamoEnemy Orc /
SkeletonGuard, played in place by anim_in_place.py). The hit times are the moments the hand reaches farthest forward (anim_swing_probe.py).
With the editor running: python enemy_mocap.py [--old]   (--old puts the orcs' own pack animations back)"""
import sys

import ue

OUT = "/Game/Amnesia/Animation/Combat/MixamoEnemy/"
BP = "/Game/Amnesia/Blueprints/"


def obj(path):
    return path + "." + path.rsplit("/", 1)[1]


def anim(target, name):
    return obj(OUT + target + "/" + name)


def attack(target, name, hit, rate=1.0, bone="hand_r"):
    return {"animation": anim(target, name), "hitTime": hit, "sourceBone": bone, "playRate": rate}


SETUPS = {
    # axe blows: a side cut, an overhead chop and a backhand
    "Orc": (["BP_Orc_Warrior", "BP_Orc_Warrior_2", "BP_Orc_Warrior_3"], dict(
        Attacks=[attack("Orc", "standing_melee_attack_horizontal", 0.93, 1.3),
                 attack("Orc", "standing_melee_attack_downward", 0.82, 1.3),
                 attack("Orc", "standing_melee_attack_backhand", 1.03, 1.3)],
        HitAnimations=[anim("Orc", "standing_react_large_gut"), anim("Orc", "Standing_React_Small_From_Front")],
        DeathAnimation=anim("Orc", "Standing_React_Death_Backward"))),
    # sword cuts
    "SkeletonGuard": (["BP_Skeleton_Guard"], dict(
        Attacks=[attack("SkeletonGuard", "sword_and_shield_attack__4_", 0.50, 1.0),
                 attack("SkeletonGuard", "sword_and_shield_slash__3_", 0.81, 1.0),
                 attack("SkeletonGuard", "sword_and_shield_attack__2_", 0.57, 1.0)],
        HitAnimations=[anim("SkeletonGuard", "sword_and_shield_impact"), anim("SkeletonGuard", "sword_and_shield_impact__2_")],
        DeathAnimation=anim("SkeletonGuard", "sword_and_shield_death"))),
}

OLD_ORC = "/Game/Low_Poly_Orc_Warrior/Animations/Orc_Warrior/"

if "--old" in sys.argv:
    SETUPS = {"Orc": (SETUPS["Orc"][0], dict(
        Attacks=[{"animation": obj(OLD_ORC + "AS_07_Orc_Warrior_Attack1"), "hitTime": 0.45, "sourceBone": "hand_r", "playRate": 1.0},
                 {"animation": obj(OLD_ORC + "AS_08_Orc_Warrior_Attack2"), "hitTime": 0.5, "sourceBone": "hand_r", "playRate": 1.0}],
        HitAnimations=[obj(OLD_ORC + "AS_09_Orc_Warrior_Hit")],
        DeathAnimation=obj(OLD_ORC + "AS_10_Orc_Warrior_Death")))}

for target, (bps, values) in SETUPS.items():
    for name in bps:
        path = BP + name
        cdo = {"refPath": path + ".Default__" + name + "_C"}
        # the array tool refuses a resize that also changes elements: empty the arrays first, then fill them
        ue.props(cdo, Attacks=[], HitAnimations=[])
        ue.props(cdo, **values)
        ue.call("editor_toolset.toolsets.blueprint.BlueprintTools", "compile_blueprint", blueprint={"refPath": obj(path)})
        ue.call(ue.AS, "save_assets", asset_paths=[path])
        got = ue.get_props(cdo, "Attacks", "DeathAnimation")
        print(name, len(got["Attacks"]), "attacks, death", str(got["DeathAnimation"])[-60:])
