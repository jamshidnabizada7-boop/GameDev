"""Verify a truncated checkpoint is rejected before changing the live player."""
import json, uuid, traceback
from pathlib import Path
import unreal as u
w,=u.EditorLevelLibrary.get_pie_worlds(False)
p=u.GameplayStatics.get_player_pawn(w,0)
s=p.foundation_session
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()))
slot="CorruptProbe_"+uuid.uuid4().hex
report={}
def snapshot():
    loc=p.get_actor_location()
    return [loc.x,loc.y,loc.z,p.equipment.active_slot,str(p.equipment.summary()),s.objective_complete]
try:
    before=snapshot()
    assert s.save_checkpoint(slot)
    files=list((root/"SaveGames").rglob(slot+".sav"))
    assert len(files)==1
    f=files[0]
    original=f.read_bytes()
    assert len(original)>128
    f.write_bytes(original[:4])
    report["rejected"]=not s.load_checkpoint(slot)
    report["feedback"]=str(s.message)
    report["unchanged"]=before==snapshot()
    report["error_feedback"]="damaged" in report["feedback"].lower()
    damaged=bytearray(original);damaged[len(damaged)//2]^=1
    f.write_bytes(damaged)
    report["checksum_rejected"]=not s.load_checkpoint(slot) and before==snapshot()
    f.write_bytes(original[:-12])
    report["legacy_load"]=s.load_checkpoint(slot)
    f.write_bytes(original)
    report["restored_load"]=s.load_checkpoint(slot)
    report["passed"]=all(report[k] for k in ("rejected","unchanged","error_feedback","restored_load","checksum_rejected","legacy_load"))
except Exception:report["error"]=traceback.format_exc()
finally:
    u.GameplayStatics.delete_game_in_slot(slot,0)
    (root/"Foundation/corrupt_checkpoint_pie.json").write_text(json.dumps(report,indent=2))
