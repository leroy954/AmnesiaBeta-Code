"""PIE test: opens the admin panel for a picture, then scans the whole path for things that block the hero."""
import time, collections
import ue


def pie(name):
    return [a for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag="") if "UEDPIE" in a["refPath"]]


if ue.pie_on():
    ue.call(ue.E, "StopPIE")
    time.sleep(3)
ue.call(ue.E, "StartPIE", options={"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 5})
hud = pie("HUD")[0]
ue.props(hud, bDebugCloseMenu=True)
time.sleep(2)
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(1.0)
ue.shot("shots/admin.png")
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(0.5)

r = ue.job([{"op": "path_scan", "x0": -1900, "x1": 64300, "step": 20}], world="pie", timeout=600)
print(r["errors"])
rows = [l.split("|") for l in r["report"]]
print("blockers:", len(rows))
by = collections.defaultdict(list)
for x, floor, label, cls, comp, mesh, vis, profile, dec in rows:
    by[(label.rstrip("0123456789_"), cls, mesh, vis, profile, dec)].append(int(float(x)))
for k, xs in sorted(by.items(), key=lambda kv: kv[1][0]):
    print(len(xs), k, xs[:8])
