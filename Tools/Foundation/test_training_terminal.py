"""Drive the sandbox objective through focus and input, then reload its prior state."""
import json
import time
import traceback
import uuid
from pathlib import Path
import unreal as ue
world, = ue.EditorLevelLibrary.get_pie_worlds(False)
p = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
session = p.foundation_session
terminal, = [a for a in ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationInteractable)
             if str(a.get_editor_property("StableId")) == "Foundation_TrainingTerminal"]
slot = f"TerminalTest_{uuid.uuid4().hex}"
report = {}
peak_reach = 0.0
output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/terminal_pie.json"
def check(label, value):
    report[label] = bool(value)
    assert value, label
def key(pressed):
    ue.Phase2Tools.test_key(pc, "E", pressed)
def aim_and_save():
    check("starts_pending", not session.objective_complete)
    pc.set_control_rotation(ue.MathLibrary.find_look_at_rotation(
        p.p2_camera.get_world_location(), terminal.interaction_point()))
    check("checkpoint_before_use", session.save_checkpoint(slot))
def first_press():
    check("focus_uses_native_dispatch", p.interaction_manager.focused_actor == terminal)
    key(True)
def first_release():
    key(False)
    check("terminal_activated", session.objective_complete and terminal.use_count == 1)
    check("hand_reaches_on_instant_use", peak_reach > .5)
    check("caption_emitted", "activated" in str(session.subtitle).lower())
def try_again():
    key(True)
def restore():
    key(False)
    check("terminal_cannot_complete_twice", terminal.use_count == 1)
    check("checkpoint_restored_pending_objective", session.load_checkpoint(slot) and not session.objective_complete)
steps = [(aim_and_save, .5), (first_press, .35), (first_release, .3),
         (try_again, .35), (restore, .2)]
index, next_time = 0, 0.0
deadline = time.monotonic() + 30
performance = ue.get_default_object(ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
throttle = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
def finish():
    key(False)
    performance.set_editor_property("bThrottleCPUWhenNotForeground", throttle)
    ue.GameplayStatics.delete_game_in_slot(slot, 0)
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
def tick(dt):
    global index, next_time, peak_reach
    try:
        peak_reach = max(peak_reach, p.interaction_manager.reach_alpha)
        assert time.monotonic() < deadline
        now = ue.GameplayStatics.get_time_seconds(world)
        if now < next_time:
            return
        if index == len(steps):
            finish()
            return
        step, delay = steps[index]
        step()
        index += 1
        next_time = now + delay
    except Exception:
        report["error"] = traceback.format_exc()
        finish()
handle = ue.register_slate_post_tick_callback(tick)
