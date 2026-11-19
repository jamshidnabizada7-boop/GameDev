"""Repair collision fixtures in the existing foundation map, preserving assets."""
import unreal

E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not L.is_in_play_in_editor()
actors = {a.get_actor_label(): a for a in E.get_all_level_actors()}
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
floor = -100.0  # Measured floor top, previously mistaken for zero.

def box(name, center, size):
    a = actors.get(name)
    if a is None:
        a = E.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center))
        a.set_actor_label(name)
        actors[name] = a
    c = a.static_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_static_mesh(cube)
    a.set_actor_location(unreal.Vector(*center), False, False)
    a.set_actor_scale3d(unreal.Vector(*(v/100.0 for v in size)))
    c.set_collision_profile_name('BlockAll')
    c.set_simulate_physics(False)
    c.set_mobility(unreal.ComponentMobility.STATIC)
    return a

def label(name, text, loc, size=18):
    a = actors.get(name)
    if a is None:
        a = E.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*loc), unreal.Rotator(pitch=0,yaw=180,roll=0))
        a.set_actor_label(name)
        actors[name] = a
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(text)
    c.set_world_size(size)
    c.set_text_render_color(unreal.Color(255,220,80,255))
    return a

def tunnel(prefix, y, clearance):
    x = -600
    box(prefix+'_Roof', (x,y,floor+clearance+15), (260,240,30))
    for side in (-1,1):
        box(prefix+('_Left' if side<0 else '_Right'), (x,y+side*130,floor+120), (260,20,240))
    label(prefix+'_Label',f'{prefix.replace("P2_", "")} | {clearance} cm clearance', (x-150,y-100,floor+250))

tunnel('P2_Crouch', -250, 134)
tunnel('P2_Prone', 350, 76)
box('StaticMeshActor6',(-600,-930,20),(260,20,240))
box('StaticMeshActor7',(-600,-670,20),(260,20,240))
box('StaticMeshActor8',(-600,-800,125),(260,280,30))
for wall_name,center,size in (
    ('StaticMeshActor2',(0,1025,100),(2000,50,400)),
    ('StaticMeshActor3',(0,-1025,100),(2000,50,400)),
    ('StaticMeshActor4',(1025,0,100),(50,2000,400)),
    ('StaticMeshActor5',(-1025,0,100),(50,2000,400))):
    box(wall_name,center,size)
box('TRAVERSAL_VAULT_LOW', (250,-800,floor+35), (90,200,70))
box('TRAVERSAL_CLIMB_MID', (550,-800,floor+60), (130,200,120))
box('TRAVERSAL_INVALID_TALL', (800,-800,floor+150), (80,200,300))
label('P2_Vault_Label','G: VAULT | 70 cm', (185,-900,100))
label('P2_Climb_Label','G: CLIMB | 120 cm', (420,-900,160))
label('P2_Tall_Label','REJECT | 300 cm', (750,-900,240))
label('P2_Standing_Label','STANDING PASSAGE | 210 cm', (-750,-900,180))
label('P2_Lean_Label','Q / F LEAN WALL TEST', (50,-980,170))
label('P2_Combat_Label','COMBAT / TARGET | E INTERACT', (450,-500,170))
label('P2_Controls','SHIFT Sprint | CTRL Crouch | Z Prone\nQ / F Lean | G Vault / Climb\nE Interact | LMB Fire | R Restart when dead', (-200,50,160),14)
if actors.get('BP_TestInteractable'):
    actors['BP_TestInteractable'].set_actor_location(unreal.Vector(200,-400,-50),False,False)

# Correct the spawn height to the actual floor and keep the existing start.
start = actors.get('PlayerStart')
if start:
    start.set_actor_location(unreal.Vector(0,-800,floor+98),False,False)
for preview in actors.values():
    if isinstance(preview,unreal.Phase2Character) and preview.get_actor_location().z>5000:
        preview.set_editor_property('auto_possess_ai',unreal.AutoPossessAI.DISABLED)
        preview.set_editor_property('is_editor_only_actor',True)
        preview.set_actor_hidden_in_game(True)
        preview.set_actor_enable_collision(False)

assert L.save_current_level()
print('MAP_SAVED: floor -100; standing 192, crouch 120, prone 60 cm capsules; static fixtures')
