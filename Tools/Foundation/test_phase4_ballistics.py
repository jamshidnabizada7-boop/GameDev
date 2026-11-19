from pathlib import Path
source=Path('C:/GameDev/AfghanistanFPS/Tools/Foundation/test_phase4_equipment.py').read_text()
exec(compile(source.split('st=[]')[0],__file__,'exec'))
base_sample=esample
eres={'method':'PIE fired input; reversible definition flags and cover dimensions arrange bounded cases','filename':'ballistics_results.json','samples':[]}
definition=eq.slots[0].definition
original_flags=(definition.allow_penetration,definition.projectile_mode)
cover=eactor('P4_ThinCover');original_scale=cover.get_actor_scale3d();original_tags=list(cover.tags)
hpclass=unreal.load_class(None,'/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C')
def esample(name):
    base_sample(name);r=eres['samples'][-1]
    r.update(penetrated=eq.last_penetrated,damage=eq.last_shot_damage,zone=str(eq.last_damage_zone),projectile_impacts=eq.projectile_impacts,shells=eq.shell_ejections)
    for label in ('P4_PenetrationTarget','P4_ProjectileTarget','P4_LimbTarget','P4_RangeTarget'):
        r[label]=eactor(label).get_component_by_class(hpclass).get_editor_property('CurrentHealth')
    with open(eroot+'/ballistics_results.json','w') as f:json.dump(eres,f,indent=2)
def aim_at(label,distance=400,socket=None):
    a=eactor(label)
    if socket:point=a.get_components_by_class(unreal.SkeletalMeshComponent)[0].get_socket_location(socket)
    else:point=a.get_actor_location()+unreal.Vector(0,0,75)
    ep.p2_set_stance(0);ep.set_actor_location(unreal.Vector(point.x-distance,point.y,-1.8),False,True)
    ec.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(point.x-distance-13.456,point.y,67.57),point))
def flags(pen,projectile):definition.set_editor_properties({'allow_penetration':pen,'projectile_mode':projectile})
def restore():
    flags(*original_flags);cover.set_actor_scale3d(original_scale);cover.set_editor_property('tags',original_tags)
st=[]
def step(n,t,f=None,d=None):st.append((n,t,f,d))
def tap(n,k,t=.4):step(n+'_press',.06,lambda:ek(k));step(n,t,lambda:ek(k,False))
step('initial',.7,lambda:(eplace(),flags(False,False)))
step('ads',.7,lambda:ek('RightMouseButton'))
step('pen_normal_focus',.6,lambda:aim_at('P4_PenetrationTarget'));tap('pen_disabled','LeftMouseButton')
step('pen_enable',.2,lambda:flags(True,False));tap('pen_thin','LeftMouseButton')
step('thick_arrange',.2,lambda:cover.set_actor_scale3d(unreal.Vector(.5,2,2)));tap('pen_thick_rejected','LeftMouseButton')
step('untag_arrange',.2,lambda:(cover.set_actor_scale3d(original_scale),cover.set_editor_property('tags',[])));tap('pen_untagged_rejected','LeftMouseButton')
step('projectile_arrange',.6,lambda:(flags(False,True),aim_at('P4_ProjectileTarget')));tap('projectile_hit','LeftMouseButton',.5)
step('head_arrange',.6,lambda:(flags(False,False),aim_at('P4_LimbTarget',socket='head')));tap('head_hit','LeftMouseButton')
step('limb_arrange',.6,lambda:aim_at('P4_LimbTarget',socket='foot_l'));tap('limb_hit','LeftMouseButton')
step('range_arrange',.6,lambda:aim_at('P4_RangeTarget',2200));tap('range_hit','LeftMouseButton')
step('restore',.3,lambda:(restore(),ek('RightMouseButton',False)))
state={'i':0,'entered':False,'stable':0,'ready':False}
def ballistics_tick(dt):
    try:
        if not state['ready']:
            state['stable']=state['stable']+1 if unreal.GameplayStatics.get_world_delta_seconds(ew)<.05 else 0
            if state['stable']<20:return
            state['ready']=True
        if state['i']>=len(st):restore();erel();unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_BALLISTICS_DONE');return
        n,t,enter,during=st[state['i']]
        if not state['entered']:
            state['entered']=True;state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-state['start']>=t:esample(n);state['i']+=1;state['entered']=False
    except Exception:
        restore();erel();unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_BALLISTICS_ERROR',traceback.format_exc())
state['handle']=unreal.register_slate_post_tick_callback(ballistics_tick)
print('PHASE4_BALLISTICS_STARTED')
