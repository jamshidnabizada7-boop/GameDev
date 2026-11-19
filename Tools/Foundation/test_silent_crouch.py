import unreal as u,json,traceback
from pathlib import Path
w,=u.EditorLevelLibrary.get_pie_worlds(False)
p=u.GameplayStatics.get_player_pawn(w,0);pc=u.GameplayStatics.get_player_controller(w,0)
guard=next(n for n in u.GameplayStatics.get_all_actors_of_class(w,u.FoundationNPC) if str(n.stable_id)=="P5_Enemy_Patrol")
for n in u.GameplayStatics.get_all_actors_of_class(w,u.FoundationNPC):n.set_ai_enabled(False)
guard.set_actor_location_and_rotation(u.Vector(1900,650,0),u.Rotator(yaw=180),False,True)
guard.patrol_route=[];guard.set_ai_enabled(True);guard.character_movement.disable_movement()
p.set_actor_location(u.Vector(2000,650,0),False,True);pc.set_control_rotation(u.Rotator())
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/"Foundation/silent_crouch_pie.json"
start=u.GameplayStatics.get_time_seconds(w);phase=0;report={}
keys=["W","LeftControl","LeftAlt"]
def tick(dt):
    global start,phase
    try:
        t=u.GameplayStatics.get_time_seconds(w)
        if phase==0:
            u.Phase2Tools.test_key(pc,"LeftControl",True);u.Phase2Tools.test_key(pc,"LeftAlt",True)
            start=t;phase=1;return
        if phase==1 and t-start>.5:
            report["quiet_heard_start"]=guard.heard_events;report["quiet_steps_start"]=p.p2_footsteps
            u.Phase2Tools.test_key(pc,"W",True);phase=2;start=t;return
        if phase==2 and t-start>2.:
            u.Phase2Tools.test_key(pc,"W",False)
            report["quiet_heard_end"]=guard.heard_events;report["quiet_steps_end"]=p.p2_footsteps
            for k in keys:u.Phase2Tools.test_key(pc,k,False)
            p.set_actor_location(u.Vector(2000,650,0),False,True);phase=3;start=t;return
        if phase==3 and t-start>.6:
            report["walk_heard_start"]=guard.heard_events
            u.Phase2Tools.test_key(pc,"W",True);phase=4;start=t;return
        if phase!=4 or t-start<1.5:return
        report["walk_heard_end"]=guard.heard_events
        report["passed"]=(report["quiet_steps_end"]>report["quiet_steps_start"] and report["quiet_heard_end"]==report["quiet_heard_start"] and report["walk_heard_end"]>report["walk_heard_start"])
    except Exception:report["error"]=traceback.format_exc()
    for k in keys:u.Phase2Tools.test_key(pc,k,False)
    out.write_text(json.dumps(report,indent=2));u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
