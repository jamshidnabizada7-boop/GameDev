"""Drive tactical actions through the possessed player's inputs."""
import json
import os
import traceback
import unreal as ue

world = ue.EditorLevelLibrary.get_pie_worlds(False)[0]
player = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
actors = {a.get_actor_label():a for a in ue.GameplayStatics.get_all_actors_of_class(world,ue.Actor)}
guard = actors['P5_Enemy_Patrol']
other = actors['P5_Enemy_Room']
friend = actors['P6_Squadmate']
for npc in (guard,other,friend):
    npc.set_ai_enabled(False)
directory = 'C:/GameDev/AfghanistanFPS/Saved/Phase6'
os.makedirs(directory,exist_ok=True)
data = {}
v = ue.Vector

def place(actor,x,y,yaw=0):
    actor.character_movement.stop_movement_immediately()
    actor.set_actor_location_and_rotation(v(x,y,-1.6),ue.Rotator(yaw=yaw),False,True)

def aim(x,y,z):
    eye = player.p2_camera.get_world_location()
    pc.set_control_rotation(ue.MathLibrary.find_look_at_rotation(eye,v(x,y,z)))

def key(name,down=True):
    ue.Phase2Tools.test_key(pc,name,down)

def damage(npc,amount):
    ue.GameplayStatics.apply_damage(npc,amount,pc,player,None)

def vec(point):
    return [point.x,point.y,point.z]

def sample(name):
    data[name] = dict(guard=str(guard.state), hp=guard.health_value(), player=vec(player.get_actor_location()),
        tactical=player.tactical_target.get_actor_label() if player.tactical_target else None,
        hostage=player.holding_hostage, using=player.interaction_manager.using, progress=player.interaction_manager.progress,
        prompt=str(player.interaction_manager.prompt), guard_pos=vec(guard.get_actor_location()),
        pelvis=vec(guard.mesh.get_socket_location('pelvis')), hidden=guard.body_hidden,
        speed=player.character_movement.max_walk_speed, shots=player.equipment.shots_fired,
        friendly=str(friend.state), friendly_hp=friend.health_value(), friendly_pos=vec(friend.get_actor_location()),
        order=str(friend.squad_order), goal=vec(friend.move_goal), move_accepted=friend.last_move_accepted,
        noise=str(other.last_noise_tag), hearing=vec(other.last_heard_location), heard=other.heard_events)
    with open(directory+'/tactical_results.json','w',encoding='utf-8') as out:
        json.dump(data,out,indent=2)

steps = []
def step(name,delay,action=None):
    steps.append((name,delay,action))
def tap(name,k,delay=.45):
    step(name+'_down',.34,lambda:key(k))
    step(name,delay,lambda:key(k,False))

step('front_place',.55,lambda:(place(guard,2700,400),place(player,2810,400,180)))
step('front_aim',.55,lambda:aim(2700,400,62))
step('front_e',1.3,lambda:key('E'))
step('rear_place',.6,lambda:(key('E',False),place(player,2590,400)))
step('rear_aim',.55,lambda:aim(2700,400,62))
tap('hostage','U')
tap('hostage_fire_denied','LeftMouseButton')
tap('release','U')
step('restraint_place',.55,lambda:place(player,guard.get_actor_location().x-105,guard.get_actor_location().y))
step('restraint_aim',.55,lambda:aim(*vec(guard.mesh.get_socket_location('spine_03'))))
step('restrained',1.35,lambda:key('E'))
step('restrain_release',.6,lambda:key('E',False))
def aim_body():
    p=guard.mesh.get_socket_location('pelvis')
    place(player,p.x-100,p.y)
step('body_place',.55,aim_body)
step('body_aim',.55,lambda:aim(*vec(guard.mesh.get_socket_location('pelvis'))))
step('drag',.65,lambda:key('E'))
step('drag_moved',1.1,lambda:key('S'))
step('drag_released',.55,lambda:(key('S',False),key('E',False)))
step('friendly_down',.55,lambda:(place(friend,2800,640,180),place(player,2660,640),damage(friend,200)))
step('friendly_aim',.55,lambda:aim(*vec(friend.mesh.get_socket_location('pelvis'))))
step('revive_progress',1.5,lambda:key('E'))
step('revived',2.2)
step('revive_release',.6,lambda:key('E',False))
step('follow_setup',.6,lambda:(place(player,3200,850),friend.set_ai_enabled(True)))
tap('follow','Five',3.1)
tap('hold','Six',1.1)
step('move_aim',.55,lambda:aim(2900,800,-98))
tap('move','MiddleMouseButton',2.6)
tap('cease','Nine')
step('whistle_setup',.55,lambda:(place(other,2840,200,180),other.set_ai_enabled(True),place(player,3250,230,0)))
tap('whistle','K',1.2)

index = 0
start = None
def tick(_):
    global index,start
    try:
        at = ue.GameplayStatics.get_time_seconds(world)
        name,delay,action = steps[index]
        if start is None:
            if action:action()
            start=at
        if at-start>=delay:
            sample(name)
            index+=1
            start=None
            if index==len(steps):
                ue.unregister_slate_post_tick_callback(handle)
    except Exception:
        with open(directory+'/error.txt','w') as f:f.write(traceback.format_exc())
        ue.unregister_slate_post_tick_callback(handle)
handle=ue.register_slate_post_tick_callback(tick)

