"""Run in the sandbox during PIE; checks real save files, pause, and state restoration."""
import json
import uuid
from pathlib import Path
import unreal as ue

output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation"
output.mkdir(parents=True, exist_ok=True)
world, = ue.EditorLevelLibrary.get_pie_worlds(False)
pawn = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
session = pawn.foundation_session
prefs = session.preferences
assert prefs
slot = "Phase7Test_" + uuid.uuid4().hex
other_slot = slot + "_bad"
report = {}
profile = output.parent / "SaveGames" / "FoundationPreferences.sav"
previous_profile = profile.read_bytes() if profile.exists() else None
previous_sensitivity = prefs.get_editor_property("LookSensitivity")

def check(name, ok):
    report[name] = bool(ok)
    assert ok, name

def active_weapon():
    equipment = pawn.equipment
    return equipment.slots[equipment.active_slot]

try:
    session.toggle_pause()
    check("pause", session.menu_open and ue.GameplayStatics.is_game_paused(world))
    session.toggle_pause()
    check("resume", not session.menu_open and not ue.GameplayStatics.is_game_paused(world))
    prefs.set_editor_property("LookSensitivity", 1.75)
    check("preferences_saved", session.save_preferences())
    saved = ue.GameplayStatics.load_game_from_slot("FoundationPreferences", 0)
    check("preferences_disk_round_trip", saved.get_editor_property("LookSensitivity") == 1.75)
    check("preferences_applied_to_player", pawn.equipment.look_sensitivity == 1.75)
    check("invalid_slot_rejected", not session.save_checkpoint("../bad"))
    check("checkpoint_saved", session.save_checkpoint(slot))
    checkpoint = ue.GameplayStatics.load_game_from_slot(slot, 0)
    old = pawn.get_actor_location()
    start_health = checkpoint.get_editor_property("Health")
    start_weapon = active_weapon()
    inventory = pawn.interaction_manager
    item = ue.Name("phase7_test_token")
    check("inventory_changed", inventory.grant_item(item, 2) and inventory.owned_items[item] == 2)
    check("ammo_changed", pawn.equipment.fire_one() and active_weapon().magazine == start_weapon.magazine - 1)
    ue.GameplayStatics.apply_damage(pawn, 17, pc, pawn, ue.DamageType)
    check("damage_feedback", session.damage_alpha > 0)
    pawn.set_actor_location(old + ue.Vector(0, 65, 0), False, True)
    check("moved_before_restore", (pawn.get_actor_location() - old).length() > 50)
    session.complete_objective()
    check("objective_changed", session.objective_complete)
    check("wrong_save_class_written", ue.GameplayStatics.save_game_to_slot(saved, other_slot, 0))
    check("invalid_checkpoint_rejected_without_reset", not session.load_checkpoint(other_slot)
          and session.objective_complete and (pawn.get_actor_location() - old).length() > 50)
    check("checkpoint_loaded", session.load_checkpoint(slot))
    check("position_restored", (pawn.get_actor_location() - old).length() < 1)
    check("objective_restored", not session.objective_complete)
    check("restored_state_resaved", session.save_checkpoint(other_slot))
    restored = ue.GameplayStatics.load_game_from_slot(other_slot, 0)
    check("health_restored", restored.get_editor_property("Health") == start_health)
    check("equipment_restored", len(pawn.equipment.slots) > 0)
    end_weapon = active_weapon()
    check("exact_ammo_and_weapon_restored", end_weapon.definition == start_weapon.definition
          and end_weapon.magazine == start_weapon.magazine and end_weapon.reserve == start_weapon.reserve)
    check("inventory_restored", item not in inventory.owned_items)
finally:
    if session.menu_open:
        session.toggle_pause()
    prefs.set_editor_property("LookSensitivity", previous_sensitivity)
    session.save_preferences()
    if previous_profile is not None:
        profile.write_bytes(previous_profile)
    elif profile.exists():
        profile.unlink()
    for name in (slot, other_slot):
        ue.GameplayStatics.delete_game_in_slot(name, 0)
    (output / "phase7_pie.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
