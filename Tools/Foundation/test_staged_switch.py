"""Observe old/new slots and mount position while the real selection input runs."""
from pathlib import Path
import unreal as u
exec(compile((Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))/"Tools/Foundation/test_dynamic_grips.py").read_text(),"dynamic_fixture","exec"))
st.clear(); state["ready"]=True
timeline=[]
mount=next(c for c in ep.get_components_by_class(u.SceneComponent) if "FP_WeaponMount" in c.get_name())
out=folder.parents[1]/"Saved/Foundation/staged_switch_pie.json"
def observe():
    timeline.append(dict(time=time.monotonic(),slot=eq.active_slot,z=mount.relative_location.z,
        pitch=mount.relative_rotation.pitch,equipping=eq.equipping,shots=eq.shots_fired))
def done():
    perf.set_editor_property("bThrottleCPUWhenNotForeground",old)
    first=timeline[0]["time"]
    rows=[dict(r,elapsed=r["time"]-first) for r in timeline]
    a=next((r for r in rows if r["slot"]==1),None)
    report=dict(samples=rows,passed=bool(a and rows[0]["slot"]==0 and a["elapsed"]>.2
        and a["z"]<rows[0]["z"]-10 and rows[-1]["slot"]==1 and not rows[-1]["equipping"]
        and rows[-1]["shots"]==rows[0]["shots"] and rows[-1]["z"]>a["z"]+10))
    out.write_text(json.dumps(report,indent=2))
step("place",.5,eplace)
step("pickup_focus",.5,lambda:eaim(eactor("P3_Weapon")))
step("pickup",1.2,None,lambda:ek("E"))
step("release",.6,lambda:ek("E",False))
step("select",.06,lambda:ek("Two"),observe)
step("switch_and_fire",.5,lambda:ek("Two",False),lambda:(ek("LeftMouseButton"),observe()))
step("settle",.4,lambda:ek("LeftMouseButton",False),observe)
step("finish",.1,done)
