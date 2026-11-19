"""Verify a visible live frag drives navigation; other threat cases are separate."""
import unreal as u,json,traceback
from pathlib import Path
w,=u.EditorLevelLibrary.get_pie_worlds(False)
guard=next(n for n in u.GameplayStatics.get_all_actors_of_class(w,u.FoundationNPC) if str(n.stable_id)=="P5_Enemy_Patrol")
p=u.GameplayStatics.get_player_pawn(w,0)
p.set_actor_location(u.Vector(-800,-800,0),False,True)
for n in u.GameplayStatics.get_all_actors_of_class(w,u.FoundationNPC):n.set_ai_enabled(False)
guard.set_actor_location_and_rotation(u.Vector(2200,400,0),u.Rotator(yaw=180),False,True)
guard.patrol_route=[]
guard.set_ai_enabled(True)
# Use the runtime world spawn API exposed by this engine build.
throw=u.get_default_object(u.load_class(None,"/Script/Engine.GameplayStatics"))
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/"Foundation/grenade_evasion_pie.json"
# EditorActorSubsystem cannot spawn actors in PIE. Duplicate a runtime object via
# the existing equipment throw workflow instead.
pc=u.GameplayStatics.get_player_controller(w,0)
p.set_actor_location(u.Vector(2000,400,0),False,True)
pc.set_control_rotation(u.Rotator(yaw=0,pitch=-25))
start=u.GameplayStatics.get_time_seconds(w)
initial=guard.get_actor_location()
phase=0
report={}
def tick(dt):
    global phase
    try:
        elapsed=u.GameplayStatics.get_time_seconds(w)-start
        if phase==0:
            u.Phase2Tools.test_key(pc,"T",True);phase=1;return
        if phase==1 and elapsed>.15:
            u.Phase2Tools.test_key(pc,"T",False);phase=2;return
        grenades=[g for g in u.GameplayStatics.get_all_actors_of_class(w,u.FoundationThrowable) if g.kind==u.FoundationThrowableKind.FRAG and g.launched and not g.detonated]
        if phase==2 and grenades:
            g=grenades[0];g.body.set_simulate_physics(False)
            g.set_actor_location(guard.get_actor_location()+u.Vector(-180,0,30),False,True)
            g.set_editor_property("remaining_fuse",12.)
            report["arranged_visible_live_frag"]=True
            phase=3
        if phase==3:
            report["evading"]=report.get("evading",False) or guard.evading_grenade
            report["paths"]=guard.grenade_escapes
            report["travel_cm"]=(guard.get_actor_location()-initial).length()
        if elapsed<5:return
        report["passed"]=bool(report.get("arranged_visible_live_frag") and report.get("evading") and report.get("paths",0)>0 and report.get("travel_cm",0)>100)
    except Exception:report["error"]=traceback.format_exc()
    u.Phase2Tools.test_key(pc,"T",False)
    out.write_text(json.dumps(report,indent=2))
    u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
