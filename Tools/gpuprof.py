"""Parses the last 'GPU Profile for Frame' table in the editor log: top exclusive GPU costs and the hierarchy's big nodes."""
import re, sys
LOG = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\Logs\AmnesiaBeta.log"
which = int(sys.argv[1]) if len(sys.argv) > 1 else -1   # which profile in the log (-1 = last)
top = int(sys.argv[2]) if len(sys.argv) > 2 else 28

lines = open(LOG, encoding="utf-8", errors="replace").read().splitlines()
starts = [i for i, l in enumerate(lines) if "GPU Profile for Frame" in l]
print("profiles in log:", len(starts))
s = starts[which]
rows = []
frame_time = None
for l in lines[s:s + 1500]:
    m = re.search(r"Frame Time\s*:\s*([\d.]+)ms", l)
    if m and frame_time is None:
        frame_time = float(m.group(1))
    if "LogRHI" not in l:
        if rows:
            break
        continue
    body = l.split("Display:", 1)[-1]
    if "┃" not in body:
        continue
    cells = [c.strip() for c in re.split(r"[┃│╂┊]", body)]
    # exclusive: Draws,Dsptch,Prim,Vert,Percent,(bar),Time ; inclusive ... ; name is last non-empty cell
    nums = re.findall(r"([\d.]+) ms", body)
    name = cells[-1] if cells and cells[-1] else (cells[-2] if len(cells) > 1 else "")
    name = re.sub(r"^[\s├└─│┬┴╴]+", "", body.split("┃")[-1]).strip() or name
    if not nums:
        continue
    excl = float(nums[0])
    incl = float(nums[1]) if len(nums) > 1 else excl
    draws = re.search(r"┃\s*(\d+)\s*│", body)
    rows.append((excl, incl, int(draws.group(1)) if draws else 0, name))
print("frame time: %s ms, rows: %d" % (frame_time, len(rows)))
print("--- top exclusive ---")
agg = {}
for e, i, d, n in rows:
    key = re.sub(r"\s+\d+(\.\d+)?\s*$", "", n)[:70]
    a = agg.setdefault(key, [0.0, 0, 0])
    a[0] += e
    a[1] += d
    a[2] += 1
for k, (e, d, c) in sorted(agg.items(), key=lambda kv: -kv[1][0])[:top]:
    print("%7.3f ms  draws %5d  x%-3d  %s" % (e, d, c, k))
print("--- biggest inclusive nodes ---")
for e, i, d, n in sorted(rows, key=lambda r: -r[1])[:22]:
    print("%7.3f ms incl  %7.3f excl  %s" % (i, e, n[:80]))
