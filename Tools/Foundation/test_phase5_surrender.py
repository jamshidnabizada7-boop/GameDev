import unreal as ue
import os
import json
world=ue.EditorLevelLibrary.get_pie_worlds(False)[0]
p=ue.GameplayStatics.get_player_pawn(world,0)
pc=ue.GameplayStatics.get_player_controller(world,0)
npcs=ue.GameplayStatics.get_all_actors_of_class(world,ue.FoundationNPC)
guard=next(n for n in npcs if n.get_actor_label()=='P5_Enemy_Patrol')
for n in npcs:n.set_ai_enabled(False)
guard.patrol_route=[]
guard.set_actor_location_and_rotation(ue.Vector(2700,400,-1.6),ue.Rotator(yaw=0),False,True)
p.set_actor_location_and_rotation(ue.Vector(2480,400,-1.6),ue.Rotator(yaw=0),False,True)
pc.set_control_rotation(ue.Rotator())
guard.set_ai_enabled(True)
start=ue.GameplayStatics.get_time_seconds(world)
sent=False
captured=False
def watch(_):
    global sent,captured
    elapsed=ue.GameplayStatics.get_time_seconds(world)-start
    if elapsed>.25 and not sent:
        ue.Phase2Tools.test_key(pc,'RightMouseButton',True)
        sent=True
    if elapsed>1.5 and not captured:
        ue.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename=C:/GameDev/AfghanistanFPS/Saved/Phase5/surrender.png -nosuffix')
        captured=True
    if elapsed>2:
        ue.Phase2Tools.test_key(pc,'RightMouseButton',False)
        drops=[a for a in ue.GameplayStatics.get_all_actors_of_class(world,ue.FoundationInteractable) if str(a.stable_id).startswith('RuntimeAIWeapon_')]
        with open('C:/GameDev/AfghanistanFPS/Saved/Phase5/surrender.json','w') as out:
            json.dump(dict(state=str(guard.state),armed=bool(guard.ai_weapon.get_editor_property('child_actor')),pose=guard.p2_surrender_alpha,stance=guard.p2_stance,shots=guard.shots_fired,drops=len(drops)),out,indent=2)
        ue.unregister_slate_post_tick_callback(handle)
handle=ue.register_slate_post_tick_callback(watch)
