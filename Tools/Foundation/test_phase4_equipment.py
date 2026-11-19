"""Equipment integration: real Enhanced Input events, bounded fixture arrangements."""
import unreal,time,json,os,traceback,math
ew=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];ep=unreal.GameplayStatics.get_player_pawn(ew,0);ec=unreal.GameplayStatics.get_player_controller(ew,0);eq=ep.equipment
eroot='C:/GameDev/AfghanistanFPS/Saved/Phase4';os.makedirs(eroot,exist_ok=True)
eres={'method':'possessed PIE; generated keys through IMC; ammo setters only arrange independent edge scenarios','samples':[]};ectx={}
def ek(k,on=True,n=1):unreal.Phase2Tools.test_key(ec,k,on,n)
def erel():
    for k in ('LeftMouseButton','RightMouseButton','E','R','B','One','Two','X','H','I','MouseX'):ek(k,False)
def eweapon():return ep.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
def eammo(mag,reserve):
    eweapon().set_editor_properties({'CurrentAmmo':mag,'ReserveAmmo':reserve});eq.sync_ammo()
def eplace():
    ep.p2_set_stance(0);ep.character_movement.stop_movement_immediately();ep.set_actor_location(unreal.Vector(2600,700,-1.8),False,True);ec.set_control_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0))
def eaim(actor,distance=180):
    point=actor.interaction_point() if isinstance(actor,unreal.FoundationInteractable) else actor.get_actor_location();ep.p2_set_stance(0);ep.character_movement.stop_movement_immediately()
    x=max(-900,min(3300,point.x-distance));y=max(-900,min(900,point.y))
    ep.set_actor_location(unreal.Vector(x,y,-1.8),False,True)
    yaw=math.atan2(point.y-y,point.x-x)
    eye=unreal.Vector(x-13.456*math.cos(yaw),y-13.456*math.sin(yaw),67.57)
    ec.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,point))
def eactor(label):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.Actor) if a.get_actor_label()==label)
def edropaim():
    a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.FoundationInteractable) if str(a.item_id)=='weapon_rifle_alt' and not a.consumed)
    ectx['dropped']=a;eaim(a)
def dropped_base():
    a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.FoundationInteractable) if str(a.item_id)=='weapon_rifle_base' and not a.consumed)
    eaim(a)
def eshot(name):unreal.SystemLibrary.execute_console_command(ew,'Shot SHOWUI filename='+eroot+'/'+name+'.png -nosuffix')
def esample(name):
    p=unreal.GameplayStatics.get_player_pawn(unreal.EditorLevelLibrary.get_pie_worlds(False)[0],0)
    q=p.equipment;w=p.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor');r={'name':name,
       'active':q.active_slot,'slots':[{'id':str(s.definition.weapon_id),'mag':s.magazine,'reserve':s.reserve,'mode':str(s.mode)} for s in q.slots],
       'reload':q.reloading,'equipping':q.equipping,'inspect':q.inspecting,'holstered':q.holstered,'ads':q.ads_alpha,'fov':p.p2_camera.field_of_view,
       'spread':q.spread_degrees,'muzzle_blocked':q.muzzle_blocked,'shots':q.shots_fired,'dry':q.dry_clicks,'last':q.last_action,
       'ammo':w.get_editor_property('CurrentAmmo') if w else None,'reserve':w.get_editor_property('ReserveAmmo') if w else None,
       'weapon_class':w.get_class().get_name() if w else None,'summary':q.summary(),'dead':p.p2_is_dead(),
       'yaw':ec.get_control_rotation().yaw,
       'frame_ms':unreal.GameplayStatics.get_world_delta_seconds(ew)*1000}
    hud=p.get_editor_property('AmmoHUD');r['crosshair']=str(hud.get_editor_property('CrosshairText').get_visibility())
    r['ammo_text']=str(hud.get_editor_property('Team1Score').get_text())
    r['look_metrics']={k:v for k,v in ectx.items() if k.endswith(('_delta','_frames'))}
    eres['samples'].append(r)
    with open(eroot+'/'+eres.get('filename','equipment_results.json'),'w') as f:json.dump(eres,f,indent=2)
st=[]
def step(n,t,f=None,d=None):st.append((n,t,f,d))
def tap(n,k,t=.4):step(n+'_press',.07,lambda:ek(k));step(n,t,lambda:ek(k,False))
def look_begin(label):ectx[label+'_start']=ec.get_control_rotation().yaw;ectx[label+'_frames']=0
def look_feed(label):ek('MouseX',True,.2);ectx[label+'_frames']+=1
def look_end(label):
    ek('MouseX',False);ectx[label+'_delta']=(ec.get_control_rotation().yaw-ectx[label+'_start']+180)%360-180
step('initial',.8,eplace)
step('semi_hold',.7,None,lambda:ek('LeftMouseButton'))
step('semi_release',.25,lambda:ek('LeftMouseButton',False))
tap('second_semi','LeftMouseButton')
step('reload_start',.2,lambda:ek('R'))
step('reload_fire_rejected',.4,lambda:ek('R',False),lambda:ek('LeftMouseButton'))
step('reload_complete',1.5,lambda:ek('LeftMouseButton',False))
tap('full_reload_rejected','R')
step('no_reserve_arrange',.2,lambda:eammo(27,0));tap('no_reserve_reload_rejected','R')
step('limited_arrange',.2,lambda:eammo(3,4));tap('limited_reload','R',1.8)
step('empty_arrange',.2,lambda:eammo(0,0));tap('dry_click','LeftMouseButton')
step('mode_arrange',.2,lambda:eammo(30,90));tap('auto_mode','B')
step('auto_hold',.8,None,lambda:ek('LeftMouseButton'))
step('auto_release_applied',.12,lambda:ek('LeftMouseButton',False));step('auto_release',.4)
tap('burst_mode','B');step('burst_press',.06,lambda:ek('LeftMouseButton'));step('burst_release',.8,lambda:ek('LeftMouseButton',False))
tap('semi_mode','B')
step('rapid_initial',.2)
for rapid_index in range(6):
    step('rapid_press_'+str(rapid_index),.01,lambda:ek('LeftMouseButton'))
    step('rapid_release_'+str(rapid_index),.01,lambda:ek('LeftMouseButton',False))
step('rapid_complete',.3)
step('ads_hold',.8,None,lambda:ek('RightMouseButton'));step('ads_image',.3,lambda:eshot('ads'),lambda:ek('RightMouseButton'))
step('ads_release',.8,lambda:ek('RightMouseButton',False))
step('hip_look_prepare',.2,lambda:(eplace(),look_begin('hip')))
step('hip_look',.5,None,lambda:look_feed('hip'));step('hip_look_end',.15,lambda:look_end('hip'))
step('ads_look_prepare',.8,lambda:(eplace(),ek('RightMouseButton'),look_begin('ads')))
step('ads_look',.5,None,lambda:look_feed('ads'));step('ads_look_end',.15,lambda:look_end('ads'))
step('look_release',.7,lambda:ek('RightMouseButton',False))
step('pickup_focus',.6,lambda:eaim(eactor('P3_Weapon')))
step('pickup_hold',1.2,None,lambda:ek('E'));step('pickup_release',.3,lambda:ek('E',False))
step('save_base_ammo',.2,lambda:eammo(12,34));tap('switch_alt','Two',.6)
step('alt_image',.5,lambda:eshot('alternate'))
tap('switch_base','One',.6);tap('alt_again','Two',.6)
tap('drop_alt','X',.6);step('dropped_focus',.6,edropaim)
step('reclaim_hold',1.2,None,lambda:ek('E'));step('reclaim_release',.3,lambda:ek('E',False));tap('reclaimed_alt','Two',.6)
tap('holster','H',.5);tap('holster_fire_rejected','LeftMouseButton');tap('draw','H',.5)
step('inspect_begin',.2,lambda:ek('I'));step('inspect_fire_rejected',.5,lambda:ek('I',False),lambda:ek('LeftMouseButton'))
step('inspect_end',1.0,lambda:ek('LeftMouseButton',False))
tap('alternate_fire','LeftMouseButton');tap('alternate_reload','R',1.8)
tap('drop_all_alt','X',.6);tap('drop_all_base','X',.6)
step('unarmed_ads_rejected',.6,None,lambda:ek('RightMouseButton'));step('unarmed_release',.2,lambda:ek('RightMouseButton',False))
step('base_recover_focus',.6,dropped_base);step('base_recover_hold',1.2,None,lambda:ek('E'));step('base_recovered',.6,lambda:ek('E',False))
state={'i':0,'entered':False,'stable':0,'ready':False}
def equipment_tick(dt):
    try:
        if not state['ready']:
            state['stable']=state['stable']+1 if unreal.GameplayStatics.get_world_delta_seconds(ew)<.05 else 0
            if state['stable']<20:return
            state['ready']=True
        if state['i']>=len(st):erel();unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_EQUIPMENT_DONE');return
        n,t,enter,during=st[state['i']]
        if not state['entered']:
            state['entered']=True;state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-state['start']>=t:esample(n);state['i']+=1;state['entered']=False
    except Exception:
        erel();unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_EQUIPMENT_ERROR',traceback.format_exc())
state['handle']=unreal.register_slate_post_tick_callback(equipment_tick)
print('PHASE4_EQUIPMENT_STARTED')
