"""Exercise AI behavior in a possessed PIE world."""
import json
import os
import traceback
import unreal as ue

world = ue.EditorLevelLibrary.get_pie_worlds(False)[0]
player = ue.GameplayStatics.get_player_pawn(world, 0)
controller = ue.GameplayStatics.get_player_controller(world, 0)
npcs = {npc.get_actor_label(): npc for npc in ue.GameplayStatics.get_all_actors_of_class(world, ue.FoundationNPC)}
guard = npcs['P5_Enemy_Patrol']
room = npcs['P5_Enemy_Room']
room.set_ai_enabled(False)
root = 'C:/GameDev/AfghanistanFPS/Saved/Phase5'
os.makedirs(root, exist_ok=True)
results = {}
steps = []
index = 0
start = None
vec = ue.Vector

def move(actor, x, y, yaw=0):
    actor.character_movement.stop_movement_immediately()
    actor.set_actor_location_and_rotation(vec(x, y, -1.6), ue.Rotator(yaw=yaw), False, True)

def location(v):
    return [v.x, v.y, v.z]

def sample(name):
    hp = next(c for c in player.get_components_by_class(ue.ActorComponent) if 'BPC_Health' in c.get_name())
    results[name] = dict(state=str(guard.state), awareness=guard.awareness, sight=guard.sees_target,
        health=guard.health_value(), player_health=hp.get_editor_property('CurrentHealth'),
        shots=guard.shots_fired, blocked=guard.blocked_shots, footsteps=guard.footsteps,
        arrivals=guard.patrol_arrivals, npc=location(guard.get_actor_location()), known=location(guard.last_known_location),
        heard=location(guard.last_heard_location), hearing_tag=str(guard.last_noise_tag), cover=guard.cover_selections,
        ragdoll=guard.mesh.is_simulating_physics(), armed=bool(guard.ai_weapon.get_editor_property('child_actor')))

def combat_setup():
    guard.set_ai_enabled(False)
    guard.patrol_route = []
    move(guard, 2600, 180)
    move(player, 2250, 180)
    guard.set_ai_enabled(True)

def expose():
    move(player, 3100, 180, 180)
    controller.set_control_rotation(ue.Rotator(yaw=180))

def shoot():
    ue.GameplayStatics.apply_damage(guard, 23, controller, player, None)

def hide():
    global blocker
    g = guard.get_actor_location()
    p = player.get_actor_location()
    blocker = ue.Phase2Tools.test_blocker(player, vec((g.x+p.x)/2,180,20), vec(35,1100,240))
    move(player, 3110, 280)

def kill():
    ue.GameplayStatics.apply_damage(guard, 200, controller, player, None)

steps = [
    ('patrol_before', .2, None), ('patrol_after', 6, None),
    ('outside_cone', .7, combat_setup), ('acquiring', .35, expose), ('combat', .8, None),
    ('occluded', .55, hide), ('occluded_later', .8, None),
    ('stagger', .12, shoot), ('after_stagger', .65, None), ('dead', .7, kill),
    ('respawn', .5, lambda: guard.reset_for_drill()),
]
results['nav_path'] = [location(v) for v in ue.FoundationAITools.path_to(guard, vec(3300,-500,-100))]

def tick(_):
    global index, start
    try:
        t = ue.GameplayStatics.get_time_seconds(world)
        name, delay, action = steps[index]
        if start is None:
            if action:
                action()
            start = t
        if t-start >= delay:
            sample(name)
            index += 1
            start = None
            if index == len(steps):
                ue.unregister_slate_post_tick_callback(handle)
    except Exception:
        results['error'] = traceback.format_exc()
        ue.unregister_slate_post_tick_callback(handle)
    with open(root+'/ai_results.json','w',encoding='utf-8') as out:
        json.dump(results,out,indent=2)

handle = ue.register_slate_post_tick_callback(tick)

