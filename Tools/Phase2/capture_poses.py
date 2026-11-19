"""Capture visible Manny poses and their shadows in the possessed PIE world."""
import unreal,time,os,json,traceback
pose_world=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
pose_pc=unreal.GameplayStatics.get_player_controller(pose_world,0)
pose_player=unreal.GameplayStatics.get_player_pawn(pose_world,0)
pose_player.p2_set_stance(0)
pose_player.character_movement.stop_movement_immediately()
pose_player.set_actor_location(unreal.Vector(-400,0,-1.8),False,True)
pose_pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0))
pose_cam=unreal.Phase2Tools.test_camera(pose_player,unreal.Vector(-240,-230,55),
    unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-240,-230,55),unreal.Vector(-400,0,-15)))
assert pose_cam
pose_pc.set_view_target_with_blend(pose_cam,0)
pose_player.mesh.set_owner_no_see(False)
pose_player.p2_arms.set_visibility(False)
pose_weapon=pose_player.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
pose_weapon.set_actor_hidden_in_game(True)

def pose_key(k,on=True):unreal.Phase2Tools.test_key(pose_pc,k,on)
def pose_shot(name):
    unreal.SystemLibrary.execute_console_command(pose_world,'Shot SHOWUI filename=C:/GameDev/AfghanistanFPS/Saved/Phase2/'+name+'.png -nosuffix')
def fp_view():
    pose_player.p2_set_stance(0);pose_pc.set_view_target_with_blend(pose_player,0)
    pose_player.mesh.set_owner_no_see(True);pose_player.p2_arms.set_visibility(True);pose_weapon.set_actor_hidden_in_game(False)

pose_stages=[
 ('stand_ready',0.8,None,None),
 ('standing_body',0.6,lambda:pose_shot('standing_body'),None),
 ('crouch_ready',1.0,None,lambda:pose_key('LeftControl')),
 ('crouching_body',0.6,lambda:pose_shot('crouching_body'),lambda:pose_key('LeftControl')),
 ('prone_ready',1.0,lambda:(pose_key('LeftControl',False),pose_key('Z')),None),
 ('prone_body',0.6,lambda:(pose_key('Z',False),pose_shot('prone_body')),None),
 ('crawl_ready',0.8,None,lambda:pose_key('W')),
 ('crawling_body',0.6,lambda:pose_shot('crawling_body'),lambda:pose_key('W')),
 ('fp_ready',0.8,lambda:(pose_key('W',False),fp_view()),None),
 ('first_person',0.6,lambda:pose_shot('first_person'),None),
]
pose_state={'i':0,'start':0,'entered':False}
def pose_tick(dt):
    try:
        i=pose_state['i']
        if i>=len(pose_stages):
            unreal.unregister_slate_post_tick_callback(pose_state['handle'])
            print('P2_POSES_DONE');return
        name,dur,enter,during=pose_stages[i]
        if not pose_state['entered']:
            pose_state['entered']=True;pose_state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-pose_state['start']>dur:
            pose_state['i']+=1;pose_state['entered']=False
    except Exception:
        unreal.unregister_slate_post_tick_callback(pose_state['handle'])
        print('P2_POSES_ERROR',traceback.format_exc())
pose_state['handle']=unreal.register_slate_post_tick_callback(pose_tick)
print('P2_POSES_STARTED')
