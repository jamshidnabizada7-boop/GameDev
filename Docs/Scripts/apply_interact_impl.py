import unreal, json, time, traceback

# ZCODE INT IMPL - wire the Interact toggle into the EXISTING Event Interact node.
# Event Interact -> Branch(Not bBig) -> GROW(2x) / SHRINK(1x) + PrintString evidence.
# Preserves the user's interface assignment + event node; compiles + saves.

OUT = r"C:\Users\PC\Documents\Default Project\zcode_phase1\zcode_int_impl.json"
res = {"events": [], "tests": {}}
state = {"handle": None, "phase": "wait", "t0": time.time(), "sub": 0}
BEL = unreal.BlueprintEditorLibrary
TEST_PATH = "/Game/AfghanFPS/Interaction/BP_TestInteractable"
IFACE_PATH = "/Game/AfghanFPS/Interaction/Interfaces/BPI_Interactable"


def log(m):
    res["events"].append(m)
    print("ZCODE_INTIMPL: " + m)


def safe(fn, *a, **kw):
    try:
        return fn(*a, **kw)
    except Exception as e:
        return "ERR: " + repr(e)


def dump():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(res, f, indent=2, default=str)


def out_exec(node):
    for p in (safe(BEL.list_output_pins, node) or []):
        if isinstance(p, str):
            continue
        if '"exec"' in str(safe(p.get_pin_type_as_json_schema)):
            return p
    return None


def out_data(node, name):
    for p in (safe(BEL.list_output_pins, node) or []):
        if isinstance(p, str):
            continue
        if str(safe(p.get_pin_name)) == name:
            return p
    return None


def in_data(node, *names):
    names_l = [n.lower() for n in names]
    first = None
    for p in (safe(BEL.list_input_pins, node) or []):
        if isinstance(p, str):
            continue
        nm = str(safe(p.get_pin_name)).lower()
        sc = str(safe(p.get_pin_type_as_json_schema))
        if '"exec"' in sc or nm in ("self",):
            continue
        if first is None:
            first = p
        if nm in names_l:
            return p
    return first


def connect(a, b, label):
    if a is None or b is None or isinstance(a, str) or isinstance(b, str):
        res.setdefault("wire_log", []).append(label + " : PIN-MISSING")
        return False
    try:
        r = str(a.try_create_connection(b))
        res.setdefault("wire_log", []).append(label + " : " + r)
        return r == "True"
    except Exception as e:
        res.setdefault("wire_log", []).append(label + " : ERR " + repr(e))
        return False


def set_val(node, pin_name, val):
    p = in_data(node, pin_name)
    if p is not None and not isinstance(p, str):
        return str(safe(p.set_pin_value, val))
    return "pin missing"


def build():
    tbp = safe(unreal.EditorAssetLibrary.load_asset, TEST_PATH)
    if tbp is None or isinstance(tbp, str):
        log("load fail")
        return False
    eg = safe(BEL.find_event_graph, tbp)
    ge = unreal.BlueprintGraphEditor.get_graph_editor(eg)
    nodes = safe(ge.list_all_nodes) or []
    ovl = None
    call_count = 0
    for n in nodes:
        if isinstance(n, str):
            continue
        cls = str(safe(n.get_class().get_name))
        title = str(safe(n.get_node_title))
        if cls == "K2Node_Event" and "interact" in title.lower():
            ovl = n
        if cls == "K2Node_CallFunction":
            call_count += 1
    res["ovl_found"] = ovl is not None
    res["existing_call_nodes"] = call_count
    if ovl is None:
        log("no Event Interact node")
        return False
    if call_count > 0:
        log("toggle logic already present (%d call nodes) - not duplicating" % call_count)
        res["already_built"] = True
        return True

    getbig = safe(ge.add_get_member_variable_node, "bBig")
    notn = safe(ge.add_call_function_node, "/Script/Engine.KismetMathLibrary.Not_PreBool")
    br = safe(ge.add_branch_node)
    scale2 = safe(ge.add_call_function_node, "/Script/Engine.Actor.K2_SetActorScale3D")
    setbig = safe(ge.add_set_member_variable_node, "bBig")
    pr1 = safe(ge.add_call_function_node, "/Script/Engine.KismetSystemLibrary.PrintString")
    scale1 = safe(ge.add_call_function_node, "/Script/Engine.Actor.K2_SetActorScale3D")
    setbig2 = safe(ge.add_set_member_variable_node, "bBig")
    pr2 = safe(ge.add_call_function_node, "/Script/Engine.KismetSystemLibrary.PrintString")
    ok = {k: ("OK" if v is not None and not isinstance(v, str) else "FAIL")
          for k, v in (("getbig", getbig), ("not", notn), ("branch", br),
                       ("scale2", scale2), ("setbig", setbig), ("pr1", pr1),
                       ("scale1", scale1), ("setbig2", setbig2), ("pr2", pr2))}
    res["nodes_ok"] = ok
    if any(v == "FAIL" for v in ok.values()):
        log("node creation failed: %s" % ok)
        return False

    connect(out_exec(ovl), safe(BEL.find_execute_pin, br), "ovl->br")
    connect(out_data(getbig, "bBig"), in_data(notn, "A"), "bBig->not")
    connect(out_data(notn, "ReturnValue"), in_data(br, "Condition"), "not->br.cond")
    connect(safe(BEL.find_then_pin, br), safe(BEL.find_execute_pin, scale2), "br.then->scale2")
    res["scale2_set"] = set_val(scale2, "NewScale3D", "2, 2, 2")
    safe(BEL.find_input_pin(setbig, "bBig").set_pin_value, "true")
    connect(safe(BEL.find_then_pin, scale2), safe(BEL.find_execute_pin, setbig), "scale2->setbig")
    connect(safe(BEL.find_then_pin, setbig), safe(BEL.find_execute_pin, pr1), "setbig->pr1")
    res["pr1_set"] = set_val(pr1, "InString", "INTERACTED - GROW")
    connect(safe(BEL.find_else_pin, br), safe(BEL.find_execute_pin, scale1), "br.else->scale1")
    res["scale1_set"] = set_val(scale1, "NewScale3D", "1, 1, 1")
    safe(BEL.find_input_pin(setbig2, "bBig").set_pin_value, "false")
    connect(safe(BEL.find_then_pin, scale1), safe(BEL.find_execute_pin, setbig2), "scale1->setbig2")
    connect(safe(BEL.find_then_pin, setbig2), safe(BEL.find_execute_pin, pr2), "setbig2->pr2")
    res["pr2_set"] = set_val(pr2, "InString", "INTERACTED - SHRINK")

    res["compile"] = str(safe(BEL.compile_blueprint, tbp))
    res["status"] = str(safe(BEL.get_editor_property, tbp, "Status"))
    if res["status"].endswith("BS_UP_TO_DATE: 3>"):
        res["save"] = str(safe(unreal.EditorAssetLibrary.save_asset, TEST_PATH))
        iface = safe(unreal.EditorAssetLibrary.load_asset, IFACE_PATH)
        a = safe(unreal.EditorLevelLibrary.spawn_actor_from_class, tbp.generated_class(), unreal.Vector(5000, 5000, 200))
        if a is not None and not isinstance(a, str):
            res["does_implement"] = str(safe(unreal.SystemLibrary.does_implement_interface, a, iface.generated_class()))
            safe(unreal.EditorLevelLibrary.destroy_actor, a)
    log("toggle logic built")
    return True


def tick(*args):
    try:
        if state["phase"] == "wait":
            if time.time() - state["t0"] > 1.0:
                state["phase"] = "done"
                build()
                dump()
    except Exception:
        res["fatal"] = traceback.format_exc()
        dump()
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
log("int impl registered")
dump()
state["handle"] = unreal.register_slate_post_tick_callback(tick)
print("ZCODE_INTIMPL_REGISTERED")
