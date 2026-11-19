"""An input-triggered sound should reach both nearby guards and no distant guard."""
import json, time, traceback
from pathlib import Path
import unreal as u
w,=u.EditorLevelLibrary.get_pie_worlds(False)
p=u.GameplayStatics.get_player_pawn(w,0)
pc=u.GameplayStatics.get_player_controller(w,0)
origin=p.get_actor_location()
p.set_actor_location(u.Vector(1600,300,0),False,True)
pos=p.get_actor_location()
existing=u.GameplayStatics.get_all_actors_of_class(w,u.FoundationNPC)
initial_enabled=[n.ai_enabled for n in existing]
for n in existing:n.set_ai_enabled(False)
guards=[u.AIHelperLibrary.spawn_ai_from_class(w,u.FoundationNPC,None,pos+u.Vector(x,y,60),u.Rotator(yaw=0),True)
        for x,y in ((400,250),(650,-250),(6500,0))]
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/"Foundation/group_hearing_pie.json"
start=u.GameplayStatics.get_time_seconds(w)
pressed=False
released=False
heard=set()
report={}
def tick(dt):
    global pressed,released
    try:
        elapsed=u.GameplayStatics.get_time_seconds(w)-start
        if elapsed<1:return
        if not pressed:
            report["shots_before"]=p.equipment.shots_fired
            u.Phase2Tools.test_key(pc,"LeftMouseButton",True);pressed=True;return
        if not released:u.Phase2Tools.test_key(pc,"LeftMouseButton",False);released=True
        for i,n in enumerate(guards):
            if str(n.last_noise_tag)=="Gunshot" and (pos-n.last_heard_location).length()<200:heard.add(i)
        if elapsed<3:return
        report.update(heard=sorted(heard),shots_after=p.equipment.shots_fired)
        report["passed"]=heard=={0,1} and report["shots_after"]==report["shots_before"]+1
    except Exception:report["error"]=traceback.format_exc()
    out.write_text(json.dumps(report,indent=2))
    for n in guards:
        if n:n.destroy_actor()
    for n,en in zip(existing,initial_enabled):n.set_ai_enabled(en)
    p.set_actor_location(origin,False,True)
    u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
