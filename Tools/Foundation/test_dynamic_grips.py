"""Exercise moving, reloading and swapping grips using the Phase 4 input fixtures."""
from pathlib import Path
import json
import unreal
folder = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Tools/Foundation"
exec(compile((folder/"test_phase4_equipment.py").read_text(), "equipment_fixture", "exec"))
# Replace the scenario before the first editor callback. Keep the older report intact.
eres["filename"] = "dynamic_grips_fixture.json"
st.clear()
samples = {}
out = folder.parents[1]/"Saved/Foundation/dynamic_grips_pie.json"
perf = unreal.get_default_object(unreal.load_class(None,"/Script/UnrealEd.EditorPerformanceSettings"))
old = perf.get_editor_property("bThrottleCPUWhenNotForeground")
perf.set_editor_property("bThrottleCPUWhenNotForeground", False)
def measure(name):
    gun = eweapon()
    l,r = ep.p2_grip_left,ep.p2_grip_right
    row = samples.setdefault(name, dict(frames=0, max_cm=0., speed=0., reload=False, slot=-1, correct_owner=True))
    row["frames"] += 1
    row["correct_owner"] &= bool(l and r and l.get_owner()==gun and r.get_owner()==gun)
    if l and r:
        for hand,c in (("hand_l",l),("hand_r",r)):
            row["max_cm"]=max(row["max_cm"],(ep.p2_arms.get_socket_location(hand)-c.get_world_location()).length())
    row.update(speed=max(row["speed"],ep.get_velocity().length()),reload=row["reload"] or eq.reloading,slot=eq.active_slot)
def finish():
    perf.set_editor_property("bThrottleCPUWhenNotForeground",old)
    passed=(samples["walk"]["speed"]>100 and samples["reload"]["reload"] and samples["alt"]["slot"]==1 and samples["base"]["slot"]==0
            and all(r["frames"]>=3 and r["max_cm"]<2 and r["correct_owner"] for r in samples.values()))
    out.write_text(json.dumps(dict(passed=passed,samples=samples),indent=2))
step("start",.6,eplace)
step("walk",.6,None,lambda:(ek("W"),measure("walk")))
step("stop",.4,lambda:ek("W",False))
step("arrange",.2,lambda:eammo(5,30))
step("reload",1.,lambda:ek("R"),lambda:measure("reload"))
step("release",1.,lambda:ek("R",False))
step("pickup_focus",.6,lambda:eaim(eactor("P3_Weapon")))
step("pickup",1.2,None,lambda:ek("E"))
step("release_use",.3,lambda:ek("E",False))
tap("switch_alt","Two",.7)
step("alt",.6,None,lambda:measure("alt"))
tap("switch_base","One",.7)
step("base",.6,None,lambda:measure("base"))
step("finish",.1,finish)
