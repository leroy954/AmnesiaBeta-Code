"""Rebuilds files written in earlier Claude sessions by replaying their Write and Edit tool calls from the session transcripts.
Usage: python recover.py <out dir> <file name> [<file name> ...]"""
import json, os, sys, glob

out_dir = sys.argv[1]
names = sys.argv[2:]
transcripts = sorted(glob.glob(os.path.expanduser(r"~\.claude\projects\C--Users-User-Documents-Unreal-Projects-AmnesiaBeta\*.jsonl")), key=os.path.getmtime)

files = {}
for path in transcripts:
    with open(path, encoding="utf-8") as f:
        for line in f:
            try:
                entry = json.loads(line)
            except Exception:
                continue
            msg = entry.get("message") or {}
            content = msg.get("content")
            if not isinstance(content, list):
                continue
            for block in content:
                if block.get("type") != "tool_use":
                    continue
                inp = block.get("input") or {}
                fp = inp.get("file_path", "")
                base = os.path.basename(fp)
                if base not in names:
                    continue
                if block.get("name") == "Write":
                    files[base] = inp.get("content", "")
                elif block.get("name") == "Edit" and base in files:
                    old, new = inp.get("old_string", ""), inp.get("new_string", "")
                    if inp.get("replace_all"):
                        files[base] = files[base].replace(old, new)
                    elif old in files[base]:
                        files[base] = files[base].replace(old, new, 1)
                    else:
                        print("edit did not apply to", base, repr(old[:60]))

os.makedirs(out_dir, exist_ok=True)
for name in names:
    if name in files:
        with open(os.path.join(out_dir, name), "w", encoding="utf-8") as f:
            f.write(files[name])
        print("recovered", name, len(files[name]), "chars")
    else:
        print("not found", name)
