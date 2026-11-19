"""Measure virtual controller stick input through the player's active mappings."""
import json
import time
import traceback
from pathlib import Path
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
player = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
origin = player.get_actor_location()
view = pc.get_control_rotation()
performance = ue.get_default_object(ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
previous = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
start = ue.GameplayStatics.get_time_seconds(world)
deadline = time.monotonic()+20
report = {}
out = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/controller_axes_pie.json"
def axis(name, value):
    ue.Phase2Tools.test_key(pc, name, True, value)
def tick(dt):
    try:
        assert time.monotonic()<deadline
        elapsed = ue.GameplayStatics.get_time_seconds(world)-start
        if elapsed < .65:
            axis("Gamepad_LeftX", .7)
            axis("Gamepad_RightX", .65)
            return
        delta = player.get_actor_location()-origin
        report["travel_cm"] = (delta.x**2+delta.y**2)**.5
        report["yaw_degrees"] = abs((pc.get_control_rotation().yaw-view.yaw+180)%360-180)
        report["passed"] = report["travel_cm"]>15 and report["yaw_degrees"]>2
    except Exception:
        report["error"] = traceback.format_exc()
    axis("Gamepad_LeftX", 0)
    axis("Gamepad_RightX", 0)
    player.character_movement.stop_movement_immediately()
    player.set_actor_location(origin, False, True)
    pc.set_control_rotation(view)
    performance.set_editor_property("bThrottleCPUWhenNotForeground", previous)
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
handle = ue.register_slate_post_tick_callback(tick)
