from pathlib import Path
source=Path('C:/GameDev/AfghanistanFPS/Tools/Foundation/test_phase4_equipment.py').read_text()
exec(compile(source.split('st=[]')[0],__file__,'exec'))
base_sample=esample
eres={'method':'PIE held/release input, actual physics and damage; arranged positions only','filename':'throwable_results.json','samples':[],'detonations':{}}
def esample(name):
    base_sample(name);r=eres['samples'][-1]
    r.update(inventory={str(k):v for k,v in ep.interaction_manager.owned_items.items()},throws=eq.throws_released,
             flash=eq.flash_alpha,held=eq.held_throwable.get_name() if eq.held_throwable else None,
             focus=ep.interaction_manager.focused_actor.get_name() if ep.interaction_manager.focused_actor else None,
             use_progress=ep.interaction_manager.progress,
             visible_preview=sum(c.is_visible() for c in ep.get_components_by_class(unreal.StaticMeshComponent)),
             throwables=[])
    for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.FoundationThrowable):
        pos=a.body.get_world_location();v=a.body.get_physics_linear_velocity()
        r['throwables'].append({'name':a.get_name(),'kind':str(a.kind),'held':a.held,'launched':a.launched,'impacted':a.impacted,
          'detonated':a.detonated,'events':a.impact_events,'fuse':a.remaining_fuse,'position':[pos.x,pos.y,pos.z],
          'velocity':[v.x,v.y,v.z],'physics':a.body.is_simulating_physics()})
    hp=ep.get_component_by_class(unreal.load_class(None,'/Game/AfghanFPS/Core/Components/BPC_Health.BPC_Health_C'))
    r['health']=hp.get_editor_property('CurrentHealth')
    for label in ('P4_BlastTarget','P4_ShieldedTarget'):
        actor=eactor(label);r[label]=actor.get_component_by_class(hp.get_class()).get_editor_property('CurrentHealth')
    with open(eroot+'/throwable_results.json','w') as f:json.dump(eres,f,indent=2)
def thrown(kind):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.FoundationThrowable) if a.kind==kind and a.launched and not a.consumed)
def retrieve(kind):eaim(thrown(kind),90)
def track(kind):
    a=thrown(kind);ec.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(ep.p2_camera.get_world_location(),a.get_actor_location()))
def empty_lane():
    eplace();ep.set_actor_location(unreal.Vector(2600,500,-1.8),False,True)
def smoke_view():
    a=thrown(unreal.FoundationThrowableKind.SMOKE);ep.set_actor_location(unreal.Vector(2700,700,-1.8),False,True)
    point=a.body.get_world_location()+unreal.Vector(0,0,150)
    ec.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(2686.544,700,67.57),point))
def down():
    ep.set_actor_location(unreal.Vector(3000,700,-1.8),False,True);ec.set_control_rotation(unreal.Rotator(pitch=-65,yaw=0,roll=0))
def safe():ep.set_actor_location(unreal.Vector(2600,700,-1.8),False,True)
def shield():
    eactor('P4_ShieldedTarget').set_actor_location(unreal.Vector(3200,700,-4),False,True)
    ectx['shield']=unreal.Phase2Tools.test_blocker(ep,unreal.Vector(3150,700,40),unreal.Vector(20,600,280))
def unshield():ectx['shield'].set_actor_enable_collision(False);ectx['shield'].set_actor_hidden_in_game(True)
st=[]
def step(n,t,f=None,d=None):st.append((n,t,f,d))
def tap(n,k,t=.3):step(n+'_press',.07,lambda:ek(k));step(n,t,lambda:ek(k,False))
def collect(label):
    step(label+'_focus',.5,lambda:eaim(eactor(label)));step(label+'_hold',1.2,None,lambda:ek('E'));step(label+'_release',.2,lambda:ek('E',False))
step('initial',.7,lambda:(eplace(),eactor('P4_ShieldedTarget').set_actor_location(unreal.Vector(3200,200,-4),False,True)));tap('select_stone','M')
step('stone_hold',.7,lambda:ek('T'));step('stone_preview',.3,lambda:eshot('stone_preview'))
step('stone_launch',.3,lambda:ek('T',False));step('stone_impacts',4.0)
step('stone_retrieve_focus',.6,lambda:retrieve(unreal.FoundationThrowableKind.STONE),lambda:track(unreal.FoundationThrowableKind.STONE));step('stone_retrieve_hold',.9,None,lambda:ek('E'));step('stone_retrieved',.3,lambda:ek('E',False))
step('knife_prepare',.6,empty_lane);tap('select_knife','N');step('knife_hold',.6,lambda:ek('T'))
step('knife_launch',.3,lambda:ek('T',False));step('knife_embed',1.0)
step('knife_retrieve_focus',.6,lambda:retrieve(unreal.FoundationThrowableKind.KNIFE));step('knife_retrieve_hold',.9,None,lambda:ek('E'));step('knife_retrieved',.3,lambda:ek('E',False))
collect('P4_GrenadePickup');collect('P4_CookGrenadePickup');tap('select_frag','V')
step('blast_prepare',.6,down);step('frag_hold',.15,lambda:ek('T'));step('frag_release',.3,lambda:ek('T',False));step('frag_safe',.2,safe);step('frag_damage',3.2)
step('shield_arrange',.3,shield);step('shield_throw_prepare',.6,down);step('shield_frag_hold',.15,lambda:ek('T'));step('shield_frag_release',.3,lambda:ek('T',False));step('shield_safe',.2,safe);step('shielded_damage',3.2)
step('unshield',.3,unshield);step('unshield_throw_prepare',.6,down);step('unshield_frag_hold',.15,lambda:ek('T'));step('unshield_frag_release',.3,lambda:ek('T',False));step('unshield_safe',.2,safe);step('unshielded_damage',3.2)
tap('select_smoke','V');step('smoke_prepare',.5,down);step('smoke_hold',.2,lambda:ek('T'));step('smoke_release',.3,lambda:ek('T',False));step('smoke_safe',.2,safe);step('smoke_active',3.2);step('smoke_view',.6,smoke_view);step('smoke_image',.4,lambda:eshot('smoke'))
tap('select_flash','V');step('flash_prepare',.5,eplace);step('flash_cook',3.15,lambda:ek('T'));step('flash_image',.2,lambda:(ek('T',False),eshot('flash')));step('flash_fades',3.0)
for i in range(3):tap('cycle_to_frag_'+str(i),'V')
step('cook_prepare',.5,eplace);step('cook_near_fuse',2.5,lambda:ek('T'));step('cook_detonates',1.0);step('cook_release',.2,lambda:ek('T',False))
state={'i':0,'entered':False,'stable':0,'ready':False}
def throwable_tick(dt):
    try:
        for a in unreal.GameplayStatics.get_all_actors_of_class(ew,unreal.FoundationThrowable):
            if a.detonated and a.get_name() not in eres['detonations']:
                pos=a.body.get_world_location();eres['detonations'][a.get_name()]={'kind':str(a.kind),'position':[pos.x,pos.y,pos.z],'at':a.detonated_at}
        if not state['ready']:
            state['stable']=state['stable']+1 if unreal.GameplayStatics.get_world_delta_seconds(ew)<.05 else 0
            if state['stable']<20:return
            state['ready']=True
        if state['i']>=len(st):erel();ek('T',False);unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_THROWABLES_DONE');return
        n,t,enter,during=st[state['i']]
        if not state['entered']:
            state['entered']=True;state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-state['start']>=t:esample(n);state['i']+=1;state['entered']=False
    except Exception:
        erel();ek('T',False);unreal.unregister_slate_post_tick_callback(state['handle']);print('PHASE4_THROWABLES_ERROR',traceback.format_exc())
state['handle']=unreal.register_slate_post_tick_callback(throwable_tick)
print('PHASE4_THROWABLES_STARTED')
