import unreal
for path in ('/Game/AfghanFPS/Weapons/BP_Weapon_Base','/Game/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation'):
    b=unreal.load_asset(path);print('BLUEPRINT',path)
    for graph in unreal.BlueprintEditorLibrary.list_graphs(b):
        print('GRAPH',graph.get_name())
        if str(graph.get_name()) not in ('CanFire',):continue
        for node in unreal.BlueprintGraphEditor.get_graph_editor(graph).list_all_nodes():
            title=str(node.get_node_title())
            print('NODE',title,[(str(p.get_pin_name()),str(p.get_pin_value())) for p in unreal.BlueprintEditorLibrary.list_input_pins(node) if str(p.get_pin_name()) not in ('execute','then','self')])
