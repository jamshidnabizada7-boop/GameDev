"""Check the map owns one active, looping ambient source throughout PIE."""
import json
from pathlib import Path
import unreal as ue
world, = ue.EditorLevelLibrary.get_pie_worlds(False)
actors = [a for a in ue.GameplayStatics.get_all_actors_of_class(world, ue.AmbientSound)
          if a.actor_has_tag("Foundation_AmbientWind")]
assert len(actors) == 1, len(actors)
component = actors[0].get_component_by_class(ue.AudioComponent)
sound = component.get_editor_property("sound")
start = ue.GameplayStatics.get_time_seconds(world)
report = dict(sound=sound.get_name(), playing_at_start=component.is_playing(),
              looping=sound.get_editor_property("looping"),
              effects_class=sound.get_editor_property("sound_class_object").get_name())
output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/ambient_pie.json"
def tick(dt):
    elapsed = ue.GameplayStatics.get_time_seconds(world) - start
    if elapsed < 7:
        return
    report.update(elapsed=elapsed, playing_after_one_loop=component.is_playing())
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
handle = ue.register_slate_post_tick_callback(tick)
