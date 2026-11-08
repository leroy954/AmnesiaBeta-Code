"""Checks that the saved master volume really changes what the game plays.
For each volume it writes the value into the save file, starts the game, and measures the output peak of the editor's
audio session (the Windows mixer meter, audio_peak.ps1). The save file is put back as it was.
Usage: python audio_volume_test.py [volume ...]   (percent, default 80 10)"""
import os
import shutil
import struct
import subprocess
import sys
import threading
import time

import ue
from pie_lib import pie, start_play, HERE

PROJECT = os.path.dirname(HERE)
SAVE = os.path.join(PROJECT, "Saved", "SaveGames", "AmnesiaProgress.sav")
BACKUP = SAVE + ".volume_test_backup"
VOLUMES = [int(v) for v in sys.argv[1:]] or [80, 10]

MISC = {"refPath": "/Script/UnrealEd.Default__LevelEditorMiscSettings"}
PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}


def volume_offset(data):
    """Where the float of MasterVolume starts in the save file, or -1. The file only lists values that differ from the default."""
    i = data.find(b"MasterVolume\x00")
    j = data.find(b"FloatProperty\x00", i) if i >= 0 else -1
    return j + len(b"FloatProperty\x00") + 8 + 1 if j >= 0 else -1      # type parameters, size, property guid flag


def write_volume(value):
    data = bytearray(open(SAVE, "rb").read())
    k = volume_offset(data)
    if k < 0:
        raise SystemExit("MasterVolume is not in the save file (it is at the default): set it once in the menu first")
    data[k:k + 4] = struct.pack("<f", value)
    open(SAVE, "wb").write(data)


samples = []          # (unix ms, peak)


def meter_thread(seconds):
    p = subprocess.Popen(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "audio_peak.ps1"),
                          "-Seconds", str(seconds), "-Window", "0.5"], stdout=subprocess.PIPE, text=True)
    for line in p.stdout:
        parts = line.split()
        if len(parts) == 2:
            try:
                samples.append((int(parts[0]), float(parts[1])))
            except ValueError:
                pass


def level_between(t0, t1):
    vals = [v for (t, v) in samples if t0 <= t <= t1]
    return (round(max(vals), 4), round(sum(vals) / len(vals), 4), len(vals)) if vals else None


def now_ms():
    return int(time.time() * 1000)


if __name__ == "__main__":
    shutil.copy2(SAVE, BACKUP)
    background = ue.get_props(MISC, "bAllowBackgroundAudio")["bAllowBackgroundAudio"]
    ue.props(MISC, bAllowBackgroundAudio=True)         # the editor mutes the game when its window is not in front
    ue.props(PERF, bThrottleCPUWhenNotForeground=False)
    results = {}
    try:
        threading.Thread(target=meter_thread, args=(60 + 40 * len(VOLUMES),), daemon=True).start()

        for percent in VOLUMES:
            shutil.copy2(BACKUP, SAVE)                   # New Game rewrites the file without the default values
            write_volume(percent / 100.0)
            if not start_play():
                raise SystemExit("PIE did not start as the player")
            time.sleep(3)
            ue.props(pie("HUD")[0], bDebugCloseMenu=True)
            time.sleep(5)
            t0 = now_ms()
            time.sleep(6)
            results[percent] = level_between(t0, now_ms())
            print("volume %3d%%: peak/avg/n = %s" % (percent, results[percent]))
            ue.call(ue.E, "StopPIE")
            time.sleep(3)

        base = VOLUMES[0]
        if results.get(base) and results[base][1] > 0.0005:
            for percent in VOLUMES[1:]:
                if results.get(percent):
                    print("average level at %d%% / at %d%% = %.2f   (volume ratio %.2f)" % (percent, base, results[percent][1] / results[base][1], percent / base))
        else:
            print("no sound to compare (is the game quiet here?)")
    finally:
        try:
            ue.call(ue.E, "StopPIE")
        except Exception:
            pass
        shutil.copy2(BACKUP, SAVE)
        os.remove(BACKUP)
        ue.props(MISC, bAllowBackgroundAudio=background)
        ue.props(PERF, bThrottleCPUWhenNotForeground=True)
        print("save file restored")
