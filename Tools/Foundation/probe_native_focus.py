import unreal
fw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];fp=unreal.GameplayStatics.get_player_pawn(fw,0);fc=unreal.GameplayStatics.get_player_controller(fw,0)
fp.set_actor_location(unreal.Vector(1626,-45,-1.8),False,True)
fc.set_control_rotation(unreal.Rotator(pitch=-15.75,yaw=0,roll=0))
unreal.SystemLibrary.execute_console_command(fw,'t.IdleWhenNotForeground 0')
unreal.SystemLibrary.execute_console_command(fw,'Slate.bAllowThrottling 0')
print('NATIVE',fp.interaction_manager.inspect_focus())
