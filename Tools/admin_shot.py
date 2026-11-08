"""Opens the admin panel in a running game and captures it."""
import time, base64, io
import ue
from PIL import Image
from pie_lib import pie, start_play

start_play()
hud = pie("HUD")[0]
ue.props(hud, bDebugCloseMenu=True)
time.sleep(3)
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(2)
r = ue.call(ue.E, "CaptureEditorImage")
img = r["image"] if "image" in r else r
im = Image.open(io.BytesIO(base64.b64decode(img["data"]))).convert("RGB")
w, h = im.size
im.crop((int(w * 0.14), int(h * 0.1), int(w * 0.8), int(h * 0.98))).save("shots/admin_long.png")
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(1)
ue.call(ue.E, "StopPIE")
print("saved")
