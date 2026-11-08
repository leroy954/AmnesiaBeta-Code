"""Gives the Orc Warrior its own sounds: battle roars at the start of a swing, growls when it is hurt, a roar when it dies, the body falling, steps, and metal
for the sword on its armour. The roars are from the MonsterRoarsAndGrowls pack. Sets the Blueprint and the orcs already placed in the level (they keep
their own copies of the sets). Usage: python orc_sounds.py"""
import ue

BPS = ["/Game/Amnesia/Blueprints/BP_Orc_Warrior" + suffix for suffix in ("", "_2", "_3")]


def roar(name):
    return "/Script/Engine.SoundWave'/Game/MonsterRoarsAndGrowls/wav/%s.%s'" % (name, name)


def enemy(folder, base, numbers):
    return ["/Script/Engine.SoundWave'/Game/Audio/%s/SW_%s_%02d.SW_%s_%02d'" % (folder, base, i, base, i) for i in numbers]


def sset(sounds, vol=1.0, pmin=0.95, pmax=1.05, rng=0.0):
    return "(Sounds=(%s),Volume=%.3f,PitchMin=%.3f,PitchMax=%.3f,Range=%.1f)" % (",".join(sounds), vol, pmin, pmax, rng)


ORC = {
    # the sword on a shield and armour
    "HitSounds": sset(enemy("Enemy", "Impact_Metal", range(1, 7)), 0.7, 0.9, 1.05, 3500),
    # short growls, and not on every hit: they last about two seconds
    "HurtSounds": sset([roar("25_Warning_Growl"), roar("12_Ambush_Growl"), roar("19_Intimidation_Growl")], 0.6, 1.0, 1.15, 3500),
    "HurtSoundChance": 0.3,
    "DeathSounds": sset([roar("02_Ferocious_Roar"), roar("09_Mythic_Roar"), roar("03_Defiant_Roar")], 0.85, 0.95, 1.05, 4000),
    "DeathFallSounds": sset(enemy("Enemy", "BodyFall_Heavy", (1, 2)), 0.8, 0.9, 1.0, 4000),
    "DeathFallDelay": 1.2,
    "AttackSounds": sset([roar("10_Attack_Roar"), roar("06_Battle_Roar"), roar("01_Primal_Roar"), roar("04_Signal_Roar")], 0.75, 0.95, 1.1, 3500),
    "FootstepSounds": sset(enemy("Footsteps", "Step_Monster", range(1, 7)), 0.4, 0.9, 1.05, 3000),
    "FootstepInterval": 0.55,
}

ops = [{"op": "set_object_props", "object": "%s.%s_C" % (bp, bp.rsplit("/", 1)[1]), "props": ORC} for bp in BPS if ue.call(ue.AS, "exists", path=bp)]
for a in ue.call(ue.S, "find_actors", name="Orc", collision_channels=[], tag=""):
    if "UEDPIE" in a["refPath"] or "BP_Orc_Warrior" not in a["refPath"]:
        continue
    ops.append({"op": "set_props", "label": ue.call(ue.A, "get_label", actor=a), "props": ORC})
ops.append({"op": "save"})
print(ue.job(ops))

cdo = {"refPath": BPS[0] + ".Default__BP_Orc_Warrior_C"}
print(ue.get_props(cdo, "HurtSoundChance", "DeathFallDelay", "FootstepInterval"))
print(str(ue.get_props(cdo, "AttackSounds"))[:400])
