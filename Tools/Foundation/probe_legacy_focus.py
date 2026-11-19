import unreal,time
lw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];lp=unreal.GameplayStatics.get_player_pawn(lw,0);lc=unreal.GameplayStatics.get_player_controller(lw,0)
lp.p2_set_stance(0);lp.set_actor_location(unreal.Vector(20,-400,-1.8),False,True)
lc.set_control_rotation(unreal.Rotator(pitch=-31.3,yaw=0,roll=0))
ls={'start':time.monotonic()}
def legacyprobe(dt):
    if time.monotonic()-ls['start']<.6:return
    print('LEGACY_PROBE',lp.interaction_manager.inspect_focus(),lp.interaction_manager.focused_actor)
    lp.interaction_manager.begin_use();lp.interaction_manager.end_use()
    print('LEGACY_RESULT',lp.interaction_manager.last_result)
    unreal.unregister_slate_post_tick_callback(ls['handle'])
ls['handle']=unreal.register_slate_post_tick_callback(legacyprobe)
