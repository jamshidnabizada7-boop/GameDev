"""Phase 3 integration observations, real PIE frames and Enhanced Input key events.
Run after possessed PIE begins. Never substitutes state setters for gameplay results.
"""
import unreal, time, json, os, traceback
p3w=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
p3p=unreal.GameplayStatics.get_player_pawn(p3w,0)
p3pc=unreal.GameplayStatics.get_player_controller(p3w,0)
p3m=p3p.interaction_manager
p3actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(p3w,unreal.Actor)}
p3out={'method':'PIE key events through Enhanced Input; teleport only to arrange test positions','samples':[]}
p3dir='C:/GameDev/AfghanistanFPS/Saved/Phase3';os.makedirs(p3dir,exist_ok=True)
def p3key(k,on=True,n=1):unreal.Phase2Tools.test_key(p3pc,k,on,n)
def p3release():
    for k in ('E','Q','F','LeftControl','MouseWheelAxis','LeftMouseButton'):p3key(k,False)
def p3aim(name,distance=180):
    a=p3actors[name];p3p.p2_set_stance(0);p3p.character_movement.stop_movement_immediately()
    point=a.interaction_point() if isinstance(a,unreal.FoundationInteractable) else a.get_actor_location()
    p3p.set_actor_location(unreal.Vector(point.x-distance,point.y,-1.8),False,True)
    # The character updates its camera in Tick; aim from the settled standing eye,
    # not the previous fixture's camera offset immediately after teleporting.
    eye=unreal.Vector(point.x-distance-13.456,point.y,67.57)
    p3pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,point))
def p3sample(name):
    a=p3m.focused_actor
    r={'name':name,'focus':a.get_actor_label() if a else None,'prompt':str(p3m.prompt),'using':p3m.using,
       'progress':p3m.progress,'mirror':p3m.mirror_active,'peeking':p3m.peeking,'fov':p3p.p2_camera.field_of_view,
       'result':p3m.last_result,'transactions':p3m.completed_transactions,
       'items':{str(k):v for k,v in p3m.owned_items.items()},'frame_ms':unreal.GameplayStatics.get_world_delta_seconds(p3w)*1000,'actors':{}}
    for n in ('P3_ArmoryDoor','P3_PeekDoor','P3_ArmoryKey','P3_Ammo','P3_Health','P3_Weapon','P3_Drawer','P3_Switch','P3_Loot','P3_Breakable'):
        a=p3actors[n];r['actors'][n]={k:a.get_editor_property(k) for k in ('door_angle','locked','listening','consumed','open','broken','light_on','use_count','blocked')}
    hp=p3p.get_component_by_class(unreal.load_class(None,'/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C'))
    weapon=p3p.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
    r.update(health=hp.get_editor_property('CurrentHealth'),ammo=weapon.get_editor_property('CurrentAmmo'),reserve=weapon.get_editor_property('ReserveAmmo'),hud_health=str(p3p.get_editor_property('AmmoHUD').get_editor_property('HealthText').get_text()))
    r['linked_light_visible']=p3actors['P3_LinkedLamp'].get_component_by_class(unreal.PointLightComponent).get_editor_property('visible')
    pos=p3p.get_actor_location();cam=p3p.p2_camera.get_world_location()
    r.update(position=[pos.x,pos.y,pos.z],camera=[cam.x,cam.y,cam.z],lean=p3p.p2_lean)
    p3out['samples'].append(r)
    with open(p3dir+'/runtime_results.json','w') as f:json.dump(p3out,f,indent=2)
def p3shot(name):unreal.SystemLibrary.execute_console_command(p3w,'Shot SHOWUI filename='+p3dir+'/'+name+'.png -nosuffix')
p3stages=[]
def p3stage(name,duration,enter=None,during=None):p3stages.append((name,duration,enter,during))
def p3tap(name):
    p3stage(name+'_press',.12,lambda:p3key('E'))
    p3stage(name,1.3,lambda:p3key('E',False))
def p3pickup(name):
    p3stage(name+'_focus',.5,lambda:p3aim(name))
    p3stage(name+'_hold',1.2,None,lambda:p3key('E'))
    p3stage(name+'_release',.2,lambda:p3key('E',False))
p3stage('locked_focus',.6,lambda:p3aim('P3_ArmoryDoor'))
p3tap('locked_denied')
p3pickup('P3_ArmoryKey')
p3stage('key_door_focus',.5,lambda:p3aim('P3_ArmoryDoor'))
p3tap('key_door_open')
p3stage('listen_focus',.5,lambda:p3aim('P3_PeekDoor'))
p3stage('listening',.8,None,lambda:p3key('E'))
p3stage('listen_release',.4,lambda:p3key('E',False))
p3stage('adjust_begin',.2,lambda:p3key('E'))
p3stage('adjust_wheel',.05,lambda:p3key('MouseWheelAxis',True,2))
p3stage('adjust_settle',.4,lambda:p3key('MouseWheelAxis',False))
p3stage('adjust_release',.3,lambda:p3key('E',False))
p3stage('peek',.7,None,lambda:p3key('F'))
p3stage('peek_release',.6,lambda:p3key('F',False))
p3pickup('P3_Ammo')
p3stage('full_health_focus',.5,lambda:p3aim('P3_Health'))
p3stage('full_health_reject',1.2,None,lambda:p3key('E'))
p3stage('health_release',.2,lambda:p3key('E',False))
p3stage('damage',.3,lambda:unreal.GameplayStatics.apply_damage(p3p,22,p3pc,p3p,unreal.DamageType))
p3pickup('P3_Health')
p3pickup('P3_Weapon')
p3stage('cancel_focus',.5,lambda:p3aim('P3_Loot'))
p3stage('cancel_half',.3,lambda:p3key('E'))
p3stage('cancel_early_release',.5,lambda:p3key('E',False))
p3pickup('P3_Loot')
for p3n in ('P3_Drawer','P3_Switch','P3_Breakable'):
    p3stage(p3n+'_focus',.5,lambda n=p3n:p3aim(n));p3tap(p3n+'_use')
p3stage('overview',.5,lambda:(p3aim('P3_Drawer',220),p3shot('interaction_annex')))
p3state={'index':0,'entered':False,'stable_frames':0,'ready':False}
def p3tick(dt):
    try:
        if not p3state['ready']:
            p3state['stable_frames']=p3state['stable_frames']+1 if unreal.GameplayStatics.get_world_delta_seconds(p3w)<.05 else 0
            if p3state['stable_frames']<20:return
            p3state['ready']=True
        if p3state['index']>=len(p3stages):
            p3release();unreal.unregister_slate_post_tick_callback(p3state['handle']);print('PHASE3_RUNTIME_DONE');return
        name,duration,enter,during=p3stages[p3state['index']]
        if not p3state['entered']:
            p3state['entered']=True;p3state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-p3state['start']>=duration:
            p3sample(name);p3state['index']+=1;p3state['entered']=False
    except Exception:
        p3release();unreal.unregister_slate_post_tick_callback(p3state['handle']);print('PHASE3_RUNTIME_ERROR',traceback.format_exc())
p3state['handle']=unreal.register_slate_post_tick_callback(p3tick)
print('PHASE3_RUNTIME_STARTED')
