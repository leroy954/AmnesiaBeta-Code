"""Dumps the game thread stat averages and the list of ticking actors in PIE, to find what costs time."""
import time, re, collections
import ue
import cmdlib as C
from pie_lib import pie, teleport, live_hero

LOG = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\Logs\AmnesiaBeta.log"
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
ue.props(PERF, bThrottleCPUWhenNotForeground=False)

if ue.pie_on():
    ue.call(ue.E, "StopPIE")
    time.sleep(3)
ue.call(ue.E, "StartPIE", options={"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 6})
time.sleep(2)
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
teleport(live_hero(), 3000)
time.sleep(25)

start = sum(1 for _ in open(LOG, encoding="utf-8", errors="ignore"))
C.cmd(["stat game", "stat dumpave -num=120 -ms=0.25", "dumpticks"], wait=6.0)
time.sleep(8)
C.cmd(["stat none"])
lines = open(LOG, encoding="utf-8", errors="ignore").read().splitlines()[start:]
stats = [l for l in lines if "LogStats" in l and "ms" in l]
print("\n".join(re.sub(r"^\[.*?\]\[.*?\]", "", l)[:180] for l in stats[:70]))

# ticking actors grouped by class
ticks = collections.Counter()
for l in lines:
    m = re.search(r"LogEngine: \s*(\S+)\s.*\[(\w+)\]", l)
    if "Tick" in l and "LogEngine" in l:
        name = re.sub(r"_C?_?\d+.*", "", l.split("LogEngine:")[-1].strip().split(" ")[0])
        ticks[name] += 1
print("ticking:", ticks.most_common(25))
ue.call(ue.E, "StopPIE")
ue.props(PERF, bThrottleCPUWhenNotForeground=True)
