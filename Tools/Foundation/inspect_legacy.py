import unreal
b=unreal.load_asset('/Game/AfghanFPS/Interaction/BP_TestInteractable')
for graph in unreal.BlueprintEditorLibrary.list_graphs(b):
    print('GRAPH',graph.get_name())
    for node in unreal.BlueprintGraphEditor.get_graph_editor(graph).list_all_nodes():
        print('NODE',node.get_node_title(),[(str(p.get_pin_name()),str(p.get_pin_value())) for p in unreal.BlueprintEditorLibrary.list_input_pins(node)])
        if str(node.get_node_title())=='Event Interact':print('EVENT_OUTPUTS',[(str(p.get_pin_name()),str(p.get_pin_value())) for p in unreal.BlueprintEditorLibrary.list_output_pins(node)])
w=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='BP_TestInteractable_C')
print('CUBE',a.get_actor_label(),a.get_actor_location(),a.get_actor_bounds(False))
