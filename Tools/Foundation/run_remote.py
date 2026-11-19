"""Execute a project-authored editor script; propagate Unreal's actual status."""
import importlib.util,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('project_remote',root/'Docs/Foundation/scratch/batch2r/ue_remote.py')
bridge=importlib.util.module_from_spec(spec);spec.loader.exec_module(bridge)
remote=bridge.connect()
try:
    result=remote.run_command(str(Path(sys.argv[1]).resolve()),unattended=True,exec_mode=bridge.rex.MODE_EXEC_FILE)
    evidence=root/'Saved/Foundation';evidence.mkdir(parents=True,exist_ok=True)
    (evidence/'last_remote.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    for entry in result.get('output',[]):print(entry.get('type',''),entry.get('output',''))
    print('UNREAL_SCRIPT_SUCCESS',result.get('success',False))
    if not result.get('success',False):print(result.get('result',''));sys.exit(1)
finally:
    remote.close_command_connection();remote.stop()
