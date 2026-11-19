import unreal, os, wave, struct, math, random, json
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
EAL=unreal.EditorAssetLibrary
BEL=unreal.BlueprintEditorLibrary
AT=unreal.AssetToolsHelpers.get_asset_tools()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
by_name={a.get_actor_label():a for a in actors.get_all_level_actors()}
ROOT='/Game/AfghanFPS/Interaction/Foundation'
EAL.make_directory(ROOT)

# Original, small non-verbal room/footstep-like cue; no downloaded audio.
audio_dir='C:/GameDev/AfghanistanFPS/SourceAssets/Audio'
os.makedirs(audio_dir,exist_ok=True)
wav_path=audio_dir+'/FoundationRoom.wav'
if not os.path.exists(wav_path):
    rng=random.Random(3941);rate=22050;samples=[];filtered=0
    for i in range(rate*4):
        t=i/rate;filtered=.8*filtered+.2*rng.uniform(-1,1)
        env=sum(math.exp(-(t-p)*28) for p in (.4,1.4,2.4,3.4) if p<=t<p+.3)
        value=.015*math.sin(t*math.tau*110)+filtered*env*.35
        samples.append(struct.pack('<h',int(max(-1,min(1,value))*32767)))
    with wave.open(wav_path,'wb') as f:f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(b''.join(samples))
sound_path='/Game/AfghanFPS/Audio/Foundation/FoundationRoom'
if not EAL.does_asset_exist(sound_path):
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':wav_path,'destination_path':'/Game/AfghanFPS/Audio/Foundation','automated':True,'save':True})
    AT.import_asset_tasks([task])
sound=EAL.load_asset(sound_path);assert sound
sound.set_editor_property('looping',True);EAL.save_loaded_asset(sound,False)
use_path='/Game/AfghanFPS/Audio/Foundation/FoundationUse'
if not EAL.does_asset_exist(use_path):
    use_wav=audio_dir+'/FoundationUse.wav';rate=22050
    with wave.open(use_wav,'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate)
        f.writeframes(b''.join(struct.pack('<h',int(5000*math.exp(-i/(rate*.025))*math.sin(i/rate*math.tau*720))) for i in range(int(rate*.12))))
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':use_wav,'destination_path':'/Game/AfghanFPS/Audio/Foundation','automated':True,'save':True});AT.import_asset_tasks([task])
use_sound=EAL.load_asset(use_path);assert use_sound

action_path='/Game/Input/Actions/IA_InteractionAdjust'
if EAL.does_asset_exist(action_path):adjust=EAL.load_asset(action_path)
else:adjust=EAL.duplicate_asset('/Game/Input/Actions/IA_Interact',action_path)
adjust.set_editor_property('value_type',unreal.InputActionValueType.AXIS1D)
imc=EAL.load_asset('/Game/Input/IMC_Default')
assert unreal.Phase2Tools.map_input(imc,adjust,'MouseWheelAxis')
EAL.save_loaded_asset(adjust,False);EAL.save_loaded_asset(imc,False)
player_bp=EAL.load_asset('/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation')
print('INPUT_INTEGRATION',unreal.Phase2Tools.integrate_player(player_bp))
assert EAL.save_loaded_asset(player_bp,False)

kinds=unreal.FoundationInteractableKind
definitions=[('BP_InteractableDoor',kinds.DOOR),('BP_InteractableDrawer',kinds.DRAWER),
 ('BP_InteractableSwitch',kinds.LIGHT_SWITCH),('BP_LootContainer',kinds.LOOT_CRATE),
 ('BP_BreakableCrate',kinds.BREAKABLE),('BP_AmmoPickup',kinds.AMMO),
 ('BP_HealthPickup',kinds.HEALTH),('BP_WorldWeaponPickup',kinds.WEAPON),('BP_KeyPickup',kinds.KEY)]
classes={}
for name,kind in definitions:
    path=ROOT+'/'+name
    if EAL.does_asset_exist(path):bp=EAL.load_asset(path)
    else:
        factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.FoundationInteractable)
        bp=AT.create_asset(name,ROOT,unreal.Blueprint,factory)
    assert bp
    bp.modify();unreal.get_default_object(bp.generated_class()).set_editor_property('kind',kind)
    assert BEL.compile_blueprint(bp)
    assert EAL.save_loaded_asset(bp,False)
    classes[name]=bp.generated_class()

cube=EAL.load_asset('/Engine/BasicShapes/Cube')
def box(name,loc,size):
    a=by_name.get(name)
    if not a:a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc));a.set_actor_label(name);by_name[name]=a
    c=a.static_mesh_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_static_mesh(cube)
    a.set_actor_location(unreal.Vector(*loc),False,False);a.set_actor_scale3d(unreal.Vector(*(s/100 for s in size)))
    c.set_collision_profile_name('BlockAll');c.set_simulate_physics(False);c.set_mobility(unreal.ComponentMobility.STATIC)
    return a
def label(name,text,loc,size=20):
    a=by_name.get(name)
    if not a:a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*loc));a.set_actor_label(name);by_name[name]=a
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
    c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(text);c.set_world_size(size)
    c.set_text_render_color(unreal.Color(255,220,80,255));return a

# Extend the same non-story sandbox through a real opening in the east wall.
box('StaticMeshActor4',(1025,-600,100),(50,800,400))
box('P3_EastWallUpper',(1025,600,100),(50,800,400))
box('P3_AnnexFloor',(2200,0,-150),(2400,2000,100))
box('P3_AnnexNorth',(2200,1025,100),(2400,50,400))
box('P3_AnnexSouth',(2200,-1025,100),(2400,50,400))
box('P3_AnnexEast',(3425,0,100),(50,2000,400))
label('P3_EntryLabel','INTERACTION TEST ANNEX  >',(980,-180,200))

def place(name,bpname,kind,loc,item='ammo',amount=30,locked=False):
    a=by_name.get(name)
    if not a:a=actors.spawn_actor_from_class(classes[bpname],unreal.Vector(*loc));a.set_actor_label(name);by_name[name]=a
    a.set_actor_location(unreal.Vector(*loc),False,False)
    a.set_editor_properties({'kind':kind,'stable_id':name,'item_id':item,'item_amount':amount,'locked':locked})
    a.set_editor_property('use_sound',use_sound)
    if kind==kinds.DOOR:a.set_editor_property('interior_sound',sound)
    a.configure(kind)
    return a

door=place('P3_ArmoryDoor','BP_InteractableDoor',kinds.DOOR,(1800,-700,-100),locked=True)
peek=place('P3_PeekDoor','BP_InteractableDoor',kinds.DOOR,(1800,-100,-100))
for prefix,y in [('P3_Armory',-700),('P3_Peek',-100)]:
    box(prefix+'_LeftFrame',(1800,y-15,20),(25,20,240))
    box(prefix+'_RightFrame',(1800,y+125,20),(25,20,240))
    box(prefix+'_Header',(1800,y+55,140),(25,160,30))
    box(prefix+'_RoomBack',(2300,y+55,30),(20,400,260))
    box(prefix+'_RoomLeft',(2050,y-145,30),(520,20,260))
    box(prefix+'_RoomRight',(2050,y+255,30),(520,20,260))
    label(prefix+'_Label','E tap: door | hold: listen | hold + wheel: 5 deg\nCrouch + E: mirror | Q / F: peek crack',(1580,y-120,200),14)

place('P3_ArmoryKey','BP_KeyPickup',kinds.KEY,(1500,-450,-100),'armory_key',1)
place('P3_Lockpick','BP_KeyPickup',kinds.KEY,(1500,-300,-100),'lockpick',1)
place('P3_Ammo','BP_AmmoPickup',kinds.AMMO,(1450,250,-100),'ammo',30)
place('P3_Health','BP_HealthPickup',kinds.HEALTH,(1450,450,-100),'health',25)
place('P3_Weapon','BP_WorldWeaponPickup',kinds.WEAPON,(1450,650,-100),'weapon_rifle_alt',1)
place('P3_Drawer','BP_InteractableDrawer',kinds.DRAWER,(1900,450,-100))
switch=place('P3_Switch','BP_InteractableSwitch',kinds.LIGHT_SWITCH,(1900,700,-100))
lamp=by_name.get('P3_LinkedLamp')
if not lamp:
    lamp=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(2100,700,80));lamp.set_actor_label('P3_LinkedLamp')
lamp.get_component_by_class(unreal.PointLightComponent).set_intensity(1500)
switch.set_editor_property('linked_lights',[lamp])
place('P3_Loot','BP_LootContainer',kinds.LOOT_CRATE,(2200,450,-100),'ammo',45)
place('P3_Breakable','BP_BreakableCrate',kinds.BREAKABLE,(2450,450,-100))
place('P3_BulletBreakable','BP_BreakableCrate',kinds.BREAKABLE,(2800,450,-100))
for name,text,loc in [('P3_ItemsLabel','Hold E: key / tool / pickups',(1260,200,180)),
 ('P3_PropsLabel','E: drawer / switch / breakable | Hold E: loot',(1740,350,200))]:label(name,text,loc,16)

dummy_class=EAL.load_blueprint_class('/Game/AfghanFPS/Blueprints/BP_TargetDummy')
if 'P3_MirrorDummy' not in by_name:
    a=actors.spawn_actor_from_class(dummy_class,unreal.Vector(2100,-645,-4));a.set_actor_label('P3_MirrorDummy')
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
if 'P3_PeekMirrorDummy' not in by_name:
    a=actors.spawn_actor_from_class(dummy_class,unreal.Vector(2100,-45,-4));a.set_actor_label('P3_PeekMirrorDummy')
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('PHASE3_ASSETS_SAVED',len(definitions),'Blueprint classes; 11 interactable fixtures; original cube preserved')
