import unreal,json,os
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
ea=unreal.EditorAssetLibrary;at=unreal.AssetToolsHelpers.get_asset_tools();sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
folder='/Game/AfghanFPS/NPC/Foundation';ea.make_directory(folder)
classes={}
for name,friendly in [('BP_Enemy_Foundation',False),('BP_Friendly_Foundation',True)]:
    path=folder+'/'+name
    if ea.does_asset_exist(path):bp=ea.load_asset(path)
    else:
        f=unreal.BlueprintFactory();f.set_editor_property('parent_class',unreal.FoundationNPC);bp=at.create_asset(name,folder,unreal.Blueprint,f)
    unreal.get_default_object(bp.generated_class()).set_editor_properties({'friendly':friendly,'starting_health':100.0})
    assert unreal.BlueprintEditorLibrary.compile_blueprint(bp);ea.save_loaded_asset(bp,False);classes[name]=bp.generated_class()
placed={a.get_actor_label():a for a in sub.get_all_level_actors()}
for name,loc,route in [
 ('P5_Enemy_Patrol',(2700,-850,-4),[(2700,-850,-4),(3300,-850,-4),(3300,-500,-4),(2700,-500,-4)]),
 ('P5_Enemy_Room',(2200,-760,-4),[(1950,-760,-4),(2200,-760,-4),(2200,-540,-4),(1950,-540,-4)])]:
    a=placed.get(name)
    if not a:a=sub.spawn_actor_from_class(classes['BP_Enemy_Foundation'],unreal.Vector(*loc));a.set_actor_label(name)
    a.set_actor_location(unreal.Vector(*loc),False,False);a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
    a.set_editor_properties({'stable_id':name,'patrol_route':[unreal.Vector(*p) for p in route],'ai_enabled':True})
box=placed.get('P5_CoverBox')
if not box:box=sub.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(2700,-450,-20));box.set_actor_label('P5_CoverBox')
c=box.static_mesh_component;c.set_static_mesh(ea.load_asset('/Engine/BasicShapes/Cube'));box.set_actor_scale3d(unreal.Vector(2.2,.6,1.6));c.set_collision_profile_name('BlockAll')
for i,loc in enumerate([(2535,-450,-100),(2865,-450,-100),(2700,-535,-100),(2700,-365,-100)]):
    name='P5_CoverPoint_'+str(i);a=placed.get(name)
    if not a:a=sub.spawn_actor_from_class(unreal.TargetPoint,unreal.Vector(*loc));a.set_actor_label(name)
    a.set_editor_property('tags',['AI_Cover'])
if 'P5_Help' not in placed:
    a=sub.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(2500,-900,180));a.set_actor_label('P5_Help');a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
    t=a.get_component_by_class(unreal.TextRenderComponent);t.set_text('AI TRAINING AREA\nPatrol / sight / hearing / cover / surrender');t.set_world_size(18)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
result=unreal.FoundationAITools.build_arena_navigation(world);print('NAVIGATION',result);assert not result.startswith('FAILED'),result
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('PHASE5_ASSETS_SAVED')
