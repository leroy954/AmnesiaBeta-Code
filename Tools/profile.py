"""Profiles PIE with the cmd.json channel: baseline thread/GPU times, then one console variable change at a time.
Usage: python profile.py [x] [--gpu] [--sweep]"""
import sys, re, time
from pie_lib import *
import cmdlib as C

args = sys.argv[1:]
X = float([a for a in args if not a.startswith("--")][0]) if [a for a in args if not a.startswith("--")] else 30000


def getcv(name):
    r = C.cmd([name])
    m = re.search(r"=\s*\"?([^\s\"(]+)", r["outputs"][0].split("|", 2)[-1])
    return m.group(1) if m else None


def sample(seconds=4.0, wait=1.0, cmds=()):
    return C.cmd(cmds, sample=seconds, wait=wait)


hero = start_pie()
teleport(hero, X)
time.sleep(4)
print("warm-up:", warm_up())

base = sample(5)
print("BASELINE x=%d  %s" % (X, C.line(base)))

if "--gpu" in args:
    C.cmd(["r.ProfileGPU.ShowUI 0"])
    r = C.cmd(["ProfileGPU"])
    print(r["outputs"])
    time.sleep(2.5)
    print("ProfileGPU requested; see the log")

if "--sweep" in args:
    tests = [
        ("screen 50%", ["r.ScreenPercentage 50"], ["r.ScreenPercentage 100"]),
        ("static meshes off", ["ShowFlag.StaticMeshes 0"], ["ShowFlag.StaticMeshes 1"]),
        ("post process off", ["ShowFlag.PostProcessing 0"], ["ShowFlag.PostProcessing 1"]),
        ("lumen GI off", ["r.DynamicGlobalIlluminationMethod 0"], ["r.DynamicGlobalIlluminationMethod 1"]),
        ("reflections off", ["r.ReflectionMethod 0"], ["r.ReflectionMethod 1"]),
        ("VSM off", ["r.Shadow.Virtual.Enable 0"], ["r.Shadow.Virtual.Enable 1"]),
        ("clouds off", ["r.VolumetricCloud 0"], ["r.VolumetricCloud 1"]),
        ("vol fog off", ["r.VolumetricFog 0"], ["r.VolumetricFog 1"]),
        ("lights off", ["ShowFlag.DynamicShadows 0"], ["ShowFlag.DynamicShadows 1"]),
    ]
    for name, on, off in tests:
        r = sample(4, 1.5, on)
        print("%-18s %s" % (name, C.line(r)))
        C.cmd(off)
        time.sleep(1.0)
    print("%-18s %s" % ("baseline again", C.line(sample(4))))

ue.call(ue.E, "StopPIE")
