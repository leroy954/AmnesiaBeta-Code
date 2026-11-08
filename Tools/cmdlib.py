"""Client for the cmd.json channel of FAmnesiaLevelJobs: run console commands (in PIE if it is running) and sample frame times."""
import json, os, time

JOBDIR = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\LevelJobs"


def cmd(cmds=(), sample=0.0, wait=0.0, world="auto", timeout=120):
    os.makedirs(JOBDIR, exist_ok=True)
    res = os.path.join(JOBDIR, "cmd_result.json")
    if os.path.exists(res):
        os.remove(res)
    tmp = os.path.join(JOBDIR, "cmd.tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump({"cmds": list(cmds), "sample": sample, "wait": wait, "world": world}, f)
    os.replace(tmp, os.path.join(JOBDIR, "cmd.json"))
    t0 = time.time()
    while not os.path.exists(res):
        time.sleep(0.1)
        if time.time() - t0 > timeout + sample + wait:
            raise RuntimeError("cmd job timed out")
    time.sleep(0.15)
    with open(res, encoding="utf-8") as f:
        return json.load(f)


def line(r):
    """One-line summary of a sampled result."""
    if "frame_ms" not in r:
        return "no samples"
    g = lambda k: r[k]["avg"]
    return "fps %5.1f | frame %5.1f ms (p95 %5.1f) | game %5.1f  render %5.1f  rhi %5.1f  gpu %5.1f" % (
        r["fps"], g("frame_ms"), r["frame_ms"]["p95"], g("game_ms"), g("render_ms"), g("rhi_ms"), g("gpu_ms"))
