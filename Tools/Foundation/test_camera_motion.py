"""Measure camera feedback through actual actions in the possessed PIE player."""
import json
import time
import traceback
from pathlib import Path
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
pc = ue.GameplayStatics.get_player_controller(world, 0)
p = ue.GameplayStatics.get_player_pawn(world, 0)
prefs = p.foundation_session.preferences
initial = prefs.camera_motion
path = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/camera_motion_pie.json"
report = {"samples": []}
for npc in ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationNPC):
    npc.set_ai_enabled(False)
camera = p.p2_camera
movement = p.character_movement
def key(name, pressed):
    ue.Phase2Tools.test_key(pc, name, pressed)
def eye():
    return camera.get_world_location().z - p.get_actor_location().z
def pitch():
    return camera.get_world_rotation().pitch - pc.get_control_rotation().pitch
def start_trial(enabled):
    prefs.camera_motion = enabled
    movement.stop_movement_immediately()
    p.set_actor_location(start, False, True)
    pc.set_control_rotation(view)
    stats.update(base_fov=camera.field_of_view, fov=0, dip=0, kick=0, airborne=False, landed=False)
def sprint():
    key("LeftShift", True)
    key("W", True)
def stop():
    key("LeftShift", False)
    key("W", False)
    movement.stop_movement_immediately()
def reset_position():
    stop()
    p.set_actor_location(start, False, True)
def jump():
    stats["eye"] = eye()
    key("SpaceBar", True)
def fire():
    stats["pitch"] = pitch()
    assert p.equipment.fire_one(), "shot was rejected"
def sample(phase):
    if phase in ("landing", "shot"):
        report.setdefault("feedback_frames", []).append([
            prefs.camera_motion, phase, ue.GameplayStatics.get_time_seconds(world),
            ue.GameplayStatics.get_world_delta_seconds(world), p.get_actor_location().z,
            movement.velocity.z, eye(), pitch(), p.p2_recoil
        ])
    if phase == "sprint" and p.p2_sprinting:
        stats["fov"] = max(stats["fov"], camera.field_of_view - stats["base_fov"])
    if phase == "landing":
        stats["airborne"] |= movement.is_falling()
        if stats["airborne"] and not movement.is_falling():
            stats["landed"] = True
            stats["dip"] = max(stats["dip"], stats["eye"] - eye())
    if phase == "shot":
        stats["kick"] = max(stats["kick"], abs(pitch() - stats["pitch"]))
def record():
    report["samples"].append(dict(stats, motion=prefs.camera_motion))
stats = {}
start, view = p.get_actor_location(), pc.get_control_rotation()
steps = []
for enabled in (True, False):
    steps.extend([
        ("settle", .3, lambda active=enabled: start_trial(active)),
        ("sprint", .6, sprint), ("stop", .5, reset_position),
        ("landing", .1, jump), ("landing", 1.2, lambda: key("SpaceBar", False)),
        ("shot", .22, fire), ("record", .2, record),
    ])
index = -1
end = 0
deadline = time.monotonic() + 45
performance_class = ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings")
performance = ue.get_default_object(performance_class)
throttle = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
def finish():
    stop()
    key("SpaceBar", False)
    prefs.camera_motion = initial
    performance.set_editor_property("bThrottleCPUWhenNotForeground", throttle)
    ue.unregister_slate_post_tick_callback(handle)
    path.write_text(json.dumps(report, indent=2), encoding="utf-8")
def tick(dt):
    global index, end
    try:
        assert time.monotonic() < deadline, "PIE camera test timed out"
        now = ue.GameplayStatics.get_time_seconds(world)
        if index >= 0:
            sample(steps[index][0])
        if now < end:
            return
        index += 1
        if index < len(steps):
            _, duration, action = steps[index]
            action()
            end = now + duration
            return
        motion, reduced = report["samples"]
        report["checks"] = {
            "sprint_fov": motion["fov"] > 3,
            "landing_dip": motion["landed"] and motion["dip"] > 1,
            "shot_kick": motion["kick"] > .1,
            "reduced_motion": reduced["landed"] and all(reduced[k] < .1 for k in ("fov", "dip", "kick")),
        }
        finish()
    except Exception:
        report["error"] = traceback.format_exc()
        finish()
handle = ue.register_slate_post_tick_callback(tick)
