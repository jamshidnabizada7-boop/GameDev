"""Verify the fitted rifle through player input, including its render projection."""
from pathlib import Path
import unreal

folder = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Tools/Foundation"
exec(compile((folder / "test_dynamic_grips.py").read_bytes(), "grip_fixture", "exec"))
state["ready"] = True
st.clear()
for npc in unreal.GameplayStatics.get_all_actors_of_class(ew, unreal.FoundationNPC):
    npc.set_ai_enabled(False)
start_shots = eq.shots_fired
report = {}
digits = ("thumb", "index", "middle", "ring", "pinky")
targets = dict(zip([f"{digit}_03_{side}" for side in ("l", "r") for digit in digits],
                   eq.slots[0].definition.grip_finger_contacts, strict=True))

def observe(action):
    measure(action)
    row = report.setdefault(action, {"projection": True, "contact_cm": 0, "observed": False})
    weapon = eweapon()
    receiver = next(c for c in weapon.get_components_by_class(unreal.StaticMeshComponent)
                    if c.get_name() == "Mesh_Receiver")
    row["projection"] &= receiver.first_person_primitive_type == ep.p2_arms.first_person_primitive_type
    if weapon.get_class().get_name() == "BP_M4A1Reviewed_C":
        for bone, target in targets.items():
            contact = weapon.get_actor_transform().transform_location(target)
            error = (contact - ep.p2_arms.get_socket_location(bone)).length()
            row["contact_cm"] = max(row["contact_cm"], error)
    row["observed"] |= {
        "walk": ep.get_velocity().length() > 100,
        "sprint": ep.p2_sprinting and ep.get_velocity().length() > 450,
        "crouch": ep.p2_stance == 1,
        "aim": eq.ads_alpha > .8,
        "fire": eq.shots_fired > start_shots,
        "reload": eq.reloading,
        "switch": eq.equipping,
        "return": eq.active_slot == 0 and not eq.equipping,
    }[action]

def save():
    passed = all(row["observed"] and row["projection"] and row["contact_cm"] < 3.3
                 and samples[action]["correct_owner"] and samples[action]["max_cm"] < 2
                 for action, row in report.items()) and len(report) == 8
    path = folder.parents[1] / "Saved/Foundation/fitted_grip_actions.json"
    path.write_text(json.dumps(dict(passed=passed, actions=report, wrists=samples), indent=2))
    perf.set_editor_property("bThrottleCPUWhenNotForeground", old)

step("place", .7, eplace)
step("walk", .8, None, lambda: (ek("W"), observe("walk")))
def open_lane():
    ek("W", False)
    eplace()
    ep.set_actor_location(unreal.Vector(1600, 700, -1.8), False, True)

step("open_lane", .3, open_lane)
step("sprint", .9, None, lambda: (ek("W"), ek("LeftShift"), observe("sprint")))
step("stop", .4, lambda: (ek("W", False), ek("LeftShift", False)))
step("crouch", .8, None, lambda: (ek("LeftControl"), observe("crouch")))
step("stand", .5, lambda: ek("LeftControl", False))
step("aim", .8, None, lambda: (ek("RightMouseButton"), observe("aim")))
step("fire", .6, None, lambda: (ek("LeftMouseButton"), observe("fire")))
step("release", .5, lambda: (ek("LeftMouseButton", False), ek("RightMouseButton", False)))
step("prepare_reload", .2, lambda: eammo(5, 30))
step("reload", 1., lambda: ek("R"), lambda: observe("reload"))
step("release_reload", 1., lambda: ek("R", False))
step("focus", .6, lambda: eaim(eactor("P3_Weapon")))
step("pickup", 1.2, None, lambda: ek("E"))
step("release_use", .4, lambda: ek("E", False))
step("switch", 1., lambda: ek("Two"), lambda: observe("switch"))
step("release_switch", .4, lambda: ek("Two", False))
tap("switch_back", "One", .8)
step("return", .5, None, lambda: observe("return"))
step("finish", .1, save)
