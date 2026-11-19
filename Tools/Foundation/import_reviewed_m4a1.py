import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
source=Path("D:/GameDev/ThirdPartyReview/Reference_20261119/M4A1/M4A1")
dest="/Game/AfghanFPS/Weapons/Reviewed/M4A1"
task=u.AssetImportTask()
task.set_editor_properties(dict(filename=str(source/"M4A1.fbx"),destination_path=dest,
    destination_name="SM_M4A1",automated=True,save=True,replace_existing=False))
options=u.FbxImportUI()
options.set_editor_properties(dict(import_mesh=True,import_as_skeletal=False,import_animations=False,
    import_materials=False,import_textures=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH))
options.static_mesh_import_data.set_editor_property("combine_meshes",True)
task.options=options
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
print("MODEL_PATHS",task.imported_object_paths)
for asset in task.imported_object_paths:
    m=u.load_asset(asset)
    if isinstance(m,u.StaticMesh):print("MODEL_BOUNDS",m.get_bounds())
tasks=[]
for name in ["Base_Color","Normal","Metallic","Roughness"]:
    t=u.AssetImportTask()
    t.set_editor_properties(dict(filename=str(source/("M4A1_"+name+".png")),destination_path=dest,
        automated=True,save=True,replace_existing=False))
    tasks.append(t)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
print("TEXTURES",[list(t.imported_object_paths) for t in tasks])
