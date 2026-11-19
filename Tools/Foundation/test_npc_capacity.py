"""Exercise overflow and reuse without saving anything to the sandbox map."""
from pathlib import Path
import json
import traceback
import unreal as u
world,=u.EditorLevelLibrary.get_pie_worlds(False)
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))/"Foundation/npc_capacity_pie.json"
def live(): return u.GameplayStatics.get_all_actors_of_class(world,u.FoundationNPC)
before=list(live())
report={"initial":len(before),"counts":[]}
try:
    def spawn(i):
        u.AIHelperLibrary.spawn_ai_from_class(world,u.FoundationNPC,None,u.Vector(300+100*i,200,180),u.Rotator(),True)
        report["counts"].append(len(live()))
    for i in range(12): spawn(i)
    added=[n for n in live() if n not in before]
    report["peak"]=len(live())
    added[0].destroy_actor()
    report["after_release"]=len(live())
    spawn(13)
    report["refilled"]=len(live())
    report["passed"]=max(report["counts"])==report["peak"]==report["refilled"]==10 and report["after_release"]==9 and all(n in live() for n in before)
except Exception:
    report["error"]=traceback.format_exc()
finally:
    for n in live():
        if n not in before:n.destroy_actor()
    out.write_text(json.dumps(report,indent=2))
