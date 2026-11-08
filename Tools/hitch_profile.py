"""Measures frame times along the level in PIE with the editor running at full speed: average, p95 and worst frame per zone.
Hitches show as a worst frame far above the p95. Usage: python hitch_profile.py [--no-companion]"""
import sys, time
import ue
import cmdlib as C
from pie_lib import pie, teleport

PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
ue.props(PERF, bThrottleCPUWhenNotForeground=False)

from pie_lib import start_play
start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
hero = pie("AmnesiaCharacter")[0]
print("god:", C.cmd(["God 1"])["outputs"])
if "--no-companion" in sys.argv:
    for c in pie("AmnesiaCompanion"):
        ue.call(ue.A, "set_actor_transform", actor=c, xform=ue.xform(0, 0, -90000, 0, 0, 0, 1, 1, 1))

print("warming up shaders (40 s)...")
for x in (2000, 15000, 27000, 37000, 48000, 59000):
    teleport(pie("AmnesiaCharacter")[0], x)
    time.sleep(6)

for name, x in (("forest", 3000), ("village", 15000), ("snow", 26000), ("ruins", 36000), ("ruins city", 39500), ("cave", 47500), ("storm", 59000)):
    teleport(pie("AmnesiaCharacter")[0], x)
    time.sleep(3)
    r = C.cmd([], sample=6.0, wait=0.5)
    f = r.get("frame_ms", {})
    print("%-11s fps %5.1f  frame avg %5.1f p95 %5.1f max %6.1f | game %5.1f render %5.1f gpu %5.1f" % (
        name, r.get("fps", 0), f.get("avg", 0), f.get("p95", 0), f.get("max", 0),
        r["game_ms"]["avg"], r["render_ms"]["avg"], r["gpu_ms"]["avg"]))

ue.call(ue.E, "StopPIE")
ue.props(PERF, bThrottleCPUWhenNotForeground=True)

