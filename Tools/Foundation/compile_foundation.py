import unreal,json,os
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
paths=['/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation','/Game/AfghanFPS/Weapons/BP_Weapon_Base',
       '/Game/AfghanFPS/Blueprints/BP_TargetDummy','/Game/AfghanFPS/UI/UI_AmmoCounter','/Game/AfghanFPS/Interaction/BP_TestInteractable']
paths+=unreal.EditorAssetLibrary.list_assets('/Game/AfghanFPS/Interaction/Foundation',recursive=True,include_folder=False)
if unreal.EditorAssetLibrary.does_directory_exist('/Game/AfghanFPS/Weapons/Foundation'):
    paths+=unreal.EditorAssetLibrary.list_assets('/Game/AfghanFPS/Weapons/Foundation',recursive=True,include_folder=False)
results={}
for path in paths:
    asset=unreal.load_asset(path)
    if not isinstance(asset,unreal.Blueprint):continue
    results[path]=bool(unreal.BlueprintEditorLibrary.compile_blueprint(asset))
    if results[path]:unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
dest='C:/GameDev/AfghanistanFPS/Saved/Foundation';os.makedirs(dest,exist_ok=True)
with open(dest+'/blueprint_compilation.json','w') as f:json.dump(results,f,indent=2)
assert all(results.values()),results
print('BLUEPRINTS_COMPILED',len(results))
