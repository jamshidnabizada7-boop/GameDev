import csv,json,subprocess,hashlib,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2];out=root/'Docs/Foundation/Phase4Evidence'
audit=json.loads((out/'checks.json').read_text());restart=json.loads((root/'Saved/Phase4/restart_results.json').read_text())
assert audit['passed']==audit['total'] and restart['passed']
master=Path('D:/GameDev/FrameworkEvaluation/MASTER_FOUNDATION_REGISTER.csv')
with master.open(encoding='utf-8-sig',newline='') as f:r=csv.DictReader(f);fields=r.fieldnames;rows=list(r)
statuses={}
for category,count in [('C',10),('D',9),('E',7),('I',8)]:
    for i in range(1,count+1):statuses[f'{category}-{i}']='WORKS IN PIE: phase-4 recorded equipment/physics cases; procedural animation requires owner review'
statuses['I-8']='PARTIAL: physical stone, impact events and retrieval verified; actual AI hearing response is phase 5'
for action in ('M-5','M-6','K-8','L-2'):statuses[action]='WORKS IN PIE: phase-4 live equipment data/HUD/camera or physical ejection; owner review pending'
for action in ('K-5','L-1','L-4'):statuses[action]='PARTIAL: procedural recoil or short-lived light/mesh FX implemented; owner visual/comfort review pending'
statuses['L-5']='PARTIAL: original gunshot cues play from muzzle; distance attenuation and mixer integration require phase 7'
statuses['L-6']='PARTIAL: original reload cue and procedural handling; full timed foley integration requires phase 7'
statuses['L-9']='PARTIAL: real fuse/radial damage and placeholder explosion VFX/SFX verified; camera/light feedback integration remains'
for action in ('B-7','B-8'):statuses[action]='PARTIAL: existing sockets/grips and procedural handling used in PIE; authored animation/notify polish remains'
for row in rows:
    if row['Action_ID'] in statuses:
        row['Evidence_Status']=statuses[row['Action_ID']]+'; Docs/Foundation/Phase4Evidence/checks.json; NOT OWNER APPROVED'
        row['UE584_Implementation']='FoundationEquipment + weapon definitions; FoundationThrowables; existing BP_Weapon_Base.TryFire preserved. '+row['UE584_Implementation']
assert len(rows)==140 and len({r['Action_ID'] for r in rows})==140
with master.open('w',encoding='utf-8-sig',newline='') as f:w=csv.DictWriter(f,fields);w.writeheader();w.writerows(rows)
for src in ('phase4_editor_build.log','phase4_game_build.log'):
    shutil.copy2(root/'Saved/Foundation'/src,out/src)
shutil.copy2(root/'Saved/Phase4/restart_results.json',out/'restart_results.json')
(out/'git-status.txt').write_text(subprocess.check_output(['git','status','--short'],cwd=root,text=True))
manifest=[{'path':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in (root/'Source/AfghanistanFPS').glob('Foundation*')]
(out/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('Recorded',len(statuses),'scoped phase-4 statuses with explicit integration limits; 140 IDs retained')
