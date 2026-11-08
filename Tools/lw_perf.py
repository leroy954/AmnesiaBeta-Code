"""Frame times along the long world in the running game, one spot per stretch: average, p95 and worst frame, game, render and GPU ms."""
import time
import ue
import cmdlib as C
from pie_lib import pie, start_play, teleport, live_hero

PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
ue.props(PERF, bThrottleCPUWhenNotForeground=False)
start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
SPOTS = [("Z1 forest", 155000), ("Z1 river", 175000), ("Z1 end", 255000), ("Z2 hamlet", 270000), ("Z2 market", 340000), ("Z3 snow", 395000),
         ("Z3 deep", 470000), ("Z4 ruins", 520000), ("Z4 temple", 560000), ("Z5 cave", 610000), ("Z5 deep", 670000), ("Z6 storm", 730000), ("Z6 giant", 765000), ("Z6 end", 815000)]
print("warming up...")
for _, x in SPOTS[::2]:
    teleport(live_hero(), x)
    time.sleep(5)
for name, x in SPOTS:
    teleport(live_hero(), x)
    time.sleep(3)
    r = C.cmd([], sample=6.0, wait=0.5)
    f = r.get("frame_ms", {})
    print("%-11s fps %5.1f  frame avg %5.1f p95 %5.1f max %6.1f | game %5.1f render %5.1f gpu %5.1f" % (
        name, r.get("fps", 0), f.get("avg", 0), f.get("p95", 0), f.get("max", 0), r["game_ms"]["avg"], r["render_ms"]["avg"], r["gpu_ms"]["avg"]))
ue.call(ue.E, "StopPIE")
ue.props(PERF, bThrottleCPUWhenNotForeground=True)
