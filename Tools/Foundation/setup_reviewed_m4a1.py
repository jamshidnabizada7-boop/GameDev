import unreal as u
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
ea=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
dest="/Game/AfghanFPS/Weapons/Reviewed/M4A1"
mat=at.create_asset("M_M4A1",dest,u.Material,u.MaterialFactoryNew()) if not ea.does_asset_exist(dest+"/M_M4A1") else ea.load_asset(dest+"/M_M4A1")
for name,prop,sampler in [
    ("Base_Color",u.MaterialProperty.MP_BASE_COLOR,u.MaterialSamplerType.SAMPLERTYPE_COLOR),
    ("Normal",u.MaterialProperty.MP_NORMAL,u.MaterialSamplerType.SAMPLERTYPE_NORMAL),
    ("Metallic",u.MaterialProperty.MP_METALLIC,u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
    ("Roughness",u.MaterialProperty.MP_ROUGHNESS,u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)]:
    tex=ea.load_asset(dest+"/M4A1_"+name)
    if name in ("Metallic","Roughness"):tex.set_editor_property("srgb",False);ea.save_loaded_asset(tex,False)
    n=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTextureSample,-300,0)
    n.texture=tex;n.sampler_type=sampler
    u.MaterialEditingLibrary.connect_material_property(n,"RGB" if name in ("Base_Color","Normal") else "R",prop)
u.MaterialEditingLibrary.recompile_material(mat);ea.save_loaded_asset(mat,False)
mesh=ea.load_asset(dest+"/SM_M4A1");mesh.set_material(0,mat);ea.save_loaded_asset(mesh,False)
bp=ea.load_asset(dest+"/BP_M4A1Reviewed") if ea.does_asset_exist(dest+"/BP_M4A1Reviewed") else ea.duplicate_asset("/Game/AfghanFPS/Weapons/BP_Weapon_Base",dest+"/BP_M4A1Reviewed")
sub=u.get_engine_subsystem(u.SubobjectDataSubsystem)
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    c=u.SubobjectDataBlueprintFunctionLibrary.get_object(sub.k2_find_subobject_data_from_handle(h))
    if not c:continue
    if isinstance(c,u.StaticMeshComponent):
        if c.get_name().startswith("Mesh_Receiver"):
            c.set_static_mesh(mesh)
            c.set_editor_properties(dict(relative_scale3d=u.Vector(1,1,1),relative_rotation=u.Rotator(yaw=90),relative_location=u.Vector(0,0,0)))
        else:c.set_editor_property("visible",False)
assert u.BlueprintEditorLibrary.compile_blueprint(bp)
ea.save_loaded_asset(bp,False)
# Keep the rifle definition unchanged while visually evaluating this candidate.
print("REVIEW_BLUEPRINT",bp.get_path_name())
