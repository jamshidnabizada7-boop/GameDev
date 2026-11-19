"""Runtime combat/death/clearance regressions. Run in possessed PIE."""
import unreal, json, time, traceback, os

HP=unreal.load_class(None,'/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C')
TARGET=unreal.load_class(None,'/Game/AfghanFPS/Blueprints/BP_TargetDummy.BP_TargetDummy_C')
HUD_CLASS=unreal.load_class(None,'/Game/AfghanFPS/UI/UI_AmmoCounter.UI_AmmoCounter_C')
out={'test_type':'PIE simulated keys through existing mappings; real health, physics and HUD','samples':[]}
ctx={}
def world():return unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
def player():return unreal.GameplayStatics.get_player_pawn(world(),0)
def controller():return unreal.GameplayStatics.get_player_controller(world(),0)
def target():return unreal.GameplayStatics.get_all_actors_of_class(world(),TARGET)[0]
def weapon():return player().get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
def key(k,on=True,amount=1):unreal.Phase2Tools.test_key(controller(),k,on,amount)
def release():
    for k in ('W','A','S','D','LeftShift','LeftControl','Z','Q','F','G','LeftMouseButton','R','E'):key(k,False)
def place(x,y):
    pl=player();pl.p2_set_stance(0);pl.character_movement.stop_movement_immediately()
    pl.set_actor_location(unreal.Vector(x,y,-1.8),False,True)
    controller().set_control_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0))
def aim_target():
    place(200,-600)
    eye=player().p2_camera.get_world_location()
    aim=target().get_actor_location()+unreal.Vector(0,0,25)
    controller().set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,aim))
def shot(name):
    os.makedirs('C:/GameDev/AfghanistanFPS/Saved/Phase2',exist_ok=True)
    unreal.SystemLibrary.execute_console_command(world(),'Shot SHOWUI filename=C:/GameDev/AfghanistanFPS/Saved/Phase2/'+name+'.png -nosuffix')
def snapshot(name):
    pl=player(); hp=pl.get_component_by_class(HP); hud=pl.get_editor_property('AmmoHUD');t=target()
    v=pl.get_velocity();pos=pl.get_actor_location();cam=pl.p2_camera.get_world_location()
    row={'name':name,'pawn':pl.get_name(),'health':hp.get_editor_property('CurrentHealth'),
         'dead':pl.p2_is_dead(),'hud_health':str(hud.get_editor_property('HealthText').get_text()),
         'ammo':weapon().get_editor_property('CurrentAmmo'),'reserve':weapon().get_editor_property('ReserveAmmo'),
         'ammo_text':str(hud.get_editor_property('Team1Score').get_text()),
         'hud_instances_in_viewport':len(unreal.WidgetLibrary.get_all_widgets_of_class(world(),HUD_CLASS,True)),
         'target_health':t.get_component_by_class(HP).get_editor_property('CurrentHealth'),
         'target_physics':[m.is_simulating_physics() for m in t.get_components_by_class(unreal.SkeletalMeshComponent)],
         'hit':weapon().get_editor_property('bLastHitConfirmed'),'lethal':weapon().get_editor_property('bLastHitLethal'),
         'speed':(v.x*v.x+v.y*v.y)**0.5,'position':[pos.x,pos.y,pos.z],
         'camera':[cam.x,cam.y,cam.z],'lean':pl.p2_lean,'stance':pl.p2_stance,
         'control_rotation':str(controller().get_control_rotation()),'traversal':pl.p2_traversal_result,
         'death_overlay':str(hud.get_editor_property('DeathOverlay').get_visibility())}
    out['samples'].append(row);print('P2_REGRESSION',json.dumps(row))
def blocker():
    place(100,-800)
    ctx['blocker']=unreal.Phase2Tools.test_blocker(player(),unreal.Vector(384,-800,10),unreal.Vector(100,150,220))
def remove_blocker():ctx['blocker'].set_actor_enable_collision(False);ctx['blocker'].set_actor_hidden_in_game(True)
def die():
    unreal.GameplayStatics.apply_damage(player(),150,controller(),player(),unreal.DamageType)
def one_round():weapon().set_editor_property('CurrentAmmo',1)
stages=[
 ('aim',0.7,aim_target,None),
 ('target_shot_1',0.4,lambda:key('LeftMouseButton'),None),
 ('target_shot_2',0.4,lambda:key('LeftMouseButton'),None),
 ('target_shot_3',0.5,lambda:key('LeftMouseButton'),None),
 ('target_shot_4',0.5,lambda:key('LeftMouseButton'),None),
 ('empty_setup',0.4,one_round,None),
 ('last_round',0.4,lambda:key('LeftMouseButton'),None),
 ('empty_fire_rejected',0.4,lambda:key('LeftMouseButton'),None),
 ('blocked_landing_setup',0.4,blocker,None),
 ('blocked_landing_rejected',0.6,lambda:key('G'),None),
 ('unblock',0.3,remove_blocker,None),
 ('wall_lean_stand',0.7,lambda:place(-300,-965),lambda:key('Q')),
 ('wall_lean_crouch',0.7,None,lambda:(key('LeftControl'),key('Q'))),
 ('mouse_look',0.6,lambda:place(-500,0),lambda:key('MouseX',True,10)),
 ('death',0.7,die,None),
 ('dead_movement_rejected',0.8,None,lambda:(key('W'),key('LeftShift'),key('F'))),
 ('death_screenshot',0.5,lambda:shot('death'),None),
 ('restart',1.5,lambda:key('R'),None),
 ('restart_sprint',0.8,lambda:place(-800,0),lambda:(key('W'),key('LeftShift'))),
 ('post_restart',0.5,None,None),
]
state2={'index':0,'entered':False,'start':0}
def reg_tick(dt):
    try:
        i=state2['index']
        if i>=len(stages):
            release();unreal.unregister_slate_post_tick_callback(state2['handle'])
            with open('C:/GameDev/AfghanistanFPS/Saved/Phase2/regression_results.json','w') as f:json.dump(out,f,indent=2)
            print('P2_REGRESSION_DONE');return
        name,duration,enter,during=stages[i]
        if not state2['entered']:
            state2['entered']=True;state2['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-state2['start']>=duration:
            snapshot(name);release();state2['index']+=1;state2['entered']=False
    except Exception:
        unreal.unregister_slate_post_tick_callback(state2['handle'])
        print('P2_REGRESSION_ERROR',traceback.format_exc())
        with open('C:/GameDev/AfghanistanFPS/Saved/Phase2/regression_results.json','w') as f:json.dump(out,f,indent=2)
state2['handle']=unreal.register_slate_post_tick_callback(reg_tick)
print('P2_REGRESSION_STARTED')
