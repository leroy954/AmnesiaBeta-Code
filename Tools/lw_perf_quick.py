"""Three quick frame time measurements in the long world (the village, the snow forest, the storm) to compare experiments."""
import time
import ue
import cmdlib as C
from pie_lib import pie, start_play, teleport, live_hero

PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
ue.props(PERF, bThrottleCPUWhenNotForeground=False)
start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
for name, x in (("base forest", 3000), ("Z2 hamlet", 270000), ("Z3 snow", 395000), ("Z6 storm", 730000)):
    teleport(live_hero(), x)
    time.sleep(8)
    r = C.cmd([], sample=6.0, wait=0.5)
    f = r.get("frame_ms", {})
    print("%-10s fps %5.1f  frame avg %5.1f p95 %5.1f max %6.1f | game %5.1f render %5.1f gpu %5.1f" % (
        name, r.get("fps", 0), f.get("avg", 0), f.get("p95", 0), f.get("max", 0), r["game_ms"]["avg"], r["render_ms"]["avg"], r["gpu_ms"]["avg"]))
print("actors:", len(ue.call(ue.S, "find_actors", name="", collision_channels=[], tag="")) if False else "")
ue.call(ue.E, "StopPIE")
ue.props(PERF, bThrottleCPUWhenNotForeground=True)
