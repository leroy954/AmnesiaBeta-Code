"""Logs which sounds play while the hero stands at a few places, to find what is heard from far away.
Usage: python sound_probe.py x1 x2 ...   (seconds per place: 25)"""
import sys, time, re, os
import ue
from pie_lib import pie, start_play, teleport, live_hero

LOG = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\Logs\AmnesiaBeta.log"
xs = [float(a) for a in sys.argv[1:]] or [3000.0, 20000.0, 48500.0]
start_play()
hud = pie("HUD")[0]
ue.props(hud, bDebugCloseMenu=True)
time.sleep(3)
ue.props(hud, bDebugLogSounds=True)
for x in xs:
    teleport(live_hero(), x)
    size = os.path.getsize(LOG)
    time.sleep(25)
    with open(LOG, "rb") as f:
        f.seek(size)
        text = f.read().decode("utf-8", "ignore")
    names = {}
    for m in re.finditer(r"Sound: (\S+)", text):
        names[m.group(1)] = names.get(m.group(1), 0) + 1
    print("x %6.0f:" % x, names or "silent")
ue.call(ue.E, "StopPIE")
