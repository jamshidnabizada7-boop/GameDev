"""Compare ankle-to-ground clearance on a flat platform and a 30-degree side slope in PIE."""
import json
import math
import time
import traceback
from pathlib import Path

import unreal as ue

world = ue.EditorLevelLibrary.get_pie_worlds(False)[0]
player = ue.GameplayStatics.get_player_pawn(world, 0)
controller = ue.GameplayStatics.get_player_controller(world, 0)
origin = player.get_actor_location()
rotation = controller.get_control_rotation()
station = origin + ue.Vector(0, 0, 5000)
platform = ue.Phase2Tools.test_blocker(player, station, ue.Vector(1000, 1000, 30))
step = ue.Phase2Tools.test_blocker(player, station + ue.Vector(175, 0, 35), ue.Vector(150, 300, 40))
mesh = platform.static_mesh_component
mesh.set_mobility(ue.ComponentMobility.MOVABLE)
report = {}
out = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/foot_placement_pie.json"

def reset():
    player.character_movement.stop_movement_immediately()
    player.set_actor_location(station + ue.Vector(0, 0, 150), False, True)
    controller.set_control_rotation(ue.Rotator())

def ankles():
    normal = platform.get_actor_up_vector()
    top = platform.get_actor_location() + normal * 15
    clearance = []
    for bone in ("foot_l", "foot_r"):
        p = player.mesh.get_socket_location(bone)
        ground = top.z - ((p.x-top.x)*normal.x + (p.y-top.y)*normal.y)/normal.z
        clearance.append(p.z-ground)
    return clearance

def incline():
    report["flat_clearance_cm"] = ankles()
    platform.set_actor_rotation(ue.Rotator(roll=30), True)
    reset()

def start_step():
    report["slope_clearance_cm"] = ankles()
    report["slope_degrees"] = math.degrees(math.acos(platform.get_actor_up_vector().z))
    platform.set_actor_rotation(ue.Rotator(), True)
    reset()

def walk():
    report["before_step_z"] = player.get_actor_location().z
    ue.Phase2Tools.test_key(controller, "W", True)

def measure_step():
    ue.Phase2Tools.test_key(controller, "W", False)
    pos = player.get_actor_location()
    report["step_rise_cm"] = pos.z - report["before_step_z"]
    report["step_travel_cm"] = pos.x - station.x
    report["step_passed"] = abs(report["step_rise_cm"]-40) < 3 and report["step_travel_cm"] > 100
    report["max_clearance_change_cm"] = max(abs(a-b) for a,b in zip(report["flat_clearance_cm"], report["slope_clearance_cm"]))
    report["slope_passed"] = abs(report["slope_degrees"]-30) < .1 and report["max_clearance_change_cm"] < 4

steps = [(reset, 1.5), (incline, 1.5), (start_step, 1.5), (walk, .50), (measure_step, 0)]
idx = 0
next_time = 0
deadline = time.monotonic()+60
performance = ue.get_default_object(ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
throttle = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
def tick(dt):
    global idx, next_time
    try:
        assert time.monotonic() < deadline, "PIE timed out"
        now = ue.GameplayStatics.get_time_seconds(world)
        if now < next_time:
            return
        if idx < len(steps):
            action, delay = steps[idx]
            action()
            idx += 1
            next_time = now+delay
            return
    except Exception:
        report["error"] = traceback.format_exc()
    ue.Phase2Tools.test_key(controller, "W", False)
    player.set_actor_location(origin, False, True)
    controller.set_control_rotation(rotation)
    platform.destroy_actor()
    step.destroy_actor()
    performance.set_editor_property("bThrottleCPUWhenNotForeground", throttle)
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)

handle = ue.register_slate_post_tick_callback(tick)
