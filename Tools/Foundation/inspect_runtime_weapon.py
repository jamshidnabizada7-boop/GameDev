import unreal
world=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];p=unreal.GameplayStatics.get_player_pawn(world,0)
w=p.get_components_by_class(unreal.ChildActorComponent)[0].get_editor_property('child_actor')
print('PARTS',[(c.get_name(),str(c.get_editor_property('relative_location')),str(c.get_editor_property('relative_scale3d'))) for c in w.get_components_by_class(unreal.StaticMeshComponent)])
print('EQUIPMENT',p.equipment.can_act(),p.equipment.last_action,p.equipment.dry_clicks)
