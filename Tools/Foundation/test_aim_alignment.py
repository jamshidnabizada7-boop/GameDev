"""Measure muzzle alignment and both first-person grip distances during a pitch sweep."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
p = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
view = pc.get_control_rotation()
origin = p.get_actor_location()
pref = p.foundation_session.preferences
motion = pref.camera_motion
pref.camera_motion = False
performance = ue.get_default_object(ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
throttle = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
out = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/aim_alignment_pie.json"
report = {"samples": []}
angles = [-89, -60, -30, 0, 30, 60, 89]
p.set_actor_location(origin+ue.Vector(0, 0, 3000), False, True)
p.character_movement.set_movement_mode(ue.MovementMode.MOVE_FLYING)
idx = 0
settle = False
due = 0
deadline = time.monotonic()+45
def tick(dt):
    global idx, due, settle
    try:
        assert time.monotonic()<deadline
        now = ue.GameplayStatics.get_time_seconds(world)
        if now<due:
            return
        if idx < len(angles):
            if not settle:
                pc.set_control_rotation(ue.Rotator(pitch=angles[idx], yaw=view.yaw))
                due = now+.35
                settle = True
                return
            gun = p.p2_grip_right.get_owner()
            muzzle, = [c for c in gun.get_components_by_class(ue.SceneComponent) if c.get_name()=="MuzzlePoint"]
            a = muzzle.get_forward_vector()
            b = p.p2_camera.get_forward_vector()
            alignment = math.degrees(math.acos(max(-1., min(1., a.dot(b)))))
            left = p.p2_arms.get_socket_location("hand_l")
            right = p.p2_arms.get_socket_location("hand_r")
            report["samples"].append(dict(pitch=angles[idx], error_deg=alignment,
                left_grip_cm=(left-p.p2_grip_left.get_world_location()).length(),
                right_grip_cm=(right-p.p2_grip_right.get_world_location()).length()))
            idx += 1
            settle = False
            return
        report["muzzle_aligned"] = all(row["error_deg"] < 3 for row in report["samples"])
        report["both_grips_aligned"] = all(max(row["left_grip_cm"], row["right_grip_cm"]) < 1 for row in report["samples"])
    except Exception:
        report["error"] = traceback.format_exc()
    pref.camera_motion = motion
    pc.set_control_rotation(view)
    p.character_movement.set_movement_mode(ue.MovementMode.MOVE_WALKING)
    p.set_actor_location(origin, False, True)
    performance.set_editor_property("bThrottleCPUWhenNotForeground", throttle)
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
handle = ue.register_slate_post_tick_callback(tick)
