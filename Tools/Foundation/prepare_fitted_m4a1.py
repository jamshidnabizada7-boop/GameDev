import unreal as u
import sys
ea=u.EditorAssetLibrary
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
bp=ea.load_asset("/Game/AfghanFPS/Weapons/Reviewed/M4A1/BP_M4A1Reviewed")
sub=u.get_engine_subsystem(u.SubobjectDataSubsystem)
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    c=u.SubobjectDataBlueprintFunctionLibrary.get_object(sub.k2_find_subobject_data_from_handle(h))
    if not c:continue
    if c.get_name().startswith("GripPoint_Left"):c.set_editor_property("relative_location",u.Vector(16,-8,-4))
    if c.get_name().startswith("GripPoint_Right"):c.set_editor_property("relative_location",u.Vector(-21.5,2.3,-7.7))
assert u.BlueprintEditorLibrary.compile_blueprint(bp)
d=ea.load_asset("/Game/AfghanFPS/Weapons/Foundation/DA_RifleFoundation")
d.actor_class=bp.generated_class();d.display_name="M4A1";d.sight_height=7.
d.use_grip_pose=True
d.left_wrist_rotation=u.Rotator(pitch=2.9,yaw=52.2,roll=101.3)
d.right_wrist_rotation=u.Rotator(pitch=.25,yaw=-171.5,roll=6.6)
d.left_curl_axis=u.Vector(0,0,1);d.right_curl_axis=u.Vector(0,0,1)
d.left_finger_curl=-25;d.right_finger_curl=-40
d.grip_finger_contacts=[u.Vector(*point) for point in (
    (26,-3.4,3),(27,3.5,3.4),(24,3.7,3.5),(21,3.5,3),(18,3.3,2),
    (-9,-2.2,-2),(-3,1.8,-4.5),(-8,-2.1,-7.3),(-9.5,-2,-9.6),(-11,-1.8,-11.8))]
d.grip_finger_bends=[u.Vector(*point) for point in (
    (23,-7,4),(28,8,-1),(24,9,-1),(21,8,-1),(18,8,-1),
    (-15,-6,0),(-2,5,-3),(-4,0,-7),(-5,0,-10),(-7,0,-12))]
if "--save" in sys.argv:
    assert ea.save_loaded_asset(bp,False)
    assert ea.save_loaded_asset(d,False)
else:
    perf=u.get_default_object(u.load_class(None,"/Script/UnrealEd.EditorPerformanceSettings"))
    perf.set_editor_property("bThrottleCPUWhenNotForeground",False)
    u.Phase2Tools.begin_player_test()
