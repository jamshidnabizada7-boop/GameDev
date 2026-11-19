"""Measure an on-hit damage number against the target's actual loss of health."""
import json
from pathlib import Path
import time
import traceback
import unreal as ue
world, = ue.EditorLevelLibrary.get_pie_worlds(False)
player = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
session = player.foundation_session
prefs = session.preferences
previous = prefs.show_damage_numbers
target, = [a for a in ue.GameplayStatics.get_all_actors_of_class(world, ue.Actor)
           if a.get_actor_label() == "P4_RangeTarget"]
hp_class = ue.load_class(None, "/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C")
health = target.get_component_by_class(hp_class)
assert health
for npc in ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationNPC):
    npc.set_ai_enabled(False)
report = {}
output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/damage_numbers_pie.json"
def check(label, result):
    report[label] = bool(result)
    assert result, label
def current_health():
    return health.get_editor_property("CurrentHealth")
def text_actors():
    return len(ue.GameplayStatics.get_all_actors_of_class(world, ue.TextRenderActor))
point = target.get_actor_location() + ue.Vector(0, 0, 75)
start = ue.Vector(point.x - 340, point.y, -1.8)
player.set_actor_location(start, False, True)
pc.set_control_rotation(ue.MathLibrary.find_look_at_rotation(
    ue.Vector(start.x - 13.456, start.y, 67.57), point))
prefs.show_damage_numbers = True
state = 0
due = ue.GameplayStatics.get_time_seconds(world) + .5
deadline = time.monotonic() + 20
def finish():
    prefs.show_damage_numbers = previous
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
def tick(dt):
    global state, due, baseline, shown
    try:
        assert time.monotonic() < deadline
        now = ue.GameplayStatics.get_time_seconds(world)
        if now < due:
            return
        if state == 0:
            baseline, before = text_actors(), current_health()
            check("first_shot_fired", player.equipment.fire_one())
            observed = before - current_health()
            shown = session.last_displayed_damage
            report["actual_loss"], report["displayed_amount"] = observed, shown
            check("amount_matches_health_loss", observed > 0 and abs(shown - observed) < .01)
            check("world_number_spawned", text_actors() == baseline + 1)
            marks = player.get_components_by_tag(ue.StaticMeshComponent, "FoundationImpactMark")
            active = [mark for mark in marks if mark.is_visible()]
            check("character_mark_attached", len(active) == 1 and active[0].get_attach_parent().get_owner() == target)
            tint = active[0].get_material(0).get_vector_parameter_value("Color")
            check("character_mark_red", tint.r > .3 and tint.g < .05 and tint.b < .05)
            prefs.show_damage_numbers = False
            due = now + 1.1
            state = 1
            return
        check("world_number_expired", text_actors() == baseline)
        check("character_mark_expired", not any(mark.is_visible() for mark in
            player.get_components_by_tag(ue.StaticMeshComponent, "FoundationImpactMark")))
        pc.set_control_rotation(ue.MathLibrary.find_look_at_rotation(
            player.p2_camera.get_world_location(), point))
        before = current_health()
        check("second_shot_fired", player.equipment.fire_one())
        check("disabled_hides_number", before > current_health() and text_actors() == baseline)
        finish()
    except Exception:
        report["error"] = traceback.format_exc()
        finish()
handle = ue.register_slate_post_tick_callback(tick)
