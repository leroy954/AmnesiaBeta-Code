"""Deletes the files of outliner folders (ActorFolder objects in __ExternalObjects__) that hang under the folders with the given labels, for folders whose
actors are gone but whose files stay and show up as empty folders. Only run it while the editor is closed (rebuild.ps1 -AfterClose does that).
Usage: python remove_folder_files.py [--dry] <label> [<label> ...]"""
import os
import sys

import stale_folders as SF

args = [a for a in sys.argv[1:] if a != "--dry"]
dry = "--dry" in sys.argv[1:]
files = SF.load()
parent, children = SF.tree(files)
doomed = []

for label in args:
    roots = [p for p, v in files.items() if v["label"] == label and p not in parent]

    if len(roots) != 1:
        raise SystemExit("expected one root folder called %s, found %d: nothing deleted" % (label, len(roots)))

    stack = [roots[0]]

    while stack:
        p = stack.pop()
        doomed.append(p)
        stack.extend(children.get(p, []))

print(len(doomed), "folder files under", ", ".join(args))

for p in doomed:
    if not dry:
        os.remove(p)

print("listed only" if dry else "deleted")
