"""Finds floating decorations across the level (dry run by default) and prints them grouped by folder and label.
Usage: python floaters.py [--apply] [folder]"""
import sys, collections
import ue

APPLY = "--apply" in sys.argv
args = [a for a in sys.argv[1:] if not a.startswith("--")]
FOLDER = args[0] if args else "Gen"

# things that hang on purpose, sit on water, or stand under the ground line on purpose
SKIP_LABELS = ["Ivy", "MouthIvy", "River", "Water", "FG_", "Mountains", "Hills", "Cave_", "Stalactite", "Ceiling", "Hang", "Vine",
               # zone 4 pieces that rest on other pieces on purpose: pillars on bases, wall rows, cornices, lintel, ledge slabs
               "Pillar", "Wall", "Lintel", "Doorway", "Shrine", "Paving", "LedgeSlab", "LedgeSupport", "Altar", "CourtCircle",
               "Parapet", "Torch", "MouthRoots", "City", "Retaining", "PathBlock", "Platform_Crates", "Lintel", "Standing", "HillFace"]
SKIP_FOLDERS = ["Gen/Terrain", "Gen/Background", "Gen/Boundaries", "Gen/River", "Gen/Lights", "Gen/Moods", "Gen/Enemies", "Gen/Checkpoints", "Gen/Pickups", "Gen/Menu"]

r = ue.job([{"op": "snap", "folder": FOLDER, "dry": not APPLY, "skip": SKIP_LABELS, "skip_folders": SKIP_FOLDERS,
             "min_gap": 6, "max_drop": 3000}], timeout=600)
rows = [l.split("|") for l in r["report"] if "|" in l]
print([l for l in r["report"] if "|" not in l], "floating:", len(rows), "moved:", r["modified"], r["errors"])

groups = collections.defaultdict(list)
for folder, label, gap, x, height in rows:
    groups[(folder, label.rstrip("0123456789_"))].append((float(gap), float(x), float(height)))
for (folder, label), items in sorted(groups.items(), key=lambda kv: -len(kv[1])):
    gaps = sorted(g for g, _, _ in items)
    print("%-40s %-18s %4d  gap median %6.0f max %6.0f  x %d..%d" % (folder, label, len(items), gaps[len(gaps) // 2], gaps[-1],
                                                                 min(x for _, x, _ in items), max(x for _, x, _ in items)))




