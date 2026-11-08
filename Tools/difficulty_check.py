"""In a running game: prints the enemies' real HP, damage, attack cooldown, stun and chase speed (after the difficulty scaling) and the dragon's fireball numbers."""
import time
import ue
from pie_lib import pie, start_play

start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
rows = []
for e in pie("BP_"):
    last = e["refPath"].split(".")[-1]
    if not any(k in last for k in ("Ent_", "Skeleton", "Armored_Golem", "AmnesiaEnemy")):
        continue
    try:
        pr = ue.get_props(e, ["EnemyLevel", "MaxHP", "MeleeDamage", "AttackCooldown", "HitStunTime", "ChaseSpeed", "XPReward"])
        x = ue.call(ue.A, "get_actor_transform", actor=e)["location"]["x"]
    except Exception:
        continue
    rows.append((x, last[:22], pr))
for x, n, pr in sorted(rows)[::3]:
    print("%6.0f %-22s lvl %s hp %.1f dmg %.2f cd %.2f stun %.2f chase %.0f xp %s" % (x, n, pr["EnemyLevel"], pr["MaxHP"], pr["MeleeDamage"], pr["AttackCooldown"], pr["HitStunTime"], pr["ChaseSpeed"], pr["XPReward"]))
print(len(rows), "enemies")
for c in pie("AmnesiaCompanion"):
    print("dragon", ue.get_props(c, ["FireCooldown", "FireballDamageScale", "FireRange"]))
ue.call(ue.E, "StopPIE")
