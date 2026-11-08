"""How far does the air slash reach? Holds the hero in the air (no gravity), puts one frozen enemy at a set offset from him,
does the air slash (bDebugAirSlash) and reads whether that enemy lost health.
Every trial uses an enemy that still has most of its health, so the table is not cut short by deaths.
Usage: python air_slash_test.py [dx ...]     (centimetres in front of the hero, negative is behind; default a spread)"""
import json
import sys
import time

import ue
from pie_lib import pie, start_play, live_hero

PERF = {"refPath": "/Script/UnrealEd.Default__EditorPerformanceSettings"}
DXS = [int(v) for v in sys.argv[1:]] or [-200, -110, 110, 130, 150, 170, 190, 210, 230, 250, 280]   # closer than ~105 the capsules would overlap
DZS = [100, 0, -100, -200]
HOLD_Z = 450.0

ue.props(PERF, bThrottleCPUWhenNotForeground=False)
try:
    start_play()
    ue.props(pie("HUD")[0], bDebugCloseMenu=True)
    time.sleep(3)
    hero = live_hero()
    move = {"refPath": hero["refPath"] + ".CharMoveComp"}
    ue.props(hero, HurtInvulnerabilityTime=99999.0)      # the first hit he takes makes him immune for good

    # the enemies to test with: Ents only, so the bodies are alike. Every other enemy (there is a hostile dragon near the start,
    # whose fireballs also hurt the Ents) goes far away
    enemy_class = {"refPath": "/Script/AmnesiaBeta.AmnesiaEnemy"}
    everyone = [a for a in ue.call(ue.S, "find_actors", name="", actor_type=enemy_class, collision_channels=[], tag="") if "UEDPIE" in a["refPath"]]
    pool = [a for a in everyone if "BP_Ent_" in a["refPath"]]
    for n, e in enumerate(everyone):
        try:
            ue.call(ue.A, "set_actor_transform", actor=e, xform=ue.xform(100000 + n * 400, 0, 3000 if e in pool else 6000, 0, 0, 0, 1, 1, 1))
        except RuntimeError:
            pass
    for e in pool:
        ue.props(e, CustomTimeDilation=0.001)
    print("%d enemies to test with" % len(pool))

    # the little dragon shoots fireballs at the nearest enemy, which would spoil the table: it is removed from this play session
    for c in pie("ompanion"):
        try:
            ue.call(ue.S, "remove_from_scene", actor=c)
        except RuntimeError:
            pass

    ue.props(move, GravityScale=0.0)
    hx = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["x"]
    print("hero at x %.0f, held at z %.0f" % (hx, HOLD_Z))
    print("rows: enemy height relative to the hero (cm); columns: distance in front of the hero (cm).")
    print("H = the hero bounced (a slash hit), d = the enemy lost health without a bounce, . = nothing happened")
    print("%8s" % "dz\\dx" + "".join("%6d" % dx for dx in DXS))

    def hp_of(e):
        try:
            return ue.get_props(e, "CurrentHP")["CurrentHP"]
        except RuntimeError:
            return None                                   # destroyed

    for dz in DZS:
        row = "%8d" % dz
        for dx in DXS:
            # an enemy that is still healthy
            target, before = None, None
            for e in pool:
                hp = hp_of(e)
                if hp is not None and hp > 3.0:
                    target, before = e, hp
                    break
            if target is None:
                row += "%6s" % "?"
                continue
            try:
                ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(hx, 0, HOLD_Z, 0, 0, 0, 1, 1, 1))
            except RuntimeError:
                print("the hero died, a new one takes over")
                hero = live_hero()
                move = {"refPath": hero["refPath"] + ".CharMoveComp"}
                ue.props(hero, HurtInvulnerabilityTime=99999.0)
                ue.props(move, GravityScale=0.0)
                ue.call(ue.A, "set_actor_transform", actor=hero, xform=ue.xform(hx, 0, HOLD_Z, 0, 0, 0, 1, 1, 1))
            ue.props(move, Velocity={"x": 0, "y": 0, "z": 0})
            ue.props(target, CustomTimeDilation=0.001)
            ue.call(ue.A, "set_actor_transform", actor=target, xform=ue.xform(hx + dx, 0, HOLD_Z + dz, 0, 0, 0, 1, 1, 1))
            time.sleep(0.9)                                  # long enough for the sleeping enemy to wake (the hero checks every 0.4 s)
            ue.props(hero, bDebugAirSlash=True)
            time.sleep(0.9)
            after = hp_of(target)
            lost = after is None or after < before - 0.01
            hz = ue.call(ue.A, "get_actor_transform", actor=hero)["location"]["z"]
            bounced = hz > HOLD_Z + 120                      # a slash hit bounces the hero up, and without gravity he keeps rising
            row += "%6s" % ("H" if bounced else "d" if lost else ".")
            if lost and not bounced:
                print("   (dx %d dz %d: health %.2f -> %s)" % (dx, dz, before, "gone" if after is None else "%.2f" % after))
            # put it away again, frozen
            try:
                ue.props(target, CustomTimeDilation=0.001)
                ue.call(ue.A, "set_actor_transform", actor=target, xform=ue.xform(100000, 0, 3000, 0, 0, 0, 1, 1, 1))
            except RuntimeError:
                pass
        print(row)
        sys.stdout.flush()
finally:
    try:
        ue.call(ue.E, "StopPIE")
    except Exception:
        pass
    ue.props(PERF, bThrottleCPUWhenNotForeground=True)
