"""Finds the outliner folder objects (ActorFolder files in __ExternalObjects__) that hang under a folder of the given name, by reading the files: a file holds its
own guid in the object name (ActorFolder_UID_AAAAAAAA-BBBBBBBB-CCCCCCCC-DDDDDDDD) and the guid of its parent as 16 bytes, and its label as a length prefixed string.
Prints the tree and writes the list of the files to delete (the editor must be closed when they are deleted, it keeps the folders in memory).
Usage: python stale_folders.py <root folder label> [list.json to write]"""
import json
import os
import re
import struct
import sys

ROOT = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Content\__ExternalObjects__\Amnesia\Maps\Lvl_Forest01"
NAME = re.compile(rb"ActorFolder_UID_([0-9A-F]{8})-([0-9A-F]{8})-([0-9A-F]{8})-([0-9A-F]{8})")


def label_of(data):
    """The label: the last length prefixed ASCII string of the file."""
    found = None
    for i in range(len(data) - 5):
        n = struct.unpack_from("<i", data, i)[0]
        if 2 <= n <= 64 and i + 4 + n <= len(data) and data[i + 4 + n - 1] == 0:
            body = data[i + 4:i + 4 + n - 1]
            if all(32 <= b < 127 for b in body):
                found = body.decode()
    return found


def load():
    files = {}
    for dp, _, fn in os.walk(ROOT):
        for f in fn:
            path = os.path.join(dp, f)
            data = open(path, "rb").read()
            m = NAME.search(data)
            if not m:
                continue
            guid = struct.pack("<4I", *[int(g, 16) for g in m.groups()])
            files[path] = {"guid": guid, "data": data, "label": label_of(data)}
    return files


def tree(files):
    by_guid = {v["guid"]: p for p, v in files.items()}
    parent = {}
    for p, v in files.items():
        for g, q in by_guid.items():
            if q != p and g in v["data"]:
                parent[p] = q
                break
    children = {}
    for p, q in parent.items():
        children.setdefault(q, []).append(p)
    return parent, children


if __name__ == "__main__":
    root_label = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else None
    files = load()
    parent, children = tree(files)
    roots = [p for p, v in files.items() if v["label"] == root_label and p not in parent]
    print(len(files), "folder files;", "roots called", root_label, ":", len(roots))
    found = []

    def walk(p, depth):
        found.append(p)
        print("  " * depth + (files[p]["label"] or "?"), os.path.relpath(p, ROOT) if depth == 0 else "")
        for c in sorted(children.get(p, []), key=lambda c: files[c]["label"] or ""):
            if depth < 2:
                walk(c, depth + 1)
            else:
                stack = [c]
                while stack:
                    s = stack.pop()
                    found.append(s)
                    stack.extend(children.get(s, []))

    for r in roots:
        walk(r, 0)
    labels = {}
    for p in found:
        labels[files[p]["label"]] = labels.get(files[p]["label"], 0) + 1
    print("files under it:", len(found), "labels:", dict(sorted(labels.items())))
    if out:
        json.dump(found, open(out, "w"))
