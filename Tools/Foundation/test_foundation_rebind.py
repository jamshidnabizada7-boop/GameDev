"""Validate key collision handling, live mapped input and default recovery in PIE."""
import json
from pathlib import Path
import time
import traceback
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
controller = ue.GameplayStatics.get_player_controller(world, 0)
pawn = ue.GameplayStatics.get_player_pawn(world, 0)
session = pawn.foundation_session
output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir()))
profile = output / "SaveGames/FoundationPreferences.sav"
backup = profile.read_bytes() if profile.exists() else None
out = output / "Foundation/rebind_pie.json"
report = {}
replacement = "Comma"

def key(name):
    value = ue.Key()
    assert value.import_text(name), name
    return value
def press(name, down):
    # TestKey reports whether a binding consumed the event. Unbound W is expected.
    ue.Phase2Tools.test_key(controller, name, down)
def check(name, value):
    report[name] = bool(value)
    assert value, name
def setup():
    session.restore_defaults()
    check("duplicate_key_rejected", not session.rebind("W", key("A")))
    check("reserved_key_rejected", not session.rebind("W", key("Escape")))
    check("alternate_key_saved", session.rebind("W", key(replacement)))
    check("profile_written", ue.GameplayStatics.does_save_game_exist("FoundationPreferences", 0))
def sample_old():
    check("old_key_disabled", (pawn.get_actor_location() - start).length() < 5)
    press("W", False)
    press(replacement, True)
def sample_new():
    check("new_key_moves", (pawn.get_actor_location() - start).length() > 40)
    press(replacement, False)
    session.restore_defaults()
    pawn.character_movement.stop_movement_immediately()
def sample_restored():
    check("default_key_moves", (pawn.get_actor_location() - start).length() > 40)
    press("W", False)
start = pawn.get_actor_location()
def reset():
    pawn.character_movement.stop_movement_immediately()
    pawn.set_actor_location(start, False, True)
steps = [(setup, .4), (lambda: press("W", True), .55), (sample_old, .55),
         (sample_new, .4), (reset, .25), (lambda: press("W", True), .55), (sample_restored, .1)]
index, next_time = 0, 0
deadline = time.monotonic() + 30
def finish():
    press("W", False)
    press(replacement, False)
    session.restore_defaults()
    if backup is not None:
        profile.write_bytes(backup)
    elif profile.exists():
        profile.unlink()
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
def tick(dt):
    global index, next_time
    try:
        assert time.monotonic() < deadline
        now = ue.GameplayStatics.get_time_seconds(world)
        if now < next_time:
            return
        if index == len(steps):
            finish()
            return
        action, delay = steps[index]
        action()
        index += 1
        next_time = now + delay
    except Exception:
        report["error"] = traceback.format_exc()
        finish()
handle = ue.register_slate_post_tick_callback(tick)
