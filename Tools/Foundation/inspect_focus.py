import unreal
ww=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];pp=unreal.GameplayStatics.get_player_pawn(ww,0);cc=unreal.GameplayStatics.get_player_controller(ww,0)
aa=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ww,unreal.FoundationInteractable) if a.get_actor_label()=='P3_PeekDoor')
hit=unreal.SystemLibrary.line_trace_single(ww,pp.p2_camera.get_world_location(),aa.interaction_point(),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[pp]+[c.get_editor_property('child_actor') for c in pp.get_components_by_class(unreal.ChildActorComponent)],unreal.DrawDebugTrace.NONE,True)
print('FOCUS',pp.interaction_manager.focused_actor,'CAM',pp.p2_camera.get_world_location(),'ROT',cc.get_control_rotation(),'HIT',hit.to_tuple())
cc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(pp.p2_camera.get_world_location(),aa.interaction_point()))
print('CAMROT',pp.p2_camera.get_world_rotation(),'FORWARD',pp.p2_camera.get_forward_vector());print('PROMPT',pp.interaction_manager.prompt)
pp.interaction_manager.begin_use(); print('BEGIN',pp.interaction_manager.focused_actor,pp.interaction_manager.last_result,pp.interaction_manager.using); pp.interaction_manager.end_use()
