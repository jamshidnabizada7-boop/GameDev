"""Add a reusable training terminal to the sandbox map."""
import unreal as ue
level = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
assert not level.is_in_play_in_editor()
assert world.get_path_name().startswith("/Game/AfghanFPS/TestMaps/L_Test_PlayerFoundation.")
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
label = "Foundation_TrainingTerminal"
matches = [a for a in actors.get_all_level_actors() if a.get_actor_label() == label]
assert len(matches) <= 1
# Keep the jump/traversal lane straight ahead of the PlayerStart unobstructed.
position = ue.Vector(150, -680, -98)
terminal = matches[0] if matches else actors.spawn_actor_from_class(ue.FoundationInteractable, position)
assert isinstance(terminal, ue.FoundationInteractable)
terminal.set_actor_label(label)
terminal.set_actor_location(position, False, False)
terminal.set_editor_properties({"stable_id": ue.Name(label),
                                "kind": ue.FoundationInteractableKind.TERMINAL})
terminal.configure(ue.FoundationInteractableKind.TERMINAL)
assert level.save_current_level()
print("Training terminal saved in the foundation sandbox.")
