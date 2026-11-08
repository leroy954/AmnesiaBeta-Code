"""Starts PIE past the menu, teleports the hero along the level and saves game view shots into one sheet.
Usage: python pie_tour.py out.png x1 x2 ...   [--wait s]"""
import sys, time, base64, io
import ue
import levelgen as lg
from PIL import Image, ImageDraw

args = sys.argv[1:]
wait = 3.5
if "--wait" in args:
    wait = float(args[args.index("--wait") + 1])
    del args[args.index("--wait"):args.index("--wait") + 2]
args = [a for a in args if a != "--park"]
out, xs = args[0], [float(a) for a in args[1:]]


def pie(name):
    return [a for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag="") if "UEDPIE" in a["refPath"]]


from pie_lib import start_play
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
ue.props(PERF, bThrottleCPUWhenNotForeground=False)
start_play(warmup=5)
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(2.0)
if "--park" in sys.argv:
    # enemies out of the way, so the hero isn't killed between shots
    # "Golem" also matches the GiantGolem scenery, which stays
    for n, e in enumerate(pie("Ent_") + pie("Skeleton") + [g for g in pie("Golem") if "Giant" not in g["refPath"]]):
        try:
            ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
        except RuntimeError:
            pass

W, H = 760, 470
cols = 2
sheet = Image.new("RGB", (W * cols, H * ((len(xs) + cols - 1) // cols)))
d = ImageDraw.Draw(sheet)
for i, x in enumerate(xs):
    hero = pie("AmnesiaCharacter")[0]
    import lw
    zz = lw.h(x) if x >= lw.X_START - 2000 else lg.h(x)
    ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(x, 0, zz + 120, 0, 0, 0, 1, 1, 1))
    time.sleep(wait)
    r = ue.call(ue.E, "CaptureEditorImage")
    img = r["image"] if "image" in r else r
    im = Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")
    w, h = im.size
    # the game viewport inside the editor window
    im = im.crop((int(w * 0.14), int(h * 0.15), int(w * 0.78), int(h * 0.96))).resize((W, H))
    sheet.paste(im, ((i % cols) * W, (i // cols) * H))
    d.text(((i % cols) * W + 6, (i // cols) * H + 4), "x %d" % x, fill=(255, 255, 0))
sheet.save(out)
print("saved", out)

ue.call(ue.E, 'StopPIE')
ue.props(PERF, bThrottleCPUWhenNotForeground=True)
