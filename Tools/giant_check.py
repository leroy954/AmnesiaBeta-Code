"""Films the giant in a running game before and after setting its own transform again, to see whether a transform
update is what makes it show."""
import time, base64, io
import ue
import levelgen as lg
from PIL import Image, ImageDraw
from pie_lib import pie, start_play, live_hero, comps

start_play()
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(3)
hero = live_hero()
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(61500, 0, lg.h(61500) + 120, 0, 0, 0, 1, 1, 1))
g = pie("GiantGolem")[0]
mesh = [c for c in comps(g, "Mesh") if c["refPath"].endswith(".Mesh")][0]
W, H = 640, 400
sheet = Image.new("RGB", (W * 2, H * 2))
d = ImageDraw.Draw(sheet)


def shot(i, label):
    time.sleep(3)
    r = ue.call(ue.E, "CaptureEditorImage")
    img = r["image"] if "image" in r else r
    im = Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")
    w, h = im.size
    im = im.crop((int(w * 0.14), int(h * 0.15), int(w * 0.78), int(h * 0.96))).resize((W, H))
    sheet.paste(im, ((i % 2) * W, (i // 2) * H))
    d.text(((i % 2) * W + 6, (i // 2) * H + 4), label, fill=(255, 255, 0))


shot(0, "as started")
print("mesh", ue.get_props(mesh, ["bVisible", "bHiddenInGame", "BoundsScale", "bComponentUseFixedSkelBounds", "VisibilityBasedAnimTickOption"]))
ue.props(mesh, BoundsScale=3.0)
shot(1, "bounds scale 3")
t = ue.call(ue.A, "get_actor_transform", actor=g)
l, r_ = t["location"], t["rotation"]
ue.call(ue.A, "set_actor_transform", actor=g, xform=ue.xform(l["x"], l["y"], l["z"], r_["pitch"], r_["yaw"], r_["roll"], 35, 35, 35))
shot(2, "same transform set again")
sheet.save("shots/giant_refresh.png")
ue.call(ue.E, "StopPIE")
print("saved")
