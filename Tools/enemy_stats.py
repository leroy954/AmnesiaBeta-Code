"""Lists every placed enemy with level, HP, damage and position, plus the hero's and the dragon's damage numbers."""
import ue

rows = []
for a in ue.call(ue.S, "find_actors", name="", collision_channels=[], tag=""):
    p = a["refPath"]
    if "UEDPIE" in p or ("BP_" not in p.split(".")[-1] and "Ent_" not in p):
        continue
    last = p.split(".")[-1]
    if not any(k in last for k in ("Ent", "Skeleton", "Golem", "Oak", "Enemy")) or "Giant" in last:
        continue
    try:
        pr = ue.get_props(a, ["EnemyLevel", "MaxHP", "MeleeDamage", "SizeScale", "XPReward"])
        x = ue.call(ue.A, "get_actor_transform", actor=a)["location"]["x"]
    except Exception as e:
        continue
    rows.append((x, last[:28], pr))
for x, n, pr in sorted(rows):
    print("%7.0f %-28s lvl %s hp %s dmg %s size %s xp %s" % (x, n, pr.get("EnemyLevel"), pr.get("MaxHP"), pr.get("MeleeDamage"), pr.get("SizeScale"), pr.get("XPReward")))
print(len(rows), "enemies")
H = {"refPath": "/Game/Amnesia/Blueprints/BP_AmnesiaCharacter.Default__BP_AmnesiaCharacter_C"}
for k in ("MaxHP", "MeleeDamage", "AirSlashDamage", "HPPerLevel", "DamageBonusPerLevel"):
    try:
        print("hero", k, ue.get_props(H, [k]))
    except Exception as e:
        print("hero", k, "ERR")
