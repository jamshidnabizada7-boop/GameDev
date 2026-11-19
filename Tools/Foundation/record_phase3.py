"""Update only scoped register entries after the evidence audit succeeds."""
import csv,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
evidence=root/'Docs/Foundation/Phase3Evidence'
checks=json.loads((evidence/'checks.json').read_text())
assert checks['passed']==checks['total'],'Do not record passing scope while evidence fails'
master=Path('D:/GameDev/FrameworkEvaluation/MASTER_FOUNDATION_REGISTER.csv')
with master.open(encoding='utf-8-sig',newline='') as f:
    reader=csv.DictReader(f);fields=reader.fieldnames;rows=list(reader)
scope={
 'J-1':'WORKS IN PIE: shared interface, focus, holds and original cube; owner acceptance pending',
 'J-2':'WORKS IN PIE: hinge open/close, locked denial and obstruction stop; owner acceptance pending',
 'J-4':'PARTIAL: one-shot ammunition/health transactions verified in PIE; grenade/equipment integration is phase 4',
 'J-5':'WORKS IN PIE: own light and linked external lamp toggle; owner acceptance pending',
 'J-6':'WORKS IN PIE: designed ammunition reward, once only, cancellation verified',
 'J-7':'WORKS IN PIE: existing weapon damages native crate to mesh-swap debris',
 'J-8':'WORKS IN PIE: two wheel units stop and hold door at ten degrees within 0.1 degree',
 'J-9':'WORKS IN PIE: lean peek, FOV entry/restore and stationary capsule; comfort review pending',
 'J-10':'WORKS IN PIE: real hidden-room mirror camera, bounded pan and clean exit; whole-frame samples only',
 'J-11':'PARTIAL: spatial audio filtering/volume transitions in PIE; owner audibility and walking NPC source pending',
 'J-12':'WORKS IN PIE: key unlock and aligned timed lockpick; original placeholder cues; owner audio review pending',
 'H-3':'PARTIAL: procedural pickup reach IK integrated; authored animation and owner visual review pending',
}
for row in rows:
    if row['Action_ID'] not in scope:continue
    row['Evidence_Status']=scope[row['Action_ID']]+'; Docs/Foundation/Phase3Evidence/checks.json; NOT OWNER APPROVED'
    row['UE584_Implementation']='FoundationInteraction component/interface + reusable Blueprint children; '+row['UE584_Implementation']
assert len(rows)==140 and len({r['Action_ID'] for r in rows})==140
with master.open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader();writer.writerows(rows)
(evidence/'git-status.txt').write_text(subprocess.check_output(['git','status','--short'],cwd=root,text=True))
(evidence/'git-head.txt').write_text(subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True))
manifest=[]
for p in list((root/'Source/AfghanistanFPS').glob('Foundation*'))+list((root/'Content/AfghanFPS/Interaction/Foundation').glob('*.uasset')):
    import hashlib
    manifest.append({'path':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(evidence/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('Recorded',len(scope),'scoped statuses; 140 unique actions retained; no owner approval asserted')
