"""Add reusable squad and body-hide test fixtures to the foundation map."""
import math
import os
import struct
import wave
import unreal as ue

editor = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
assert not editor.is_in_play_in_editor()
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
placed = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()}
assets = ue.EditorAssetLibrary
friendly = assets.load_blueprint_class('/Game/AfghanFPS/NPC/Foundation/BP_Friendly_Foundation')
assert friendly
if 'P6_Squadmate' not in placed:
    actor = actors.spawn_actor_from_class(friendly, ue.Vector(1400, -780, -3))
    actor.set_actor_label('P6_Squadmate')
    actor.set_editor_property('stable_id', 'P6_Squadmate')

if 'P6_HideArea' not in placed:
    actor = actors.spawn_actor_from_class(ue.TargetPoint, ue.Vector(3100, 400, -100))
    actor.set_actor_label('P6_HideArea')
    actor.set_editor_property('tags', ['BodyHide'])
if 'P6_HideSign' not in placed:
    actor = actors.spawn_actor_from_class(ue.TextRenderActor, ue.Vector(3100, 400, 80))
    actor.set_actor_label('P6_HideSign')
    actor.set_actor_rotation(ue.Rotator(yaw=180), False)
    label = actor.get_component_by_class(ue.TextRenderComponent)
    label.set_text('BODY HIDE AREA')
    label.set_world_size(16)

if not assets.does_asset_exist('/Game/AfghanFPS/Audio/Foundation/Whistle'):
    path = 'C:/GameDev/AfghanistanFPS/SourceAssets/Audio/Whistle.wav'
    os.makedirs(os.path.dirname(path), exist_ok=True)
    rate = 22050
    frames = bytearray()
    for i in range(int(rate*.65)):
        t = i/rate
        contour = math.sin(math.pi*t/.65)**2
        frames.extend(struct.pack('<h', int(8000*contour*math.sin(math.tau*(1100*t+460*t*t)))))
    with wave.open(path,'wb') as out:
        out.setparams((1,2,rate,0,'NONE','not compressed'))
        out.writeframes(frames)
    task = ue.AssetImportTask()
    task.set_editor_properties(dict(filename=path,destination_path='/Game/AfghanFPS/Audio/Foundation',automated=True,save=True))
    ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    assert assets.does_asset_exist('/Game/AfghanFPS/Audio/Foundation/Whistle')

assert editor.save_current_level()
print('PHASE6_FIXTURES_SAVED')
