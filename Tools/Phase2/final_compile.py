import unreal,json
checkpaths=['/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation','/Game/AfghanFPS/Weapons/BP_Weapon_Base','/Game/AfghanFPS/Blueprints/BP_TargetDummy','/Game/AfghanFPS/UI/UI_AmmoCounter','/Game/AfghanFPS/Interaction/BP_TestInteractable']
compilestatus={}
for path in checkpaths:
 asset=unreal.load_asset(path)
 compilestatus[path]=unreal.BlueprintEditorLibrary.compile_blueprint(asset)
 assert compilestatus[path],path
 unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
json.dump(compilestatus,open('C:/GameDev/AfghanistanFPS/Saved/Phase2/blueprint_compilation.json','w'),indent=2)
print('COMPILE_RESULTS',json.dumps(compilestatus))
print('START',unreal.Phase2Tools.begin_player_test())
