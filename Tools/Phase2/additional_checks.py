import unreal,time,json,traceback
aw=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
ac=unreal.GameplayStatics.get_player_controller(aw,0)
ap=unreal.GameplayStatics.get_player_pawn(aw,0)
ic=unreal.load_class(None,'/Game/AfghanFPS/Interaction/BP_TestInteractable.BP_TestInteractable_C')
ia=unreal.GameplayStatics.get_all_actors_of_class(aw,ic)[0]
ar=[]
def ak(k,on=True):unreal.Phase2Tools.test_key(ac,k,on)
def arls():
    for k in ('W','S','A','D','Z','E','LeftShift'):ak(k,False)
def apl(x,y):
    ap.character_movement.stop_movement_immediately()
    ap.set_actor_location(unreal.Vector(x,y,-1.8),False,True)
    ap.p2_set_stance(0)
    ap.set_actor_location(unreal.Vector(x,y,-1.8),False,True)
    ac.set_control_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0))
def aaim():
    apl(70,-400)
    ac.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(ap.p2_camera.get_world_location(),ia.get_actor_location()))
def asnap(n):
    q=ap.get_actor_location();v=ap.get_velocity()
    r={'name':n,'position':[q.x,q.y,q.z],'speed':(v.x*v.x+v.y*v.y)**0.5,
       'stance':ap.p2_stance,'sprint':ap.p2_sprinting,'interactable_big':ia.get_editor_property('bBig'),
       'interactable_scale':str(ia.get_actor_scale3d()),'half_height':ap.capsule_component.get_scaled_capsule_half_height()}
    ar.append(r);print('P2_ADDITIONAL',json.dumps(r))
def afinalshot():
    apl(-500,0)
    unreal.SystemLibrary.execute_console_command(aw,'Shot SHOWUI filename=C:/GameDev/AfghanistanFPS/Saved/Phase2/first_person_final.png -nosuffix')
astages=[
 ('standing_passage',1.3,lambda:apl(-850,-800),lambda:ak('W')),
 ('interact_aim',0.5,aaim,None),
 ('interact_grow',0.4,lambda:ak('E'),None),
 ('interact_shrink',0.4,lambda:ak('E'),None),
 ('prone_prepare',0.4,lambda:(apl(-850,350),ak('Z')),None),
 ('prone_under_roof',2.1,None,lambda:ak('W')),
 ('prone_stand_blocked',0.6,lambda:ak('Z'),None),
 ('prone_shift_stays_100',1.0,None,lambda:(ak('W'),ak('LeftShift'))),
 ('left',0.4,lambda:apl(-600,0),lambda:ak('A')),
 ('right',0.4,None,lambda:ak('D')),
 ('backwards',0.4,None,lambda:ak('S')),
 ('final_camera_settle',0.6,lambda:apl(-500,0),None),
 ('final_screenshot',0.6,afinalshot,None),
]
astate={'i':0,'entered':False,'start':0}
def atick(dt):
    try:
        if astate['i']>=len(astages):
            arls();unreal.unregister_slate_post_tick_callback(astate['h'])
            json.dump(ar,open('C:/GameDev/AfghanistanFPS/Saved/Phase2/additional_results.json','w'),indent=2)
            print('P2_ADDITIONAL_DONE');return
        n,d,en,during=astages[astate['i']]
        if not astate['entered']:
            astate['entered']=True;astate['start']=time.monotonic()
            if en:en()
        if during:during()
        if time.monotonic()-astate['start']>=d:
            asnap(n);arls();astate['i']+=1;astate['entered']=False
    except Exception:
        unreal.unregister_slate_post_tick_callback(astate['h'])
        print('P2_ADDITIONAL_ERROR',traceback.format_exc())
astate['h']=unreal.register_slate_post_tick_callback(atick)
print('P2_ADDITIONAL_STARTED')
