"""Rebuild only the sandbox navigation volume and save after generation finishes."""
from pathlib import Path
import json
import time
import unreal as ue

level = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
assert not level.is_in_play_in_editor()
assert world.get_path_name().startswith("/Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation.")
result = ue.FoundationAITools.build_arena_navigation(world)
assert result.startswith("NAVIGATION_BUILD_REQUESTED"), result
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors()
volume, = [a for a in actors if a.actor_has_tag("FoundationNav")]
box = volume.get_component_by_class(ue.BoxComponent)
assert box and box.get_name() == "FoundationRuntimeNavBounds"
extent = box.get_scaled_box_extent()
assert [extent.x, extent.y, extent.z] == [2400., 1100., 400.]
path = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/nav_bounds_edit.json"
started = time.monotonic()
def finish_when_ready(delta_time):
    waited = time.monotonic() - started
    if waited < 1:
        return
    building = ue.NavigationSystemV1.is_navigation_being_built(world)
    if building and waited < 30:
        return
    ue.unregister_slate_post_tick_callback(handle)
    saved = not building and level.save_current_level()
    report = {"engine_result": result, "component": box.get_name(), "extent": [extent.x, extent.y, extent.z], "map_saved": saved}
    path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    assert saved, report
handle = ue.register_slate_post_tick_callback(finish_when_ready)
