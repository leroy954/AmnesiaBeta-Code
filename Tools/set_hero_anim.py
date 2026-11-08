"""Makes the copy of the combat anim blueprint a child of the hero overlay class and gives it to the hero."""
import ue

B = "editor_toolset.toolsets.blueprint.BlueprintTools"
bp = {"refPath": "/Game/Amnesia/Animation/ABP_Arden.ABP_Arden"}
print(ue.call(B, "set_parent", blueprint=bp, parent_class={"refPath": "/Script/AmnesiaBeta.AmnesiaHeroAnimInstance"}))
print(ue.call(B, "compile_blueprint", blueprint=bp))
print("parent:", ue.call(B, "get_parent", blueprint=bp))

mesh = "/Game/Amnesia/Blueprints/BP_AmnesiaCharacter.Default__BP_AmnesiaCharacter_C:CharacterMesh0"
print(ue.job([{"op": "set_object_props", "object": mesh,
               "props": {"AnimClass": "/Script/Engine.AnimBlueprintGeneratedClass'/Game/Amnesia/Animation/ABP_Arden.ABP_Arden_C'"}},
              {"op": "save"}]))
print(ue.get_props({"refPath": mesh}, "AnimClass"))
