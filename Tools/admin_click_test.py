"""Clicks the level buttons of the admin panel for real (through the Slate inspector of the MCP server, direct Slate events) and reads the level text
of the panel after each click. Expected: +5 -> 6, -1 -> 5, +5 -> 10, +5 -> 10 (the top), -5 -> 5, -5 -> 1, -5 -> 1 (the bottom), +1 -> 2.
A new game at the end keeps the level out of the saved progress. Usage: python admin_click_test.py"""
import re
import time

import ue
import pie_lib as P

SI = "SlateInspectorToolset.SlateInspectorToolset"


def snapshot():
    return ue.call(SI, "Snapshot", ref="", maxDepth=100)


def button(label):
    for line in snapshot().split("\n"):
        # the button that was clicked last carries more attributes (focus, hover) before its position
        m = re.search(r'button "%s".*?\[ref=(\w+)\]' % re.escape(label), line)
        if m:
            return m.group(1)
    raise RuntimeError("no button " + label)


def level_text():
    for line in snapshot().split("\n"):
        m = re.search(r'text "(\d+ / \d+)"', line)
        if m:
            return m.group(1)
    return None


P.start_pie(park_enemies=False)
time.sleep(4)
hud = P.pie("HUD")[0]
ue.props(hud, bDebugToggleAdmin=True)
time.sleep(2.0)
ue.call(SI, "Observe", ref="", maxDepth=100)
time.sleep(3.0)
print("start:", level_text())
ok = True

for label, expected in (("+5", "6 / 10"), ("-1", "5 / 10"), ("+5", "10 / 10"), ("+5", "10 / 10"), ("-5", "5 / 10"), ("-5", "1 / 10"), ("-5", "1 / 10"), ("+1", "2 / 10")):
    ue.call(SI, "Click", ref=button(label))
    time.sleep(0.6)
    got = level_text()
    flag = "ok" if got == expected else "WRONG"
    ok = ok and got == expected
    print("click %-3s -> %-8s expected %-8s %s" % (label, got, expected, flag))

print("ALL CLICKS OK" if ok else "SOME CLICKS WRONG")
ue.call(ue.E, "StopPIE")
time.sleep(3)
P.start_pie(park_enemies=False)
time.sleep(4)
ue.call(ue.E, "StopPIE")
print("progress reset by a new game")
