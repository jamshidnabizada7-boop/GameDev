"""Check the NPC label's text and size independently of player HUD color."""
from pathlib import Path
import json
import unreal as ue
world, = ue.EditorLevelLibrary.get_pie_worlds(False)
p = ue.GameplayStatics.get_player_pawn(world, 0)
prefs = p.foundation_session.preferences
previous = prefs.high_contrast
prefs.high_contrast = True
npcs = ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationNPC)
assert any(n.friendly for n in npcs)
assert any(not n.friendly for n in npcs)
output = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/high_contrast_labels_pie.json"
due = ue.GameplayStatics.get_time_seconds(world) + .6
def tick(dt):
    if ue.GameplayStatics.get_time_seconds(world) < due:
        return
    try:
        labels = [{"role": "ALLY" if n.friendly else "OPPONENT",
                   "text": str(n.state_label.get_editor_property("text")),
                   "size": n.state_label.get_editor_property("world_size")}
                  for n in npcs]
        result = {"labels": labels,
                  "passed": all(label["role"] in label["text"] and label["size"] >= 19 for label in labels)}
        output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    finally:
        prefs.high_contrast = previous
        ue.unregister_slate_post_tick_callback(handle)
handle = ue.register_slate_post_tick_callback(tick)
