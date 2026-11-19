import unreal

EAL=unreal.EditorAssetLibrary
BEL=unreal.BlueprintEditorLibrary
SDS=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
BP=EAL.load_asset('/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation')
for h in SDS.k2_gather_subobject_data_for_blueprint(BP):
    data=SDS.k2_find_subobject_data_from_handle(h)
    c=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if not c:continue
    if 'FP_WeaponMount' in c.get_name():
        c.set_editor_property('relative_location',unreal.Vector(41,12,-11))
    if 'FP_WeaponHolder' in c.get_name():
        c.set_editor_property('relative_scale3d',unreal.Vector(0.7,0.7,0.7))
BEL.compile_blueprint(BP)
assert EAL.save_loaded_asset(BP,False)

# Inspect the actual template tree, rather than editing null CDO bindings.
hud=EAL.load_asset('/Game/AfghanFPS/UI/UI_AmmoCounter')
root=unreal.get_default_object(hud.generated_class()).get_editor_property('HealthText').get_parent()
def visit(w):
    if hasattr(w,'get_text'): print('HUD_TEMPLATE',w.get_name(),w.get_text())
    if w.get_name()=='Team2Score':
        w.set_visibility(unreal.SlateVisibility.COLLAPSED)
        print('COLLAPSED unused template Team2Score zero')
    if isinstance(w,unreal.PanelWidget):
        for c in w.get_all_children():visit(c)
visit(root)
BEL.compile_blueprint(hud)
assert EAL.save_loaded_asset(hud,False)
print('PRESENTATION_SAVED')

