"""Prints the hero's ground slash setup from the Blueprint defaults."""
import ue
CDO = {"refPath": "/Game/Amnesia/Blueprints/BP_AmnesiaCharacter.Default__BP_AmnesiaCharacter_C"}
for k in ("GroundSlashes", "GroundSlashAnimation", "SlashChainWindow", "SlashComboResetTime", "BaseWalkSpeed"):
    try:
        print(k, ue.get_props(CDO, [k]))
    except Exception as e:
        print(k, "ERR", e)
