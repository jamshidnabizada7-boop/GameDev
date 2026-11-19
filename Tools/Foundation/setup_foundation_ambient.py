"""Install an original, seamless wind bed in the foundation sandbox."""
from pathlib import Path
import math
import random
import struct
import wave
import unreal as ue

level = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
assert not level.is_in_play_in_editor()
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith("/Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation.")
root = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
source = root / "SourceAssets/Audio/FoundationWind.wav"
folder = "/Game/AfghanFPS/Audio/Foundation"
name = "FoundationWind"
assets = ue.EditorAssetLibrary
if not assets.does_asset_exist(f"{folder}/{name}"):
    if not source.exists():
        # Integer cycles across the entire loop keep the waveform continuous.
        rng = random.Random(137)
        rate, seconds = 22050, 6
        frames = rate * seconds
        bands = [(rng.randrange(90, 1440), rng.uniform(0, math.tau), rng.uniform(.004, .013))
                 for _ in range(22)]
        samples = bytearray()
        for index in range(frames):
            phase = math.tau * index / frames
            gust = .6 + .25 * math.sin(phase * 2) + .12 * math.sin(phase * 5 + 1)
            pressure = sum(amplitude * math.sin(phase * cycle + offset)
                           for cycle, offset, amplitude in bands)
            samples.extend(struct.pack("<h", int(32767 * gust * pressure)))
        source.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(source), "wb") as out:
            out.setparams((1, 2, rate, frames, "NONE", "not compressed"))
            out.writeframes(samples)
    task = ue.AssetImportTask()
    task.set_editor_properties({"filename": str(source), "destination_path": folder,
                                "automated": True, "replace_existing": False, "save": True})
    ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = assets.load_asset(f"{folder}/{name}")
effects = assets.load_asset(f"{folder}/SC_Effects")
assert sound and effects
sound.set_editor_properties({"looping": True, "sound_class_object": effects})
assert assets.save_loaded_asset(sound, False)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
label = "Foundation_AmbientWind"
matches = [a for a in actors.get_all_level_actors() if a.get_actor_label() == label]
assert len(matches) <= 1
actor = matches[0] if matches else actors.spawn_actor_from_class(ue.AmbientSound, ue.Vector())
assert isinstance(actor, ue.AmbientSound)
actor.set_actor_label(label)
actor.tags = [ue.Name(label)]
component = actor.get_component_by_class(ue.AudioComponent)
component.set_sound(sound)
component.set_editor_properties({"auto_activate": True, "allow_spatialization": False,
                                 "volume_multiplier": .32})
assert level.save_current_level()
print("Foundation wind loop installed and routed to Effects.")
