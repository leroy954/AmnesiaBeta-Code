"""Runs a Python script inside the editor (full `unreal` module) through the level job runner's exec op, then prints
Saved/retarget_result.txt, where the editor scripts write their results.
Usage: python editor_py.py <script.py> [args...]"""
import os
import sys

import ue

HERE = os.path.dirname(os.path.abspath(__file__))
RESULT = os.path.join(HERE, "..", "Saved", "retarget_result.txt")

script = os.path.abspath(os.path.join(HERE, sys.argv[1])).replace("\\", "/")
args = " ".join(sys.argv[2:])

if os.path.exists(RESULT):
    os.remove(RESULT)

print(ue.job([{"op": "exec", "command": f'py "{script}" {args}'.strip()}], timeout=1200))

if os.path.exists(RESULT):
    with open(RESULT, encoding="utf-8") as f:
        print(f.read())
