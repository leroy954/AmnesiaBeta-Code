"""Shared helpers for PIE experiments: start PIE, find PIE actors, set visibility, measure frame rate from the log."""
import sys, time, json, os
OLD = r"C:\Users\User\AppData\Local\Temp\claude\C--Users-User-Documents-Unreal-Projects-AmnesiaBeta\d0b02c3a-2445-4460-9972-ba541eae391b\scratchpad"
HERE = os.path.dirname(os.path.abspath(__file__))
for p in (OLD, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
import ue
import levelgen as lg
import fps as F


def release_ctrl_in_editor():
    """Posts Ctrl key-up messages to the editor's own windows only."""
    import ctypes
    from ctypes import wintypes
    user32 = ctypes.windll.user32
    pids = set()
    import subprocess
    out = subprocess.run(["powershell", "-NoProfile", "-Command", "(Get-Process UnrealEditor).Id"], capture_output=True, text=True).stdout
    pids = {int(p) for p in out.split() if p.strip().isdigit()}
    handles = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def each(hwnd, _):
        pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value in pids:
            handles.append(hwnd)
        return True

    user32.EnumWindows(each, 0)
    WM_KEYUP = 0x0101
    for hwnd in handles:
        for vk, scan, ext in ((0x11, 0x1D, 0), (0xA2, 0x1D, 0), (0xA3, 0x1D, 1)):
            lparam = 1 | (scan << 16) | (ext << 24) | (1 << 30) | (1 << 31)
            user32.PostMessageW(hwnd, WM_KEYUP, vk, ctypes.c_long(lparam - (1 << 32) if lparam >= (1 << 31) else lparam).value)
    time.sleep(0.5)


def start_play(warmup=6):
    """Starts PIE as the player. The editor starts play as a spectator if Ctrl is down at that moment (the user may be
    typing in another app), so wait for Ctrl to be up and try again if it still came up as a spectator."""
    import ctypes, subprocess
    for attempt in range(8):
        if ue.pie_on():
            ue.call(ue.E, "StopPIE")
            time.sleep(3)
        for _ in range(100):
            if not (ctypes.windll.user32.GetAsyncKeyState(0x11) & 0x8000):
                break
            time.sleep(0.1)
        # the editor keeps its own modifier state from its window messages; a Ctrl release that went to another app
        # leaves it thinking Ctrl is down. Tell the editor window that Ctrl is up
        release_ctrl_in_editor()
        ue.call(ue.E, "StartPIE", options={"bSimulate": False, "playMode": "PlayMode_InViewPort", "warmupSeconds": warmup})
        time.sleep(1.5)
        if pie("AmnesiaCharacter"):
            return True
        print("play started as a spectator, trying again")
    return False


def pie(name=""):
    return [a for a in ue.call(ue.S, "find_actors", name=name, collision_channels=[], tag="") if "UEDPIE" in a["refPath"]]


def _last():
    s = F.samples(300)
    return s[-1] if s else None


def ping():
    ue.pie_on()
    t0 = time.time()
    last = _last()
    while time.time() - t0 < 1.5:          # wait until the log line is on disk
        time.sleep(0.05)
        cur = _last()
        if cur and (last is None or cur[0] >= time.time() - 6) and cur != last:
            return cur
        if cur and cur[0] > t0 - 0.3:
            return cur
    return _last()


def measure(seconds=4.0):
    a = ping()
    time.sleep(seconds)
    b = ping()
    if not a or not b:
        return None
    df = (b[1] - a[1]) % 1000
    dt = b[0] - a[0]
    return round(df / dt, 1) if dt > 0.5 else None


def warm_up(limit=150):
    """Waits until PIE shaders/PSOs are compiled and the frame rate is stable above 15."""
    t0 = time.time()
    good = 0
    while time.time() - t0 < limit:
        v = measure(2.5)
        if v and v > 15:
            good += 1
            if good >= 2:
                return v
        else:
            good = 0
    return None


def warm_up(limit=150):
    """Waits until PIE shaders/PSOs are compiled and the frame rate is stable above 15."""
    t0 = time.time()
    good = 0
    while time.time() - t0 < limit:
        v = measure(2.5)
        if v and v > 15:
            good += 1
            if good >= 2:
                return v
        else:
            good = 0
    return None


def start_pie(park_enemies=True):
    # start_play makes sure the session is not a spectator one (the editor sometimes thinks Ctrl is down after a restart)
    if not start_play():
        raise RuntimeError("the play session came up as a spectator")
    time.sleep(2)
    huds = pie("HUD")
    if huds:
        ue.props(huds[0], bDebugCloseMenu=True)
        time.sleep(4)
    hero = None
    for _ in range(20):
        h = pie("AmnesiaCharacter")
        if h:
            hero = h[0]
            break
        time.sleep(1)
    if park_enemies:
        n = 0
        for e in pie("Ent_"):
            ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(200000 + n * 300, 0, 3000, 0, 0, 0, 1, 1, 1))
            n += 1
    return hero


def live_hero():
    """The hero of the running game. A stopped session can leave a destroyed one behind under the same name."""
    for _ in range(20):
        for h in pie("AmnesiaCharacter"):
            try:
                ue.call(ue.A, "get_actor_transform", actor=h)
                return h
            except RuntimeError:
                continue
        time.sleep(0.5)
    return None


def ground_h(x):
    """Walkable height at x in the first level, or in the long world far beyond it."""
    if x >= 148000:
        import lw
        return lw.h(x)
    return lg.h(x)


def teleport(hero, x):
    try:
        ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(x, 0, ground_h(x) + 220, 0, 0, 0, 1, 1, 1))
    except RuntimeError:
        ue.call(ue.A, "set_actor_transform", actor=live_hero(), xform=ue.xform(x, 0, ground_h(x) + 220, 0, 0, 0, 1, 1, 1))


def comps(actor, sub):
    return [c for c in ue.call(ue.A, "get_components", actor=actor) if sub.lower() in c["refPath"].lower()]


def set_comp(actor, sub, **values):
    n = 0
    for c in comps(actor, sub):
        ue.props(c, **values)
        n += 1
    return n
