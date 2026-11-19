import unreal
p3w=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
p3p=unreal.GameplayStatics.get_player_pawn(p3w,0)
p3pc=unreal.GameplayStatics.get_player_controller(p3w,0)
p3actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(p3w,unreal.Actor)}
p3point=p3actors['P3_PeekDoor'].interaction_point()
p3p.character_movement.stop_movement_immediately()
p3p.set_actor_location(unreal.Vector(p3point.x-180,p3point.y,-1.8),False,True)
p3pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p3p.p2_camera.get_world_location(),p3point))
print('POSE',p3p.get_actor_location(),p3p.p2_camera.get_world_location(),p3pc.get_control_rotation())
for p3dn in ('P3_PeekDoor','P3_ArmoryDoor'):
    p3da=p3actors[p3dn];print(p3dn,p3da.get_actor_location(),p3da.interaction_point(),p3da.panel.get_world_location(),p3da.panel.get_world_scale(),p3da.kind)
print('TRACE',unreal.SystemLibrary.line_trace_single(p3w,p3p.p2_camera.get_world_location(),p3actors['P3_PeekDoor'].interaction_point(),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[p3p],unreal.DrawDebugTrace.NONE,True))
