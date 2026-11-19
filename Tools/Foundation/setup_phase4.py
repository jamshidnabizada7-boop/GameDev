"""Create original equipment definitions, inputs, cues and a child of the existing weapon."""
import unreal,os,wave,struct,math,random
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
ea=unreal.EditorAssetLibrary;at=unreal.AssetToolsHelpers.get_asset_tools();be=unreal.BlueprintEditorLibrary
folder='/Game/AfghanFPS/Weapons/Foundation';ea.make_directory(folder)
base=ea.load_asset('/Game/AfghanFPS/Weapons/BP_Weapon_Base');assert base
altpath=folder+'/BP_Weapon_AlternateTest'
if ea.does_asset_exist(altpath):alt=ea.load_asset(altpath)
else:
    factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',base.generated_class())
    alt=at.create_asset('BP_Weapon_AlternateTest',folder,unreal.Blueprint,factory)
assert be.compile_blueprint(alt);ea.save_loaded_asset(alt,False)
matpath=folder+'/M_AlternateTest'
if ea.does_asset_exist(matpath):mat=ea.load_asset(matpath)
else:
    mat=at.create_asset('M_AlternateTest',folder,unreal.Material,unreal.MaterialFactoryNew())
    color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-250,0)
    color.set_editor_property('constant',unreal.LinearColor(.13,.25,.17,1))
    unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat);ea.save_loaded_asset(mat,False)
definitions=[]
for name,weapon_id,title,capacity,reserve,damage,interval,actor,material in [
 ('DA_RifleFoundation','weapon_rifle_base','Foundation rifle',30,90,20,.13,base.generated_class(),None),
 ('DA_AlternateFoundation','weapon_rifle_alt','Compact test firearm',20,60,30,.2,alt.generated_class(),mat)]:
    path=folder+'/'+name
    if ea.does_asset_exist(path):definition=ea.load_asset(path)
    else:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.FoundationWeaponDefinition)
        definition=at.create_asset(name,folder,unreal.FoundationWeaponDefinition,factory)
    definition.set_editor_properties({'weapon_id':weapon_id,'display_name':title,'capacity':capacity,'initial_reserve':reserve,
        'base_damage':damage,'shot_interval':interval,'actor_class':actor,'visual_material':material,
        'ammo_type':'rifle' if capacity==30 else 'alternate','default_mode':unreal.FoundationFireMode.SEMI,'compact_variant':capacity==20})
    ea.save_loaded_asset(definition,False)
    definitions.append(definition)

imc=ea.load_asset('/Game/Input/IMC_Default')
for name,key in [('ADS','RightMouseButton'),('Reload','R'),('WeaponOne','One'),('WeaponTwo','Two'),
 ('WeaponNext','MouseScrollUp'),('FireMode','B'),('Holster','H'),('Inspect','I'),('DropWeapon','X'),
 ('Throw','T'),('ThrowableCycle','V'),('SelectKnife','N'),('SelectStone','M')]:
    path='/Game/Input/Actions/IA_'+name
    action=ea.load_asset(path) if ea.does_asset_exist(path) else ea.duplicate_asset('/Game/Input/Actions/IA_Interact',path)
    assert action
    assert unreal.Phase2Tools.map_input(imc,action,key)
    ea.save_loaded_asset(action,False)
look=ea.load_asset('/Game/Input/Actions/IA_Look')
assert unreal.Phase2Tools.map_input(imc,ea.load_asset('/Game/Input/Actions/IA_WeaponNext'),'MouseScrollDown')
assert unreal.Phase2Tools.add_aim_modifier(look)
ea.save_loaded_asset(look,False);ea.save_loaded_asset(imc,False)
mouse=ea.load_asset('/Game/Input/Actions/IA_MouseLook')
if mouse:
    assert unreal.Phase2Tools.add_aim_modifier(mouse);ea.save_loaded_asset(mouse,False)
player=ea.load_asset('/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation')
unreal.get_default_object(player.generated_class()).equipment.set_editor_property('available_definitions',definitions)
print('EQUIPMENT_INPUT_INTEGRATION',unreal.Phase2Tools.integrate_player(player));ea.save_loaded_asset(player,False)

# Authored synthesis: compact neutral placeholders, no copied game sounds.
audio_dir='C:/GameDev/AfghanistanFPS/SourceAssets/Audio';os.makedirs(audio_dir,exist_ok=True)
for name,duration,frequency,amplitude in [('Shot',.14,90,.35),('Dry',.08,1100,.18),('Reload',.3,550,.16),('Explosion',.6,65,.4)]:
    path='/Game/AfghanFPS/Audio/Foundation/'+name
    if ea.does_asset_exist(path):continue
    rate=22050;rng=random.Random(371+len(name));samples=[];smooth=0
    for i in range(int(rate*duration)):
        t=i/rate;smooth=.55*smooth+.45*rng.uniform(-1,1)
        v=(smooth*.7+math.sin(t*math.tau*frequency)*.3)*math.exp(-t/(duration*.22))*amplitude
        samples.append(struct.pack('<h',int(max(-1,min(1,v))*32767)))
    wav=audio_dir+'/'+name+'.wav'
    with wave.open(wav,'wb') as f:f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(b''.join(samples))
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':wav,'destination_path':'/Game/AfghanFPS/Audio/Foundation','automated':True,'save':True});at.import_asset_tasks([task])
for name,color,opacity in [('M_Smoke',(.18,.18,.19),.35),('M_Explosion',(5,.5,.02),.8),('M_Trajectory',(0,.8,1),1)]:
    path=folder+'/'+name
    if ea.does_asset_exist(path):continue
    m=at.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    if opacity<1:m.set_editor_properties({'blend_mode':unreal.BlendMode.BLEND_TRANSLUCENT,'two_sided':True})
    c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-250,0)
    c.set_editor_property('constant',unreal.LinearColor(*color,1));unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity<1:
        a=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-250,150);a.set_editor_property('r',opacity)
        unreal.MaterialEditingLibrary.connect_material_property(a,'',unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(m);ea.save_loaded_asset(m,False)
for name,kind in [('BP_ThrowableBase',unreal.FoundationThrowableKind.FRAG),('BP_ThrowableKnife',unreal.FoundationThrowableKind.KNIFE),('BP_ThrowableStone',unreal.FoundationThrowableKind.STONE)]:
    path=folder+'/'+name
    if ea.does_asset_exist(path):bp=ea.load_asset(path)
    else:
        f=unreal.BlueprintFactory();f.set_editor_property('parent_class',unreal.FoundationThrowable);bp=at.create_asset(name,folder,unreal.Blueprint,f)
    unreal.get_default_object(bp.generated_class()).set_editor_property('kind',kind);assert be.compile_blueprint(bp);ea.save_loaded_asset(bp,False)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);placed={a.get_actor_label():a for a in sub.get_all_level_actors()}
if 'P4_GrenadePickup' not in placed:
    a=sub.spawn_actor_from_class(unreal.FoundationInteractable,unreal.Vector(1500,850,-100));a.set_actor_label('P4_GrenadePickup')
    a.set_editor_properties({'item_id':'frag','item_amount':1,'stable_id':'P4_GrenadePickup'});a.configure(unreal.FoundationInteractableKind.ITEM)
if 'P4_CookGrenadePickup' not in placed:
    a=sub.spawn_actor_from_class(unreal.FoundationInteractable,unreal.Vector(1650,850,-100));a.set_actor_label('P4_CookGrenadePickup')
    a.set_editor_properties({'item_id':'frag','item_amount':1,'stable_id':'P4_CookGrenadePickup'});a.configure(unreal.FoundationInteractableKind.ITEM)
target_class=ea.load_blueprint_class('/Game/AfghanFPS/Blueprints/BP_TargetDummy')
for name,loc in [('P4_PenetrationTarget',(3100,-650,-4)),('P4_ProjectileTarget',(3100,-300,-4)),('P4_LimbTarget',(3100,0,-4)),('P4_RangeTarget',(3300,900,-4)),('P4_BlastTarget',(3110,700,-4)),('P4_ShieldedTarget',(3200,700,-4))]:
    if name not in placed:
        a=sub.spawn_actor_from_class(target_class,unreal.Vector(*loc));a.set_actor_label(name)
if 'P4_ThinCover' not in placed:
    a=sub.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(2950,-650,0));a.set_actor_label('P4_ThinCover')
    c=a.static_mesh_component;c.set_static_mesh(ea.load_asset('/Engine/BasicShapes/Cube'));a.set_actor_scale3d(unreal.Vector(.1,2,2));c.set_collision_profile_name('BlockAll');a.set_editor_property('tags',['Penetrable'])
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('PHASE4_EQUIPMENT_ASSETS_SAVED')
