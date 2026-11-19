"""Evidence assertions over recorded integration outcomes, not mirrored unit code."""
import json,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[2];dest=root/'Docs/Foundation/Phase3Evidence';dest.mkdir(parents=True,exist_ok=True)
main=json.loads((root/'Saved/Phase3/runtime_results.json').read_text())
edge=json.loads((root/'Saved/Phase3Edges/runtime_results.json').read_text())
m={r['name']:r for r in main['samples']};e={r['name']:r for r in edge['samples']}
checks=[]
def check(name,condition):checks.append({'check':name,'passed':bool(condition)})
def actor(row,name,prop):return row['actors']['P3_'+name][prop]
check('locked door denies entry',actor(m['locked_denied'],'ArmoryDoor','locked') and actor(m['locked_denied'],'ArmoryDoor','door_angle')==0)
check('key granted once',m['P3_ArmoryKey_release']['items'].get('armory_key')==1)
check('key unlocks and opens to 90',not actor(m['key_door_open'],'ArmoryDoor','locked') and abs(actor(m['key_door_open'],'ArmoryDoor','door_angle')-90)<.1)
check('two wheel notches hold ten degrees',abs(actor(m['adjust_release'],'PeekDoor','door_angle')-10)<.1 and abs(actor(m['peek_release'],'PeekDoor','door_angle')-10)<.1)
check('peek enters and restores FOV',m['peek']['peeking'] and abs(m['peek']['fov']-65)<1 and abs(m['peek_release']['fov']-90)<1)
check('fifteen-degree peek shifts camera without moving capsule',sum((a-b)**2 for a,b in zip(e['peek15_active']['position'],e['peek15_baseline']['position']))<1 and abs(e['peek15_active']['camera'][1]-e['peek15_baseline']['camera'][1])>5 and abs(e['peek15_release']['fov']-90)<1)
check('listen enters and releases',actor(m['listening'],'PeekDoor','listening') and not actor(m['listen_release'],'PeekDoor','listening'))
check('ammo adds reserve only',m['P3_Ammo_release']['reserve']==120 and m['P3_Ammo_release']['ammo']==30)
check('full health rejects without consuming',not actor(m['full_health_reject'],'Health','consumed'))
check('health heals and updates existing HUD',m['damage']['health']==78 and m['P3_Health_release']['health']==100 and m['P3_Health_release']['hud_health']=='HP: 100')
check('weapon ownership token granted once',m['P3_Weapon_release']['items'].get('weapon_rifle_alt')==1)
check('early release cancels loot',not actor(m['cancel_early_release'],'Loot','consumed'))
check('loot grants designed reserve once',m['P3_Loot_release']['reserve']==165 and actor(m['P3_Loot_release'],'Loot','use_count')==1)
check('drawer opens',actor(m['P3_Drawer_use'],'Drawer','open'))
check('actual light toggles off',not actor(m['P3_Switch_use'],'Switch','light_on'))
check('linked external lamp toggles off',not m['P3_Switch_use']['linked_light_visible'])
check('breakable reaches broken state',actor(m['P3_Breakable_use'],'Breakable','broken'))
check('legacy cube still toggles through E',e['legacy_cube_toggle']['legacy_cube_scale']!=e['legacy_focus']['legacy_cube_scale'] and e['legacy_cube_toggle']['transactions']>e['legacy_focus']['transactions'])
check('mirror survives opening E release',e['mirror_retained_after_release']['mirror'] and e['mirror_retained_after_release']['view_target'].startswith('CameraActor'))
check('mirror exits to pawn and walking',not e['mirror_exit_release']['mirror'] and e['mirror_exit_release']['view_target'].startswith('BP_PlayerFoundation') and 'WALKING' in e['mirror_exit_release']['movement_mode'])
check('range and LOS cancel hold without reward',all(not e[n]['using'] and e[n]['progress']==0 and e[n]['items'].get('lockpick',0)==0 for n in ('range_cancel','los_cancel')))
check('unaligned pick fails, aligned pick unlocks',actor(e['lockpick_unaligned'],'PeekDoor','locked') and e['lockpick_unaligned']['progress']==0 and not actor(e['lockpick_complete'],'PeekDoor','locked'))
check('swept door stops against obstruction',actor(e['door_collision_stop'],'PeekDoor','blocked') and 0<actor(e['door_collision_stop'],'PeekDoor','door_angle')<90)
check('existing weapon damages and breaks native prop',e['fire_breakable_focus']['bullet_breakable_durability']==50 and e['crate_release_2']['bullet_breakable_broken'] and e['crate_release_2']['ammo']<e['fire_breakable_focus']['ammo'])
report={'checks':checks,'passed':sum(c['passed'] for c in checks),'total':len(checks),'owner_approved':False,'limits':['Generated key events, not owner acceptance.','Mirror timings are sampled world/frame time, not isolated GPU cost.','Original placeholder room audio; subjective audibility and polished animations require review.','Walking NPC audio awaits later AI integration.']}
(dest/'checks.json').write_text(json.dumps(report,indent=2))
for folder in ('Phase3','Phase3Edges'):
    for filename in ('runtime_results.json','mirror.png','interaction_annex.png','editor_build.log','game_build.log'):
        p=root/'Saved'/folder/filename
        if p.exists():shutil.copy2(p,dest/(folder+'_'+filename))
print(json.dumps(report,indent=2))
if report['passed']!=report['total']:raise SystemExit(1)
