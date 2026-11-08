"""Mood volumes of the long world: one box per zone, with the moods, ambience, footsteps and camera of the matching zone of the first level."""
import ue
import lw


def wave(folder, name):
    return "/Script/Engine.SoundWave'/Game/Audio/%s/%s.%s'" % (folder, name, name)


def takes(folder, base, numbers):
    return [wave(folder, "SW_%s_%02d" % (base, i)) for i in numbers]


def sset(sounds, vol=1.0, pmin=0.95, pmax=1.05, rng=0.0):
    return "(Sounds=(%s),Volume=%.3f,PitchMin=%.3f,PitchMax=%.3f,Range=%.1f)" % (",".join(sounds), vol, pmin, pmax, rng)


GRASS = takes("Footsteps", "Step_Grass", range(1, 13))
DIRT = takes("Footsteps", "Step_Dirt", range(1, 10))
SNOW = takes("Footsteps", "Step_Snow", range(1, 7))
STONE = takes("Footsteps", "Step_Stone", range(1, 13))
MUD = takes("Footsteps", "Step_Mud", range(1, 13))


def amb(n):
    return wave("Ambience", "SW_Amb_" + n)


# key: ambience, extra layer, layer volume, footsteps, storm control, camera pitch
SETUP = {
    "Z1": (amb("Forest_Day"), amb("River"), 0.55, GRASS, -1, -8),
    "Z2": (amb("Village"), "None", 0.6, DIRT, -1, -8),
    "Z3": (amb("Forest_Quiet"), "None", 0.6, SNOW, -1, -8),
    "Z4": (amb("Forest_Deep"), amb("Crickets"), 0.35, STONE, -1, -8),
    "Z5": (amb("Cave_Drips"), amb("Wind"), 0.3, STONE, -1, -8),
    "Z6": (amb("Rain_Heavy"), amb("Wind"), 0.5, MUD, 1, 16),
}


def build():
    ops = [{"op": "delete", "folder": "LW/Moods"}]
    for z in lw.ZONES:
        a, layer, vol, steps, storm, pitch = SETUP[z.key]
        moods = "(" + ",".join('"%s"' % m for m in z.moods) + ")"
        ops.append({"op": "spawn_actor", "class": "/Script/AmnesiaBeta.AmnesiaMoodVolume", "loc": [(z.x0 + z.x1) / 2.0, 0, 1000],
                    "label": "LW_Mood_" + z.key, "folder": "LW/Moods",
                    "props": {"MoodsOnEnter": moods, "BlendTime": 5, "AmbientSound": a, "AmbientLayerSound": layer, "AmbientLayerVolume": vol,
                              "FootstepSounds": sset(steps, 0.4, 0.9, 1.1), "StormControl": storm, "bSetCameraPitch": True, "CameraPitch": pitch}})
    print(ue.job(ops))
    for z in lw.ZONES:
        for a in ue.call(ue.S, "find_actors", name="LW_Mood_" + z.key, collision_channels=[], tag=""):
            if "UEDPIE" in a["refPath"]:
                continue
            for c in ue.call(ue.A, "get_components", actor=a):
                if c["refPath"].endswith(".Box") or c["refPath"].endswith(":Box"):
                    ue.props(c, BoxExtent={"x": (z.x1 - z.x0) / 2.0, "y": 1500, "z": 5000})
    print("mood extents set")


if __name__ == "__main__":
    build()
