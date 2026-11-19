import unreal
import json
import os
import time
import traceback

OUT = r"C:\GameDev\AfghanistanFPS\Docs\Foundation\hud_pie_results.json"
SHOT_DIR = r"C:\GameDev\AfghanistanFPS\Saved\Screenshots\WindowsEditor"

res = {
    "started_at": time.strftime("%Y-%m-%d %H:%M:%S"),
    "events": [],
    "tests": {},
    "shots": []
}

st = {
    "phase": "boot",
    "sub": 0,
    "t0": time.time(),
    "world": None,
    "pc": None,
    "pawn": None,
    "holder": None,
    "weapon": None,
    "comp": None,
    "hud": None,
    "busy": False,
    "pending": None,
    "wait": 0
}


def log(msg):
    elapsed = time.time() - st["t0"]
    entry = f"[{elapsed:.2f}] {msg}"
    res["events"].append(entry)
    print(f"HUD_PIE: {msg}")


def safe(fn, *args, **kwargs):
    try:
        return fn(*args, **kwargs)
    except Exception as e:
        return f"ERR: {repr(e)[:160]}"


def dump():
    try:
        with open(OUT, "w") as f:
            json.dump(res, f, indent=2, default=str)
    except Exception as e:
        print("dump err:", e)


def passf(name, ok, detail):
    res["tests"][name] = {"pass": bool(ok), "detail": str(detail)[:280]}
    log(f"{name} => {'PASS' if ok else 'FAIL'} ({str(detail)[:130]})")
    dump()


def find_setup(world):
    pc = safe(unreal.GameplayStatics.get_player_controller, world, 0)
    if pc is None or isinstance(pc, str):
        return False
    pawn = safe(pc.get_controlled_pawn)
    if pawn is None or isinstance(pawn, str):
        return False
    holder = None
    for c in (safe(pawn.get_components_by_class, unreal.ChildActorComponent) or []):
        if not isinstance(c, str) and str(safe(c.get_name)) == "FP_WeaponHolder":
            holder = c
            break
    if holder is None or isinstance(holder, str):
        return False
    wpn = safe(holder.get_editor_property, "child_actor")
    if wpn is None or isinstance(wpn, str):
        return False
    comp = safe(pawn.get_editor_property, "BPC_Health")
    hud = safe(pawn.get_editor_property, "AmmoHUD")

    if comp is None or isinstance(comp, str):
        return False

    st["world"] = world
    st["pc"] = pc
    st["pawn"] = pawn
    st["holder"] = holder
    st["weapon"] = wpn
    st["comp"] = comp
    st["hud"] = hud
    return True


def get_hud_text():
    if st["hud"] and not isinstance(st["hud"], str):
        tb = safe(st["hud"].get_editor_property, "Team1Score")
        if tb and not isinstance(tb, str):
            txt = safe(tb.get_text)
            return str(txt)
    return "UNKNOWN"


def ammo():
    return safe(st["weapon"].get_editor_property, "CurrentAmmo")


def reserve():
    return safe(st["weapon"].get_editor_property, "ReserveAmmo")


def take_shot(name, res_x=1280, res_y=720):
    try:
        unreal.AutomationLibrary.take_high_res_screenshot(res_x, res_y, name)
        res["shots"].append(name)
        st["pending"] = name
        st["wait"] = 6
        log(f"Requested screenshot: {name} ({res_x}x{res_y})")
    except Exception as e:
        res["shots"].append(f"{name} ERR: {repr(e)[:80]}")


def tick(dummy):
    if st.get("busy"):
        return
    st["busy"] = True
    try:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        world = safe(ues.get_game_world) if les.is_in_play_in_editor() else None
        now = time.time()

        if st["pending"]:
            st["wait"] -= 1
            if st["wait"] <= 0:
                st["pending"] = None
            return

        # ---------------- Phase: Boot ----------------
        if st["phase"] == "boot":
            log("Requesting PIE session...")
            safe(les.editor_request_begin_play)
            st["phase"] = "wait_pie"
            st["t0"] = now

        # ---------------- Phase: Wait PIE ----------------
        elif st["phase"] == "wait_pie":
            if world is not None and not isinstance(world, str):
                log("PIE world active, finding player setup...")
                st["phase"] = "find"
                st["t0"] = now
            elif now - st["t0"] > 35:
                passf("pie_start", False, "timeout waiting for PIE world")
                st["phase"] = "done"

        # ---------------- Phase: Find Setup ----------------
        elif st["phase"] == "find":
            if find_setup(world):
                txt = get_hud_text()
                a = str(ammo())
                r = str(reserve())
                log(f"Setup found: Ammo={a}/{r}, HUD Text='{txt}'")
                st["phase"] = "settle"
                st["t0"] = now
            elif now - st["t0"] > 40:
                passf("find_setup", False, "timeout finding player setup")
                st["phase"] = "done"

        # ---------------- Phase: Settle & Initial Spawn Check ----------------
        elif st["phase"] == "settle":
            if now - st["t0"] > 1.5:
                txt = get_hud_text()
                a = str(ammo())
                r = str(reserve())
                passf("test1_spawn_ammo", a in ("30", "30.0") and r in ("90", "90.0"), f"ammo={a}/{r}")
                passf("test1_spawn_hud_text", "30 / 90" in txt or "30/90" in txt, f"HUD='{txt}'")
                take_shot("HUD_01_Spawn_30_90", 1280, 720)
                st["phase"] = "shot1"
                st["t0"] = now

        # ---------------- Phase: First Shot ----------------
        elif st["phase"] == "shot1":
            if now - st["t0"] > 0.5:
                log("Executing 1st shot via BP_Fire...")
                safe(st["pawn"].call_method, "BP_Fire")
                st["phase"] = "shot1_verify"
                st["t0"] = now

        elif st["phase"] == "shot1_verify":
            if now - st["t0"] > 0.5:
                txt = get_hud_text()
                a = str(ammo())
                passf("test2_shot1_ammo", a in ("29", "29.0"), f"ammo={a}")
                passf("test2_shot1_hud_text", "29 / 90" in txt or "29/90" in txt, f"HUD='{txt}'")
                take_shot("HUD_02_Shot1_29_90", 1280, 720)
                st["phase"] = "shot2"
                st["t0"] = now

        # ---------------- Phase: Second Shot ----------------
        elif st["phase"] == "shot2":
            if now - st["t0"] > 0.5:
                log("Executing 2nd shot via BP_Fire...")
                safe(st["pawn"].call_method, "BP_Fire")
                st["phase"] = "shot2_verify"
                st["t0"] = now

        elif st["phase"] == "shot2_verify":
            if now - st["t0"] > 0.5:
                txt = get_hud_text()
                a = str(ammo())
                passf("test3_shot2_ammo", a in ("28", "28.0"), f"ammo={a}")
                passf("test3_shot2_hud_text", "28 / 90" in txt or "28/90" in txt, f"HUD='{txt}'")
                take_shot("HUD_03_Shot2_28_90", 1280, 720)
                st["phase"] = "empty_prep"
                st["t0"] = now

        # ---------------- Phase: Empty Magazine ----------------
        elif st["phase"] == "empty_prep":
            if now - st["t0"] > 0.5:
                log("Setting CurrentAmmo to 1 to test empty transition...")
                safe(st["weapon"].set_editor_property, "CurrentAmmo", 1)
                safe(st["pawn"].call_method, "BP_Fire")
                st["phase"] = "empty_verify"
                st["t0"] = now

        elif st["phase"] == "empty_verify":
            if now - st["t0"] > 0.5:
                txt = get_hud_text()
                a = str(ammo())
                passf("test4_empty_ammo_zero", a in ("0", "0.0"), f"ammo={a}")
                passf("test4_empty_hud_text", "EMPTY MAGAZINE" in txt, f"HUD='{txt}'")
                take_shot("HUD_04_Empty_Magazine", 1280, 720)

                # Attempt fire on empty
                safe(st["pawn"].call_method, "BP_Fire")
                st["phase"] = "empty_block_verify"
                st["t0"] = now

        elif st["phase"] == "empty_block_verify":
            if now - st["t0"] > 0.4:
                txt = get_hud_text()
                a = str(ammo())
                passf("test4_empty_fire_blocked", a in ("0", "0.0") and "EMPTY MAGAZINE" in txt, f"ammo={a}, HUD='{txt}'")
                st["phase"] = "dead_prep"
                st["t0"] = now

        # ---------------- Phase: Death Gating ----------------
        elif st["phase"] == "dead_prep":
            if now - st["t0"] > 0.5:
                log("Resetting ammo to 30 and applying lethal damage (100 HP)...")
                safe(st["weapon"].set_editor_property, "CurrentAmmo", 30)
                if st["hud"]:
                    safe(st["hud"].call_method, "UpdateAmmo", (30, 90))
                safe(st["comp"].call_method, "ApplyDamage", (100.0,))
                st["phase"] = "dead_verify"
                st["t0"] = now

        elif st["phase"] == "dead_verify":
            if now - st["t0"] > 0.6:
                dead_comp = safe(st["comp"].get_editor_property, "bIsDead")
                dead_pawn = safe(st["pawn"].get_editor_property, "bIsDead")
                log(f"Death state: comp.bIsDead={dead_comp}, pawn.bIsDead={dead_pawn}")
                passf("test5_player_dead", str(dead_comp) == "True" or str(dead_pawn) == "True", f"comp={dead_comp}, pawn={dead_pawn}")

                # Attempt to fire while dead
                a_before = ammo()
                safe(st["pawn"].call_method, "BP_Fire")
                a_after = ammo()
                passf("test5_dead_cannot_fire", str(a_after) == str(a_before) and str(a_after) in ("30", "30.0"), f"ammo {a_before}->{a_after}")
                take_shot("HUD_05_Dead_Cannot_Fire", 1280, 720)
                st["phase"] = "restart_prep"
                st["t0"] = now

        # ---------------- Phase: Restart Level ----------------
        elif st["phase"] == "restart_prep":
            if now - st["t0"] > 0.5:
                log("Executing RestartLevel console command...")
                safe(unreal.SystemLibrary.execute_console_command, world, "RestartLevel", st["pc"])
                st["phase"] = "restart_wait"
                st["t0"] = now

        elif st["phase"] == "restart_wait":
            if now - st["t0"] > 4.5:
                if find_setup(world):
                    hp = str(safe(st["comp"].get_editor_property, "CurrentHealth"))
                    a = str(ammo())
                    r = str(reserve())
                    txt = get_hud_text()
                    log(f"Post-restart setup: HP={hp}, Ammo={a}/{r}, HUD='{txt}'")
                    passf("test6_restart_health_ammo", hp in ("100", "100.0") and a in ("30", "30.0") and r in ("90", "90.0"), f"HP={hp}, Ammo={a}/{r}")
                    passf("test6_restart_hud_text", "30 / 90" in txt or "30/90" in txt, f"HUD='{txt}'")
                    take_shot("HUD_06_Restart_Clean", 1280, 720)
                    st["phase"] = "hires_1080p"
                    st["t0"] = now
                else:
                    passf("test6_restart_setup", False, "failed to re-find setup after restart")
                    st["phase"] = "wrapup"
                    st["t0"] = now

        # ---------------- Phase: 1080p Resolution Screenshot ----------------
        elif st["phase"] == "hires_1080p":
            if now - st["t0"] > 1.0:
                log("Taking 1920x1080 layout verification screenshot...")
                take_shot("HUD_07_1080p_Layout", 1920, 1080)
                st["phase"] = "wrapup"
                st["t0"] = now

        # ---------------- Phase: Wrap-up & Shutdown ----------------
        elif st["phase"] == "wrapup":
            if now - st["t0"] > 2.5:
                log("Ending PIE session...")
                safe(les.editor_request_end_play)
                st["phase"] = "done"
                st["t0"] = now

        elif st["phase"] == "done":
            if not res.get("_finished"):
                res["_finished"] = True
                res["finished_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
                log("All HUD PIE tests complete!")
                dump()
                unreal.EditorPythonScripting.set_keep_python_script_alive(False)
                safe(unreal.SystemLibrary.execute_console_command, None, "quit")

        if int(now) % 5 == 0:
            dump()

    except Exception:
        res["fatal_error"] = traceback.format_exc()
        log(f"FATAL: {res['fatal_error']}")
        dump()
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    finally:
        st["busy"] = False


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
log("HUD PIE verification harness registered")
dump()
unreal.register_slate_post_tick_callback(tick)
print("HUD_PIE_HARNESS_REGISTERED")
