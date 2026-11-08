"""Starts PIE, teleports the hero along the level and takes screenshots.
Usage: python tour.py [--enemies] [--wait N] x1 x2 ...   (enemies are removed from the PIE world unless --enemies)"""
import sys, time
import ue
import levelgen as lg

args = sys.argv[1:]
keep_enemies = "--enemies" in args
wait = 9.0
if "--wait" in args:
    wait = float(args[args.index("--wait") + 1])
xs = []
skip = False
for i, t in enumerate(args):
    if skip:
        skip = False
        continue
    if t == "--wait":
        skip = True
        continue
    if t.startswith("--"):
        continue
    xs.append(float(t))

if not ue.pie_on():
    ue.call(ue.E, "StartPIE", options={})
    time.sleep(10)


def hero():
    for a in ue.call(ue.S, "find_actors", name="AmnesiaCharacter", collision_channels=[], tag=""):
        if "UEDPIE" in a["refPath"]:
            return a
    return None


def get_hero():
    a = hero()
    for _ in range(20):
        if a:
            return a
        time.sleep(2)
        a = hero()
    return a


a = get_hero()
print("hero", a["refPath"][-40:] if a else None)
if not keep_enemies:
    n = 0
    for e in ue.call(ue.S, "find_actors", name="Ent_", collision_channels=[], tag=""):
        if "UEDPIE" in e["refPath"]:
            ue.call(ue.S, "remove_from_scene", actor=e)
            n += 1
    print("removed enemies in PIE:", n)

for x in xs:
    a = get_hero()
    z = lg.h(x) + 220
    ue.call(ue.A, "set_actor_transform", actor=a, xform=ue.xform(x, 0, z, 0, 0, 0, 1, 1, 1))
    time.sleep(wait)
    a = get_hero()
    print("at", ue.call(ue.A, "get_actor_transform", actor=a)["location"])
    ue.shot("tour_%d.png" % int(x))
    print("shot", int(x))
