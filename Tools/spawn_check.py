"""Sets the hero's anim class (old or new) and checks whether PIE spawns the hero. Usage: spawn_check.py old|new"""
import sys, time
import ue

CLASSES = {"old": "/Game/Variant_Combat/Anims/ABP_Manny_Combat.ABP_Manny_Combat_C",
           "new": "/Game/Amnesia/Animation/ABP_Arden.ABP_Arden_C"}
mesh = "/Game/Amnesia/Blueprints/BP_AmnesiaCharacter.Default__BP_AmnesiaCharacter_C:CharacterMesh0"

if ue.pie_on():
    ue.call(ue.E, "StopPIE")
    time.sleep(3)
if sys.argv[1] in CLASSES:
    print(ue.job([{"op": "set_object_props", "object": mesh,
                   "props": {"AnimClass": "/Script/Engine.AnimBlueprintGeneratedClass'%s'" % CLASSES[sys.argv[1]]}}] + ([{"op": "save"}] if "--save" in sys.argv else [])))
ue.call(ue.E, "StartPIE", options={"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3})
print("characters", [a["refPath"][-40:] for a in ue.call(ue.S, "find_actors", name="Character", collision_channels=[], tag="") if "UEDPIE" in a["refPath"]])
print("controllers", [a["refPath"][-40:] for a in ue.call(ue.S, "find_actors", name="PlayerController", collision_channels=[], tag="") if "UEDPIE" in a["refPath"]])
ue.call(ue.E, "StopPIE")

