"""Photographs the heavy attack special of a weapon in a running game: two enemies stand in front of the hero, the heavy attack starts, and eight pictures follow
each other as fast as the capture goes. One contact sheet is saved.
Usage: python specials_look.py <out png> <weapon id> [gap between the enemies, default 230]"""
import os
import subprocess
import sys
import time

from PIL import Image

import ue
import pie_lib as P

OUT = sys.argv[1]
WEAPON = sys.argv[2]
GAP = float(sys.argv[3]) if len(sys.argv) > 3 else 230.0


def at(actor):
    return ue.call(ue.A, "get_actor_transform", actor=actor)["location"]


subprocess.run([sys.executable, "close_msglog.py"])
if not P.start_play():
    raise SystemExit("spectator")
ue.props(P.pie("HUD")[0], bDebugCloseMenu=True)
time.sleep(4)
hero = P.live_hero()
skills = [c for c in ue.call(ue.A, "get_components", actor=hero) if c["refPath"].endswith(".Skills")][0]
ue.props(skills, bDebugLearnAll=True)
time.sleep(1.5)
cands = sorted([e for e in P.pie("Ent_") if 2000 < at(e)["x"] < 20500], key=lambda e: at(e)["x"])
target, neighbor = cands[2], cands[3]
t = at(target)
ue.call(ue.A, "set_actor_transform", actor=neighbor, xform=ue.xform(t["x"] + GAP, 0, t["z"] + 10, 0, 0, 0, 1, 1, 1))
ue.props(hero, DebugWeapon=WEAPON)
time.sleep(1.8)
ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(t["x"] - 200, 0, t["z"] + 60, 0, 0, 0, 1, 1, 1))
time.sleep(2.0)
ue.props(hero, bDebugHeavySlash=True)
tiles = []
for i in range(8):
    path = ue.shot(os.path.join(os.path.dirname(OUT), "_tmp_special.png"))
    im = Image.open(path).convert("RGB")
    w, h = im.size
    tiles.append(im.crop((int(w * 0.139), int(h * 0.133), int(w * 0.781), int(h * 0.963))).crop((60, 120, 760, 600)))
ue.call(ue.E, "StopPIE")
cols = 4
tw, th = tiles[0].size
sheet = Image.new("RGB", (tw * cols, th * 2))
for i, tile in enumerate(tiles):
    sheet.paste(tile, ((i % cols) * tw, (i // cols) * th))
sheet.save(OUT)
os.remove(os.path.join(os.path.dirname(OUT), "_tmp_special.png"))
print("saved", OUT, sheet.size)
