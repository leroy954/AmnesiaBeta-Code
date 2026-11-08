"""Films the hero swinging while running and while standing, to check the legs keep running."""
import time, base64, io
import ue
from PIL import Image, ImageDraw


def pie(name):
    return [a for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag="") if "UEDPIE" in a["refPath"]]


def grab():
    r = ue.call(ue.E, "CaptureEditorImage")
    img = r["image"] if "image" in r else r
    im = Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")
    w, h = im.size
    return im.crop((int(w * 0.32), int(h * 0.42), int(w * 0.62), int(h * 0.97))).resize((300, 300))


if ue.pie_on():
    ue.call(ue.E, "StopPIE")
    time.sleep(3)
ue.call(ue.E, "StartPIE", options={"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": 5})
ue.props(pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(2.5)
hero = pie("AmnesiaCharacter")[0]
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(-1300, 0, 120, 0, 0, 0, 1, 1, 1))
ue.props({"refPath": hero["refPath"] + ".CameraBoom"}, TargetArmLength=380.0)
time.sleep(1.0)
move = {"refPath": hero["refPath"] + ".CharMoveComp"}
frames = []
for i in range(8):
    ue.props(move, Velocity={"x": 600.0, "y": 0.0, "z": 0.0})
    if i in (1, 4):
        ue.props(hero, DebugSlashStartDurationRate={"x": 0.0, "y": 0.5, "z": 1.5}, bDebugSlash=True)
    frames.append(("run %d" % i, grab()))
time.sleep(1.5)
for i in range(4):
    if i == 0:
        ue.props(hero, DebugSlashStartDurationRate={"x": 0.0, "y": 0.5, "z": 1.5}, bDebugSlash=True)
    frames.append(("stand %d" % i, grab()))

cols = 6
sheet = Image.new("RGB", (300 * cols, 300 * 2))
d = ImageDraw.Draw(sheet)
for i, (label, im) in enumerate(frames):
    sheet.paste(im, ((i % cols) * 300, (i // cols) * 300))
    d.text(((i % cols) * 300 + 4, (i // cols) * 300 + 4), label, fill=(255, 255, 0))
sheet.save("shots/run_slash.png")
print("saved")


