"""Check actual player damage events and color-independent directional cues in PIE."""
import json
import math
from pathlib import Path
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
pawn = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
session = pawn.foundation_session
prefs = session.preferences
old_contrast = prefs.high_contrast
report = {}
root = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation"
def check(name, ok):
    report[name] = bool(ok)
    assert ok, name
def damage(source):
    ue.GameplayStatics.apply_damage(pawn, 2, pc, source, ue.DamageType)
def source_at(offset):
    angle = math.radians(pc.get_control_rotation().yaw + offset)
    where = pawn.get_actor_location() + ue.Vector(80 * math.cos(angle), 80 * math.sin(angle), 0)
    return ue.Phase2Tools.test_camera(pawn, where, ue.Rotator())

fixtures = []
try:
    prefs.high_contrast = True
    for name, degrees, text in (("ahead", 0, "ahead"), ("left", -90, "left"),
                                ("right", 90, "right"), ("behind", 180, "behind")):
        source = source_at(degrees)
        assert source
        fixtures.append(source)
        damage(source)
        check(name, session.damage_has_direction and abs(abs(session.damage_angle) - abs(degrees)) < 2
              and text in str(session.damage_cue_text()).lower())
    prefs.high_contrast = False
    damage(fixtures[2])
    check("standard_uses_arrow", "▶" in str(session.damage_cue_text()))
    damage(pawn)
    check("no_false_direction_for_self_damage", session.damage_alpha > 0 and
          not session.damage_has_direction and not str(session.damage_cue_text()))
finally:
    prefs.high_contrast = old_contrast
    for source in fixtures:
        source.destroy_actor()
    (root / "damage_feedback_pie.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
