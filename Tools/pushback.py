"""Pushes tall decorations out of the hero's walking line (y 0). Decorations have no collision in the game, so anything standing in the
line makes the hero walk into it and vanish. Moves each listed actor back until its bounding box ends behind y -140.
Only the listed labels move: the zone 4 walls and hill faces stand in front on purpose.
Usage: python pushback.py"""
import ue

LABELS = [
    # the last zone (zone6.py, zone6_more.py)
    "DeadTree", "BigRock", "HillRock", "OldPillar", "Boulder", "Standing", "FallenStanding", "RuinWall", "RuinArch", "Obelisk", "PillarBase",
    "Pillar", "FallenPillar", "DryBush",
    # the cave (cave.py keeps its own rule, this is the safety net)
    "CaveFloor", "KinkMound", "GlowMushroom", "Statue", "StatueMound",
]
r = ue.job([{"op": "push_back", "labels": LABELS, "limit_y": -140, "front_y": 110, "min_height": 60}, {"op": "save"}])
print("moved:", r["modified"], r["errors"])
