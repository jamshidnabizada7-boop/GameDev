"""Run inside the existing editor after building the native project module."""
import unreal
import json

BEL = unreal.BlueprintEditorLibrary
EAL = unreal.EditorAssetLibrary
BP = EAL.load_asset('/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation')
assert BP
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()

# Reparent retains the existing Blueprint, components, inputs and combat graphs.
BEL.reparent_blueprint(BP, unreal.Phase2Character)
result = unreal.Phase2Tools.integrate_player(BP)
print('INTEGRATION', result)
assert result.startswith('COMPILED'), result

paths = [BP.get_path_name(), '/Game/AfghanFPS/Weapons/BP_Weapon_Base',
         '/Game/AfghanFPS/Blueprints/BP_TargetDummy', '/Game/AfghanFPS/UI/UI_AmmoCounter',
         '/Game/AfghanFPS/Interaction/BP_TestInteractable']
compiled = {}
for path in paths:
    asset = EAL.load_asset(path)
    if not asset:
        continue
    # Preserve diagnostic nodes and execution flow, disable their normal output.
    for graph in BEL.list_graphs(asset):
        ge = unreal.BlueprintGraphEditor.get_graph_editor(graph)
        for node in ge.list_all_nodes():
            title = str(node.get_node_title())
            for pin in BEL.list_input_pins(node):
                name = str(pin.get_pin_name())
                if ('Print String' in title or 'PrintString' in title) and name in ('bPrintToScreen', 'bPrintToLog'):
                    pin.set_pin_value('false')
                if name == 'DrawDebugType':
                    pin.set_pin_value('None')
    BEL.compile_blueprint(asset)
    compiled[path] = EAL.save_loaded_asset(asset, only_if_is_dirty=False)
print('SAVED', json.dumps(compiled))

# Capture current HUD, weapon values, and mapping context for targeted repair.
for path in ['/Game/AfghanFPS/UI/UI_AmmoCounter', '/Game/AfghanFPS/Weapons/BP_Weapon_Base']:
    a = EAL.load_asset(path)
    print('ASSET', path)
    for graph in BEL.list_graphs(a):
        if str(graph.get_name()) not in ('UpdateHealth', 'UpdateAmmo', 'TryFire', 'EventGraph', 'UserConstructionScript'):
            continue
        print('GRAPH', graph.get_name())
        for n in unreal.BlueprintGraphEditor.get_graph_editor(graph).list_all_nodes():
            print(n.get_name(), n.get_node_title(), [(str(p.get_pin_name()), str(p.get_pin_value()),
                  [str(q.get_owning_node().get_name()) for q in p.list_connected_pins()]) for p in BEL.list_input_pins(n)])

imc = EAL.load_asset('/Game/Input/IMC_Default')
for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings'):
    print('MAPPING', m.get_editor_property('action'), m.get_editor_property('key'))
print('INTEGRATION_DONE')

