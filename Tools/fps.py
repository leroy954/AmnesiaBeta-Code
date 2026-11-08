"""Estimates engine frame rate from the editor log: each line carries [timestamp][frame % 1000]."""
import re, sys, datetime, time

LOG = r"C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\Saved\Logs\AmnesiaBeta.log"
pat = re.compile(r"^\[(\d{4})\.(\d\d)\.(\d\d)-(\d\d)\.(\d\d)\.(\d\d):(\d{3})\]\[\s*(\d+)\]")


def samples(n=4000):
    with open(LOG, "rb") as f:
        f.seek(0, 2)
        size = f.tell()
        f.seek(max(0, size - 600000))
        data = f.read().decode("utf-8", "replace").splitlines()
    out = []
    for line in data[-n:]:
        m = pat.match(line)
        if m:
            y, mo, d, h, mi, s, ms, fr = map(int, m.groups())
            t = datetime.datetime(y, mo, d, h, mi, s).timestamp() + ms / 1000.0
            out.append((t, fr))
    return out


def fps(window=3.0):
    s = samples()
    if len(s) < 2:
        return None
    t_end, f_end = s[-1]
    # find the sample ~window seconds earlier
    i = len(s) - 1
    while i > 0 and t_end - s[i][0] < window:
        i -= 1
    t0, f0 = s[i]
    df = (f_end - f0) % 1000
    dt = t_end - t0
    return (df / dt) if dt > 0 else None


if __name__ == "__main__":
    for _ in range(int(sys.argv[1]) if len(sys.argv) > 1 else 1):
        v = fps()
        print("fps ~", None if v is None else round(v, 1))
        time.sleep(1.0)
