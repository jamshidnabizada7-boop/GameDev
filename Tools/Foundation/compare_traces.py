import unreal
cw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];cp=unreal.GameplayStatics.get_player_pawn(cw,0);cc=unreal.GameplayStatics.get_player_controller(cw,0)
cp.set_actor_location(unreal.Vector(1626,-45,-1.8),False,True);cc.set_control_rotation(unreal.Rotator(pitch=-15.75,yaw=0,roll=0))
def comparetick(dt):
    try:
        start=cp.p2_camera.get_world_location();direction=cp.p2_camera.get_forward_vector();ignore=[cp]+[c.get_editor_property('child_actor') for c in cp.get_components_by_class(unreal.ChildActorComponent)]
        print('NATIVE',cp.interaction_manager.inspect_focus())
        for dist in (200,250,280):
            h=unreal.SystemLibrary.line_trace_single(cw,start,start+direction*dist,unreal.TraceTypeQuery.ECC_VISIBILITY,True,ignore,unreal.DrawDebugTrace.NONE,True)
            print('COMPARE',dist,h.to_tuple() if h else None)
        aa=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(cw,unreal.FoundationInteractable) if a.get_actor_label()=='P3_PeekDoor')
        print('COLLISION',aa.panel.get_collision_profile_name(),aa.panel.get_collision_enabled(),aa.panel.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY))
        print('DIRECT',unreal.SystemLibrary.line_trace_single(cw,start,aa.interaction_point(),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,unreal.DrawDebugTrace.NONE,True))
        end=start+direction*250
        print('END',end,'POINT',aa.interaction_point(),'DIR',direction)
        for complex_trace in (False,True):
            hit=unreal.SystemLibrary.line_trace_single(cw,start,end,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,complex_trace,ignore,unreal.DrawDebugTrace.NONE,True)
            print('COMPLEX',complex_trace,hit.to_tuple() if hit else None)
    finally:unreal.unregister_slate_post_tick_callback(comparehandle)
comparehandle=unreal.register_slate_post_tick_callback(comparetick)
