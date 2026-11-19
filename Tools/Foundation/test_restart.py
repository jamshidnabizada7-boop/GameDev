import unreal,time,json,os,traceback
root='C:/GameDev/AfghanistanFPS/Saved/Phase4';os.makedirs(root,exist_ok=True)
output=root+('/restart_reduced_results.json' if globals().get('REDUCED_MOTION_TEST') else '/restart_results.json')
ctx={'start':time.monotonic(),'stage':0};report={}
def now_player():
    worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
    return unreal.GameplayStatics.get_player_pawn(worlds[0],0) if worlds else None
old=now_player();assert old
alive_eye=old.p2_camera.get_world_location().z-old.get_actor_location().z
if globals().get('REDUCED_MOTION_TEST'):
    old.foundation_session.preferences.camera_motion=False
uses_motion=old.foundation_session.preferences.camera_motion
unreal.GameplayStatics.apply_damage(old,150,unreal.GameplayStatics.get_player_controller(unreal.EditorLevelLibrary.get_pie_worlds(False)[0],0),old,unreal.DamageType)
def restart_tick(dt):
    try:
        p=now_player()
        if not p:return
        worlds=unreal.EditorLevelLibrary.get_pie_worlds(False);pc=unreal.GameplayStatics.get_player_controller(worlds[0],0)
        if ctx['stage']==0 and time.monotonic()-ctx['start']>.6:
            report['death']=p.p2_is_dead();report['death_overlay']=str(p.get_editor_property('AmmoHUD').get_editor_property('DeathOverlay').get_visibility())
            report['death_eye_drop']=alive_eye-(p.p2_camera.get_world_location().z-p.get_actor_location().z)
            report['death_motion_matches_setting']=report['death_eye_drop']>10 if uses_motion else abs(report['death_eye_drop'])<1
            unreal.Phase2Tools.test_key(pc,'R',True);ctx['stage']=1;ctx['start']=time.monotonic()
        elif ctx['stage']==1 and p!=old and not p.p2_is_dead() and len(p.equipment.slots)>0:
            unreal.Phase2Tools.test_key(pc,'R',False);unreal.Phase2Tools.test_key(pc,'W',True)
            report['restarted']=True;ctx['stage']=2;ctx['start']=time.monotonic()
            ctx['move_start']=unreal.GameplayStatics.get_time_seconds(worlds[0])
            ctx['spawn_location']=p.get_actor_location()
        elif ctx['stage']==2 and unreal.GameplayStatics.get_time_seconds(worlds[0])-ctx['move_start']>.4:
            v=p.get_velocity();report['movement_speed']=(v.x*v.x+v.y*v.y)**.5;unreal.Phase2Tools.test_key(pc,'W',False)
            report['movement_displacement']=(p.get_actor_location()-ctx['spawn_location']).length()
            hud=p.get_editor_property('AmmoHUD');report['health_text']=str(hud.get_editor_property('HealthText').get_text())
            cls=unreal.load_class(None,'/Game/AfghanFPS/UI/UI_AmmoCounter.UI_AmmoCounter_C')
            report['legacy_hud_count']=len(unreal.WidgetLibrary.get_all_widgets_of_class(worlds[0],cls,True))
            report['dead']=p.p2_is_dead();report['slots']=len(p.equipment.slots)
            report['restart_eye_delta']=abs(alive_eye-(p.p2_camera.get_world_location().z-p.get_actor_location().z))
            report['passed']=report['death'] and report['death_motion_matches_setting'] and report['restart_eye_delta']<1 and report['restarted'] and report['movement_displacement']>40 and report['health_text']=='HP: 100' and report['legacy_hud_count']==1 and not report['dead']
            with open(output,'w') as f:json.dump(report,f,indent=2)
            unreal.unregister_slate_post_tick_callback(ctx['handle']);print('RESTART_REGRESSION_DONE',report)
        if time.monotonic()-ctx['start']>10:raise RuntimeError('Restart stage timed out')
    except Exception:
        unreal.unregister_slate_post_tick_callback(ctx['handle']);report['error']=traceback.format_exc()
        with open(output,'w') as f:json.dump(report,f,indent=2)
        print('RESTART_REGRESSION_ERROR',report)
ctx['handle']=unreal.register_slate_post_tick_callback(restart_tick)
