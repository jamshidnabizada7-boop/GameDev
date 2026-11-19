import unreal
fw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];fp=unreal.GameplayStatics.get_player_pawn(fw,0)
print('NATIVE',fp.interaction_manager.inspect_focus())
print('FOCUS',fp.interaction_manager.focused_actor,fp.interaction_manager.prompt)
