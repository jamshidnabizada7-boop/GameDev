"""Additional door/mirror/cancellation tests after the main phase-3 suite."""
from pathlib import Path
source=Path('C:/GameDev/AfghanistanFPS/Tools/Foundation/test_phase3.py').read_text()
exec(compile(source.split('p3stages=[]')[0],__file__,'exec'))
p3dir='C:/GameDev/AfghanistanFPS/Saved/Phase3Edges';os.makedirs(p3dir,exist_ok=True)
exec(compile('p3stages=[]'+source.split('p3stages=[]')[1].split("p3stage('locked_focus'")[0],__file__,'exec'))
p3ctx={}
p3cube=next(a for a in p3actors.values() if a.get_class().get_name()=='BP_TestInteractable_C')
def edge_blocker():
    a=p3actors['P3_Lockpick'];point=a.interaction_point();eye=p3p.p2_camera.get_world_location()
    p3ctx['blocker']=unreal.Phase2Tools.test_blocker(p3p,(point+eye)*.5,unreal.Vector(15,60,100))
def clear_blocker():
    p3ctx['blocker'].set_actor_enable_collision(False);p3ctx['blocker'].set_actor_hidden_in_game(True)
def arrange_locked():
    # Fresh independent fixture state; unlocking still requires real input/tool.
    a=p3actors['P3_PeekDoor'];a.set_door_target(0)
def lock_fixture():p3actors['P3_PeekDoor'].set_editor_properties({'locked':True,'required_key':'test_other_key'})
def edge_mirror_aim():
    a=p3actors['P3_PeekDoor'];point=a.interaction_point()
    p3pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p3p.p2_camera.get_world_location(),point))
def edge_sample(name):
    p3sample(name)
    r=p3out['samples'][-1];d=p3actors['P3_PeekDoor']
    r.update(view_target=p3pc.get_view_target().get_name(),movement_mode=str(p3p.character_movement.movement_mode),
             interior_volume=d.interior_audio.volume_multiplier,exterior_volume=d.exterior_audio.volume_multiplier,
             interior_low_pass=d.interior_audio.get_editor_property('enable_low_pass_filter'),pick_angle=d.pick_angle,
             mirror_frame_ms=p3m.mirror_frame_milliseconds,drawer_y=p3actors['P3_Drawer'].panel.get_editor_property('relative_location').y,
             legacy_cube_scale=p3cube.get_actor_scale3d().x)
    r['bullet_breakable_broken']=p3actors['P3_BulletBreakable'].broken
    r['bullet_breakable_durability']=p3actors['P3_BulletBreakable'].durability
    with open(p3dir+'/runtime_results.json','w') as f:json.dump(p3out,f,indent=2)
p3stage('fire_breakable_focus',.6,lambda:p3aim('P3_BulletBreakable'))
for fire_index in range(3):
    p3stage('crate_fire_'+str(fire_index),.06,lambda:p3key('LeftMouseButton'))
    p3stage('crate_release_'+str(fire_index),.4,lambda:p3key('LeftMouseButton',False))
p3stage('legacy_focus',.6,lambda:p3aim(p3cube.get_actor_label()))
p3tap('legacy_cube_toggle')
p3stage('peek15_arrange',.8,lambda:p3actors['P3_PeekDoor'].set_door_target(15))
p3stage('peek15_baseline',.6,lambda:p3aim('P3_PeekDoor'))
p3stage('peek15_active',.8,None,lambda:p3key('F'))
p3stage('peek15_release',.8,lambda:p3key('F',False))
p3stage('close_prepare',1.3,arrange_locked)
p3stage('mirror_focus',.6,lambda:p3aim('P3_PeekDoor'))
p3stage('mirror_crouch',.7,None,lambda:p3key('LeftControl'))
p3stage('mirror_reaim',.4,edge_mirror_aim,lambda:p3key('LeftControl'))
p3stage('mirror_open_press',.2,lambda:p3key('E'),lambda:p3key('LeftControl'))
p3stage('mirror_retained_after_release',.8,lambda:p3key('E',False),lambda:p3key('LeftControl'))
p3stage('mirror_image',.5,lambda:p3shot('mirror'),lambda:p3key('LeftControl'))
p3stage('mirror_pan',.5,None,lambda:(p3key('LeftControl'),p3key('MouseX',True,20)))
p3stage('mirror_exit_press',.2,lambda:p3key('E'),lambda:p3key('LeftControl'))
p3stage('mirror_exit_release',.7,lambda:(p3key('E',False),p3key('LeftControl',False),p3key('MouseX',False)))
p3stage('range_focus',.5,lambda:p3aim('P3_Lockpick'))
p3stage('range_start',.3,lambda:p3key('E'))
p3stage('range_cancel',.6,lambda:p3p.set_actor_location(unreal.Vector(1100,-300,-1.8),False,True))
p3stage('range_release',.2,lambda:p3key('E',False))
p3stage('los_focus',.5,lambda:p3aim('P3_Lockpick'))
p3stage('los_start',.3,lambda:p3key('E'))
p3stage('los_cancel',.6,edge_blocker)
p3stage('los_release',.3,lambda:(p3key('E',False),clear_blocker()))
p3pickup('P3_Lockpick')
p3stage('lockpick_arrange',.5,lock_fixture)
p3stage('lockpick_focus',.5,lambda:p3aim('P3_PeekDoor'))
p3stage('lockpick_unaligned',.8,lambda:p3key('E'))
p3stage('lockpick_align',.08,lambda:p3m.adjust_use(7))
p3stage('lockpick_complete',2.1)
p3stage('lockpick_release',.3,lambda:p3key('E',False))
p3stage('collision_focus',.5,lambda:p3aim('P3_PeekDoor'))
p3stage('collision_obstacle',.3,lambda:p3ctx.update(door_blocker=unreal.Phase2Tools.test_blocker(p3p,unreal.Vector(1740,-90,10),unreal.Vector(25,35,210))))
p3tap('door_collision_stop')
p3stage('collision_cleanup',.3,lambda:(p3ctx['door_blocker'].set_actor_enable_collision(False),p3ctx['door_blocker'].set_actor_hidden_in_game(True)))
p3state={'index':0,'entered':False}
def p3edge_tick(dt):
    try:
        if p3state['index']>=len(p3stages):
            p3release();unreal.unregister_slate_post_tick_callback(p3state['handle']);print('PHASE3_EDGES_DONE');return
        name,duration,enter,during=p3stages[p3state['index']]
        if not p3state['entered']:
            p3state['entered']=True;p3state['start']=time.monotonic()
            if enter:enter()
        if during:during()
        if time.monotonic()-p3state['start']>=duration:
            edge_sample(name);p3state['index']+=1;p3state['entered']=False
    except Exception:
        p3release();unreal.unregister_slate_post_tick_callback(p3state['handle']);print('PHASE3_EDGES_ERROR',traceback.format_exc())
p3state['handle']=unreal.register_slate_post_tick_callback(p3edge_tick)
print('PHASE3_EDGES_STARTED')
