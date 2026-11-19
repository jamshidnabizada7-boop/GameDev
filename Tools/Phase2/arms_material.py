"""Give the existing FP mannequin an arms-only material mask.

Pre-skinned coordinates select original Manny arm vertices. The mask follows
animation and camera movement and does not cut the torso based on world height.
Existing third-person mesh/materials and their shadows are preserved.
"""
import unreal
EAL=unreal.EditorAssetLibrary
MEL=unreal.MaterialEditingLibrary
root='/Game/AfghanFPS/Characters/Player/FirstPerson'
EAL.make_directory(root)
BP=EAL.load_asset('/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation')
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
templates=[]
for handle in sub.k2_gather_subobject_data_for_blueprint(BP):
    obj=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(sub.k2_find_subobject_data_from_handle(handle))
    if isinstance(obj,unreal.SkeletalMeshComponent) and obj.get_name().startswith('FirstPersonMesh'):
        if obj not in templates:templates.append(obj)
assert templates
cache={}
def masked(source):
    path=source.get_path_name()
    if path in cache:return cache[path]
    if path.startswith(root+'/'):return source
    if isinstance(source,unreal.MaterialInstanceConstant):
        parent=masked(source.get_editor_property('parent'))
        target=root+'/'+source.get_name()+'_P2Arms'
        inst=EAL.load_asset(target) if EAL.does_asset_exist(target) else EAL.duplicate_asset(path,target)
        MEL.set_material_instance_parent(inst,parent)
        EAL.save_loaded_asset(inst,False);cache[path]=inst
        return inst
    assert isinstance(source,unreal.Material), source
    target=root+'/'+source.get_name()+'_P2Arms'
    if EAL.does_asset_exist(target):
        mat=EAL.load_asset(target)
    else:
        mat=EAL.duplicate_asset(path,target)
    if EAL.get_metadata_tag(mat,'Phase2ArmsMaskVersion')!='2':
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
        pos=MEL.create_material_expression(mat,unreal.MaterialExpressionPreSkinnedPosition,-1000,800)
        vi=MEL.create_material_expression(mat,unreal.MaterialExpressionVertexInterpolator,-900,800)
        assert MEL.connect_material_expressions(pos,'',vi,'')
        x=MEL.create_material_expression(mat,unreal.MaterialExpressionComponentMask,-800,800)
        x.set_editor_properties({'r':True,'g':False,'b':False,'a':False})
        z=MEL.create_material_expression(mat,unreal.MaterialExpressionComponentMask,-800,1000)
        z.set_editor_properties({'r':False,'g':False,'b':True,'a':False})
        ab=MEL.create_material_expression(mat,unreal.MaterialExpressionAbs,-600,800)
        lateral=MEL.create_material_expression(mat,unreal.MaterialExpressionSubtract,-400,800)
        lateral.set_editor_property('const_b',20.0)
        upper=MEL.create_material_expression(mat,unreal.MaterialExpressionSubtract,-400,1000)
        upper.set_editor_property('const_b',85.0)
        both=MEL.create_material_expression(mat,unreal.MaterialExpressionMin,-200,900)
        for a,output,b,input_name in ((vi,'',x,''),(vi,'',z,''),(x,'',ab,''),
                (ab,'',lateral,'A'),(z,'',upper,'A'),(lateral,'',both,'A'),(upper,'',both,'B')):
            assert MEL.connect_material_expressions(a,output,b,input_name),(a.get_class().get_name(),b.get_class().get_name(),input_name)
        assert MEL.connect_material_property(both,'',unreal.MaterialProperty.MP_OPACITY_MASK)
        EAL.set_metadata_tag(mat,'Phase2ArmsMaskVersion','2')
        MEL.recompile_material(mat)
        EAL.save_loaded_asset(mat,False)
    cache[path]=mat
    return mat
masked(EAL.load_asset('/Game/Characters/Mannequins/Materials/M_Mannequin'))
for template in templates:
    for i,material in enumerate(template.get_materials()):
        if material:template.set_material(i,masked(material))
unreal.BlueprintEditorLibrary.compile_blueprint(BP)
assert EAL.save_loaded_asset(BP,False)
print('ARMS_MASK_SAVED',[(k,v.get_path_name()) for k,v in cache.items()])



