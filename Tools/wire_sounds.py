"""Hooks the imported sounds into the hero, the enemy blueprints, the zones, the weather and the checkpoints."""
import ue

BP = "/Game/Amnesia/Blueprints/"


def wave(folder, name):
    return "/Script/Engine.SoundWave'/Game/Audio/%s/%s.%s'" % (folder, name, name)


def takes(folder, base, numbers):
    return [wave(folder, "SW_%s_%02d" % (base, i)) for i in numbers]


def sset(sounds, vol=1.0, pmin=0.95, pmax=1.05, rng=0.0):
    return "(Sounds=(%s),Volume=%.3f,PitchMin=%.3f,PitchMax=%.3f,Range=%.1f)" % (",".join(sounds), vol, pmin, pmax, rng)


R = range(1, 13)
GRASS = takes("Footsteps", "Step_Grass", R)
DIRT = takes("Footsteps", "Step_Dirt", range(1, 10))
SNOW = takes("Footsteps", "Step_Snow", range(1, 7))
STONE = takes("Footsteps", "Step_Stone", R)
MUD = takes("Footsteps", "Step_Mud", R)
MONSTER = takes("Footsteps", "Step_Monster", range(1, 7))

ops = []

# hero
ops.append({"op": "set_object_props", "object": BP + "BP_AmnesiaCharacter.BP_AmnesiaCharacter_C", "props": {
    "SwingSounds": sset(takes("Hero", "Hero_Swing", (1, 2, 3)), 0.9, 0.93, 1.07),
    "HitSounds": sset(takes("Hero", "Hero_HitThud", (1, 2, 3, 4)), 0.5, 0.9, 1.1),
    "HurtSounds": sset(takes("Hero", "Hero_Hurt", (1, 2, 3, 4, 5)), 0.8, 0.95, 1.08),
    "DeathSounds": sset(takes("Hero", "Hero_Death", (1, 2)), 0.9, 0.97, 1.03),
    "FootstepSounds": sset(GRASS, 0.4, 0.9, 1.1),
    "JumpSounds": sset(takes("Hero", "Hero_Jump", (1,)), 0.45, 0.9, 1.1),
    "LandSounds": sset(takes("Hero", "Hero_Land", (1, 2)), 0.55, 0.92, 1.08),
    "LevelUpSounds": sset(takes("Hero", "Hero_LevelUp", (1,)), 0.9, 1.0, 1.0),
    "HealSounds": sset(takes("Hero", "Orb_Pickup", (1, 2, 3)), 0.6, 0.95, 1.08),
}})

# enemies, by kind. Sounds fade out with the distance to the camera
ENT = {
    "HitSounds": sset(takes("Enemy", "Impact_Wood", range(1, 7)), 0.85, 0.9, 1.08, 3500),
    "HurtSounds": sset(takes("Enemy", "Ent_Grunt", range(1, 6)), 0.65, 0.78, 0.92, 3500),
    "DeathSounds": sset(takes("Enemy", "Ent_Death", (1, 2)), 0.9, 0.82, 0.92, 4000),
    "DeathFallSounds": sset(takes("Enemy", "BodyFall_Heavy", (1, 2)), 0.8, 0.9, 1.0, 4000),
    "DeathFallDelay": 0.9,
    "AttackSounds": sset(takes("Enemy", "Ent_Attack", (1, 2, 3)), 0.7, 0.82, 0.95, 3500),
    "FootstepSounds": sset(MONSTER, 0.28, 0.85, 1.0, 2500),
    "FootstepInterval": 0.7,
}
SKELETON = {
    "HitSounds": sset(takes("Enemy", "Impact_Bone", range(1, 7)), 0.8, 0.92, 1.1, 3500),
    "HurtSounds": sset(takes("Enemy", "Skeleton_Hurt", range(1, 6)), 0.5, 0.9, 1.1, 3500),
    "DeathSounds": sset(takes("Enemy", "Skeleton_Death", (1, 2)), 0.8, 0.95, 1.05, 4000),
    "DeathFallSounds": sset(takes("Enemy", "Skeleton_Fall", (1, 2)), 0.8, 0.95, 1.05, 4000),
    "DeathFallDelay": 0.6,
    "AttackSounds": sset(takes("Enemy", "Skeleton_Attack", (1, 2, 3, 4)), 0.6, 0.92, 1.08, 3500),
}
GOLEM = {
    "HitSounds": sset(takes("Enemy", "Impact_Metal", range(1, 7)), 0.85, 0.9, 1.05, 3500),
    "HurtSounds": sset(takes("Enemy", "Golem_Hurt", (1, 2, 3)), 0.7, 0.8, 0.92, 3500),
    "DeathSounds": sset(takes("Enemy", "Golem_Death", (1,)), 0.9, 0.85, 0.95, 4500),
    "DeathFallSounds": sset(takes("Enemy", "Golem_Fall", (1,)) + takes("Enemy", "BodyFall_Heavy", (1,)), 0.9, 0.85, 0.95, 4500),
    "DeathFallDelay": 1.0,
    "AttackSounds": sset(takes("Enemy", "Golem_Attack", (1, 2, 3)), 0.8, 0.82, 0.95, 3500),
    "FootstepSounds": sset(MONSTER, 0.45, 0.72, 0.85, 3000),
    "FootstepInterval": 0.8,
}
for name, props in (("BP_AmnesiaEnemy", ENT), ("BP_Ent_Ash", ENT), ("BP_Ent_Birch", ENT), ("BP_Ent_Spruce", ENT),
                    ("BP_Skeleton_Guard", SKELETON), ("BP_Armored_Golem", GOLEM), ("BP_Stone_Golem", GOLEM)):
    ops.append({"op": "set_object_props", "object": BP + "%s.%s_C" % (name, name), "props": props})

# zones: ambience, an extra layer and the ground under the hero's feet
AMB = lambda n: wave("Ambience", "SW_Amb_" + n)
ZONES = {
    "MoodZone_Zone1_Forest": (AMB("Forest_Day"), AMB("River"), 0.55, GRASS),
    "MoodZone_Zone2_Village": (AMB("Village"), "None", 0.6, DIRT),
    "MoodZone_Zone3_Snow": (AMB("Forest_Quiet"), "None", 0.6, SNOW),
    "MoodZone_Zone4_Ruins": (AMB("Forest_Deep"), AMB("Crickets"), 0.35, STONE),
    "MoodZone_Zone5_Cave": (AMB("Cave_Drips"), AMB("Wind"), 0.3, STONE),
    "MoodZone_Zone6_Storm": (AMB("Rain_Heavy"), AMB("Wind"), 0.5, MUD),
}
for label, (amb, layer, layer_vol, steps) in ZONES.items():
    ops.append({"op": "set_props", "label": label, "props": {
        "AmbientSound": amb, "AmbientLayerSound": layer, "AmbientLayerVolume": layer_vol,
        "FootstepSounds": sset(steps, 0.4, 0.9, 1.1)}})

# weather
ops.append({"op": "set_props", "label": "Weather", "props": {
    "RainLoopSound": AMB("Rain_Forest"), "WindLoopSound": AMB("Wind"),
    "ThunderSounds": sset(takes("World", "Thunder", (1, 2, 3)), 1.0, 0.8, 1.1)}})

# rest places
for i in range(11):
    ops.append({"op": "set_props", "label": "Checkpoint_%02d" % i, "props": {
        "LightSounds": sset(takes("World", "Fire_Ignite", (1, 2)), 0.8, 0.95, 1.05, 4000)}})

# the enemies already placed in the level keep their own copies, so set them too
for a in ue.call(ue.S, "find_actors", name="Ent_", collision_channels=[], tag=""):
    label = ue.call(ue.A, "get_label", actor=a)
    kind = GOLEM if "Golem" in label else SKELETON if "Skeleton" in label else ENT
    ops.append({"op": "set_props", "label": label, "props": kind})

ops.append({"op": "save"})
print(ue.job(ops))

# check a few values made it
hero = {"refPath": BP + "BP_AmnesiaCharacter.Default__BP_AmnesiaCharacter_C"}
print("hero swings:", ue.get_props(hero, "SwingSounds"))
for a in ue.call(ue.S, "find_actors", name="Weather", collision_channels=[], tag=""):
    print(ue.call(ue.A, "get_label", actor=a), ue.get_props(a, "RainLoopSound", "ThunderSounds"))
