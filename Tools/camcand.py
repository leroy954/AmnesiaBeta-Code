"""Renders candidate main menu camera views into one sheet. Usage: camcand.py out.png "x,y,z,pitch,yaw" ..."""
import sys, time, base64, io
import ue
from PIL import Image, ImageDraw

if ue.pie_on():
    ue.call(ue.E, "StopPIE")
    time.sleep(3)

out = sys.argv[1]
cands = [tuple(float(v) for v in a.split(",")) for a in sys.argv[2:]]
W, H = 640, 360
cols = 2
sheet = Image.new("RGB", (W * cols, H * ((len(cands) + cols - 1) // cols)))
d = ImageDraw.Draw(sheet)
for i, (x, y, z, p, yaw) in enumerate(cands):
    r = ue.call(ue.E, "CaptureViewport", captureTransform=ue.xform(x, y, z, p, yaw, 0),
                annotations={"gridSpacing": 0, "gridExtent": 0, "gridHeight": 0, "maxLabelDistance": 0, "classFilter": None, "maxLabels": 0}, bShowUI=False)
    im = Image.open(io.BytesIO(base64.b64decode(r["image"]["data"]))).convert("RGB").resize((W, H))
    sheet.paste(im, ((i % cols) * W, (i // cols) * H))
    d.text(((i % cols) * W + 6, (i // cols) * H + 4), "%d: %d,%d,%d p%d y%d" % (i, x, y, z, p, yaw), fill=(255, 255, 0))
sheet.save(out)
print("saved", out)
