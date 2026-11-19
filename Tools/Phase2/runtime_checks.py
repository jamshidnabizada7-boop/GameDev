"""Possessed PIE checks using Enhanced Input injection and real physics frames.
Does not claim to test physical keyboard mappings. Results contain raw samples.
"""
import unreal, json, time, os, traceback

world = unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
pc = unreal.GameplayStatics.get_player_controller(world, 0)
p = unreal.GameplayStatics.get_player_pawn(world, 0)
assert p and pc and isinstance(p, unreal.Phase2Character)
actions = {n: unreal.load_asset('/Game/Input/Actions/IA_'+n) for n in
           ('Move','Sprint','Crouch','Prone','LeanLeft','LeanRight','Traversal','Fire','Jump','Restart','Interact')}
results = {'test_type':'possessed PIE; simulated keyboard events through PlayerController and existing Enhanced Input mappings; real physics', 'samples':[]}
key_names={'Move':'W','Sprint':'LeftShift','SlowWalk':'LeftAlt','Crouch':'LeftControl','Prone':'Z','LeanLeft':'Q','LeanRight':'F','Traversal':'G','Fire':'LeftMouseButton','Jump':'SpaceBar','Restart':'R','Interact':'E'}

def inject(name, value=(1,0,0)):
    unreal.Phase2Tools.test_key(pc,key_names[name],True)

def release_keys():
    for key in key_names.values():unreal.Phase2Tools.test_key(pc,key,False)

def place(x, y):
    p.p2_set_stance(0)
    p.character_movement.stop_movement_immediately()
    p.set_actor_location(unreal.Vector(x,y,-1.8),False,True)
    pc.set_control_rotation(unreal.Rotator(0,0,0))

def sample(name):
    pos=p.get_actor_location(); vel=p.get_velocity(); cam=p.p2_camera.get_world_location()
    cap=p.capsule_component
    pelvis=p.mesh.get_socket_location('pelvis'); head=p.mesh.get_socket_location('head')
    hand=p.p2_arms.get_socket_location('hand_r')
    r={'name':name,'position':[pos.x,pos.y,pos.z],'speed':(vel.x*vel.x+vel.y*vel.y)**0.5,
       'stance':p.p2_stance,'half_height':cap.get_scaled_capsule_half_height(),
       'radius':cap.get_scaled_capsule_radius(),'camera':[cam.x,cam.y,cam.z],
       'lean':p.p2_lean,'sprinting':p.p2_sprinting,'crouch_pose':p.p2_pose_crouch,'prone_pose':p.p2_pose_prone,'head':[head.x,head.y,head.z],'pelvis':[pelvis.x,pelvis.y,pelvis.z],
       'right_hand':[hand.x,hand.y,hand.z], 'traversal':p.p2_traversal_result}
    weapon=p.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
    r['ammo']=weapon.get_editor_property('CurrentAmmo'); r['reserve']=weapon.get_editor_property('ReserveAmmo')
    hud=p.get_editor_property('AmmoHUD')
    health=p.get_component_by_class(unreal.load_class(None,'/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C'))
    r['health']=health.get_editor_property('CurrentHealth')
    r['hud_health']=str(hud.get_editor_property('HealthText').get_text())
    results['samples'].append(r)
    print('P2_SAMPLE', json.dumps(r))

# Each stage injects on every engine frame. Release is an actual empty-input frame.
stages=[
 ('settle',0.5,lambda:place(-800,0),None),
 ('walk',1.2,None,lambda:inject('Move',(0,1,0))),
 ('sprint',0.8,lambda:place(-800,0),lambda:(inject('Move',(0,1,0)),inject('Sprint'))),
 ('sprint_release',0.7,lambda:place(-800,0),lambda:inject('Move',(0,1,0))),
 ('slow_walk',0.8,lambda:place(-800,0),lambda:(inject('Move',(0,1,0)),inject('SlowWalk'))),
 ('slow_walk_release',0.8,lambda:place(-800,0),lambda:inject('Move',(0,1,0))),
 ('slow_sprint_conflict',0.8,lambda:place(-800,0),lambda:(inject('Move',(0,1,0)),inject('SlowWalk'),inject('Sprint'))),
 ('crouch',1.0,lambda:place(-800,0),lambda:(inject('Crouch'),inject('Move',(0,1,0)))),
 ('prone_enter',0.2,lambda:inject('Prone'),None),
 ('prone_crawl',1.0,None,lambda:inject('Move',(0,1,0))),
 ('stand_again',0.5,lambda:inject('Prone'),None),
 ('lean_left',0.7,None,lambda:inject('LeanLeft')),
 ('lean_neutral',0.6,None,None),
 ('lean_right',0.7,None,lambda:inject('LeanRight')),
 ('both_lean',0.5,None,lambda:(inject('LeanLeft'),inject('LeanRight'))),
 ('standing_tunnel_block',1.0,lambda:place(-850,-250),lambda:inject('Move',(0,1,0))),
 ('crouch_tunnel_inside',0.8,lambda:place(-850,-250),lambda:(inject('Crouch'),inject('Move',(0,1,0)))),
 ('ceiling_stand_rejected',0.5,None,None),
 ('crouch_tunnel_exit',1.8,None,lambda:inject('Move',(0,1,0))),
 ('prone_tunnel_enter',0.2,lambda:(place(-850,350),inject('Prone')),None),
 ('prone_tunnel_cross',4.2,None,lambda:inject('Move',(0,1,0))),
 ('vault_prepare',0.4,lambda:(p.p2_set_stance(0),place(100,-800)),None),
 ('vault',1.5,lambda:inject('Traversal'),None),
 ('climb_prepare',0.4,lambda:place(375,-800),None),
 ('climb',1.5,lambda:inject('Traversal'),None),
 ('tall_prepare',0.4,lambda:place(650,-800),None),
 ('tall_rejection',0.8,lambda:inject('Traversal'),None),
 ('fire_prepare',0.4,lambda:place(-500,0),None),
 ('fire_1',0.3,lambda:inject('Fire'),None),
 ('fire_2',0.3,lambda:inject('Fire'),None),
 ('ammo_wait',1.0,None,None),
 ('jump',0.3,lambda:inject('Jump'),None),
 ('land',1.4,None,None),
]
state={'index':0,'start':None,'enter':False}

def tick(delta):
    try:
        i=state['index']
        if i>=len(stages):
            release_keys();unreal.unregister_slate_post_tick_callback(state['handle'])
            os.makedirs('C:/GameDev/AfghanistanFPS/Saved/Phase2',exist_ok=True)
            with open('C:/GameDev/AfghanistanFPS/Saved/Phase2/runtime_results.json','w') as f:json.dump(results,f,indent=2)
            print('P2_RUNTIME_DONE')
            return
        name,duration,enter,during=stages[i]
        if not state['enter']:
            state['start']=time.monotonic();state['enter']=True
            if enter:enter()
        if during:during()
        if time.monotonic()-state['start']>=duration:
            sample(name);release_keys();state['index']+=1;state['enter']=False
    except Exception:
        unreal.unregister_slate_post_tick_callback(state['handle'])
        release_keys()
        print('P2_RUNTIME_ERROR',traceback.format_exc())

state['handle']=unreal.register_slate_post_tick_callback(tick)
print('P2_RUNTIME_STARTED')

