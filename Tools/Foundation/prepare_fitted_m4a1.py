import unreal as u
import sys
ea=u.EditorAssetLibrary
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
bp=ea.load_asset("/Game/AfghanFPS/Weapons/Reviewed/M4A1/BP_M4A1Reviewed")
sub=u.get_engine_subsystem(u.SubobjectDataSubsystem)
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    c=u.SubobjectDataBlueprintFunctionLibrary.get_object(sub.k2_find_subobject_data_from_handle(h))
    if not c:continue
    if c.get_name().startswith("GripPoint_Left"):c.set_editor_property("relative_location",u.Vector(30,-11,-4.5))
    if c.get_name().startswith("GripPoint_Right"):c.set_editor_property("relative_location",u.Vector(-18,4,-4))
assert u.BlueprintEditorLibrary.compile_blueprint(bp)
d=ea.load_asset("/Game/AfghanFPS/Weapons/Foundation/DA_RifleFoundation")
d.actor_class=bp.generated_class();d.display_name="M4A1";d.sight_height=7.
d.use_grip_pose=True
d.left_wrist_rotation=u.Rotator(pitch=0,yaw=90,roll=90)
d.right_wrist_rotation=u.Rotator(pitch=130,yaw=0,roll=0)
d.left_curl_axis=u.Vector(0,0,1);d.right_curl_axis=u.Vector(0,0,1)
d.left_finger_curl=-53;d.right_finger_curl=72
if "--save" in sys.argv:
    assert ea.save_loaded_asset(bp,False)
    assert ea.save_loaded_asset(d,False)
else:
    perf=u.get_default_object(u.load_class(None,"/Script/UnrealEd.EditorPerformanceSettings"))
    perf.set_editor_property("bThrottleCPUWhenNotForeground",False)
    u.Phase2Tools.begin_player_test()
