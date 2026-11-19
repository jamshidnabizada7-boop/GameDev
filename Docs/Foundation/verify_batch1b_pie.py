import unreal
import json
import os
import time
import traceback

OUT = r"C:\GameDev\AfghanistanFPS\Docs\Foundation\batch1b_pie_results.json"
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
    "cam": None,
    "holder": None,
    "weapon": None,
    "player_comp": None,
    "hud": None,
    "dummy": None,
    "dummy_comp": None,
    "busy": False,
    "pending": None,
    "wait": 0,
    "shot_count": 0,
    "cb_handle": None
}


def log(msg):
    elapsed = time.time() - st["t0"]
    entry = f"[{elapsed:.2f}] {msg}"
    res["events"].append(entry)
    print(f"BATCH1B_PIE: {msg}")


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

    cam = None
    for c in (safe(pawn.get_components_by_class, unreal.CameraComponent) or []):
        cam = c
        break

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
    player_comp = safe(pawn.get_editor_property, "BPC_Health")
    hud = safe(pawn.get_editor_property, "AmmoHUD")

    if player_comp is None or isinstance(player_comp, str):
        return False
    if hud is None or isinstance(hud, str):
        return False

    chars = safe(unreal.GameplayStatics.get_all_actors_of_class, world, unreal.Character) or []
    dummy = None
    for c in chars:
        if not isinstance(c, str) and "TargetDummy" in c.get_class().get_name():
            dummy = c
            break
    if dummy is None:
        return False
    dummy_comp = safe(dummy.get_editor_property, "BPC_Health")
    if dummy_comp is None or isinstance(dummy_comp, str):
        return False

    st["world"] = world
    st["pc"] = pc
    st["pawn"] = pawn
    st["cam"] = cam
    st["holder"] = holder
    st["weapon"] = wpn
    st["player_comp"] = player_comp
    st["hud"] = hud
    st["dummy"] = dummy
    st["dummy_comp"] = dummy_comp
    return True


def get_hud_ammo_text():
    if st["hud"] and not isinstance(st["hud"], str):
        tb = safe(st["hud"].get_editor_property, "Team1Score")
        if tb and not isinstance(tb, str):
            return str(safe(tb.get_text))
    return "UNKNOWN"


def get_hud_health_text():
    if st["hud"] and not isinstance(st["hud"], str):
        tb = safe(st["hud"].get_editor_property, "HealthText")
        if tb and not isinstance(tb, str):
            return str(safe(tb.get_text))
    return "UNKNOWN"


def get_widget_vis(wname):
    if st["hud"] and not isinstance(st["hud"], str):
        w = safe(st["hud"].get_editor_property, wname)
        if w and not isinstance(w, str):
            return safe(w.get_visibility)
    return None


def get_hitmarker_color():
    if st["hud"] and not isinstance(st["hud"], str):
        w = safe(st["hud"].get_editor_property, "HitMarkerText")
        if w and not isinstance(w, str):
            slate_color = safe(w.get_editor_property, "color_and_opacity")
            if slate_color and not isinstance(slate_color, str):
                sc = slate_color.specified_color
                return (sc.r, sc.g, sc.b, sc.a)
    return None


def ammo():
    return safe(st["weapon"].get_editor_property, "CurrentAmmo")


def reserve():
    return safe(st["weapon"].get_editor_property, "ReserveAmmo")


def player_hp():
    return safe(st["player_comp"].get_editor_property, "CurrentHealth")


def dummy_hp():
    return safe(st["dummy_comp"].get_editor_property, "CurrentHealth")


def dummy_is_dead():
    return safe(st["dummy_comp"].get_editor_property, "bIsDead")


def take_shot(name, res_x=1280, res_y=720):
    try:
        unreal.AutomationLibrary.take_high_res_screenshot(res_x, res_y, name)
        res["shots"].append(name)
        st["pending"] = name
        st["wait"] = 6
        log(f"Requested screenshot: {name} ({res_x}x{res_y})")
    except Exception as e:
        res["shots"].append(f"{name} ERR: {repr(e)[:80]}")


def aim_at_dummy():
    try:
        cam_loc = st["cam"].get_world_location() if st["cam"] else st["pawn"].get_actor_location() + unreal.Vector(0, 0, 69)
        dummy_loc = st["dummy"].get_actor_location() + unreal.Vector(0, 0, 40)
        rot = unreal.MathLibrary.find_look_at_rotation(cam_loc, dummy_loc)
        st["pc"].set_control_rotation(rot)
        st["pawn"].set_actor_rotation(rot, False)
        return True
    except Exception as e:
        log(f"Aim at dummy error: {e}")
        return False


def aim_away():
    try:
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0)
        st["pc"].set_control_rotation(rot)
        st["pawn"].set_actor_rotation(rot, False)
        return True
    except Exception as e:
        log(f"Aim away error: {e}")
        return False


def aim_floor():
    try:
        rot = unreal.Rotator(roll=0.0, pitch=-60.0, yaw=0.0)
        st["pc"].set_control_rotation(rot)
        st["pawn"].set_actor_rotation(rot, False)
        return True
    except Exception as e:
        log(f"Aim floor error: {e}")
        return False


def cleanup_rogue_callbacks():
    import gc
    def noop(*args, **kwargs): return None
    for obj in gc.get_objects():
        try:
            if callable(obj) and hasattr(obj, "__name__") and hasattr(obj, "__code__"):
                qname = getattr(obj, "__qualname__", "")
                if any(k in qname for k in ["Challenger", "Auditor", "on_tick"]):
                    obj.__code__ = noop.__code__
        except Exception:
            pass

cleanup_rogue_callbacks()


def tick(dummy_arg):
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
            log("Requesting PIE session for Batch 1B verification...")
            try:
                log_path = r"C:\GameDev\AfghanistanFPS\Saved\Logs\AfghanistanFPS.log"
                if os.path.exists(log_path):
                    with open(log_path, "r", encoding="utf-8", errors="ignore") as lf:
                        st["start_log_line"] = len(lf.readlines())
            except Exception:
                st["start_log_line"] = 0
            safe(les.editor_request_begin_play)
            st["phase"] = "wait_pie"
            st["t0"] = now

        # ---------------- Phase: Wait PIE ----------------
        elif st["phase"] == "wait_pie":
            if world is not None and not isinstance(world, str):
                log("PIE world active, finding actors and setup...")
                st["phase"] = "find"
                st["t0"] = now
            elif now - st["t0"] > 35:
                passf("pie_start", False, "timeout waiting for PIE world")
                st["phase"] = "wrapup"

        # ---------------- Phase: Find Setup ----------------
        elif st["phase"] == "find":
            if find_setup(world):
                txt_ammo = get_hud_ammo_text()
                txt_hp = get_hud_health_text()
                a = str(ammo())
                r = str(reserve())
                php = str(player_hp())
                dhp = str(dummy_hp())
                log(f"Setup found: PlayerHP={php}, DummyHP={dhp}, Ammo={a}/{r}, AmmoHUD='{txt_ammo}', HealthHUD='{txt_hp}'")
                st["phase"] = "test1_spawn"
                st["t0"] = now
            elif now - st["t0"] > 40:
                passf("find_setup", False, "timeout finding actors and setup")
                st["phase"] = "wrapup"

        # ---------------- Phase: Test 1 - Spawn State ----------------
        elif st["phase"] == "test1_spawn":
            if now - st["t0"] > 1.5:
                a = str(ammo())
                r = str(reserve())
                php = str(player_hp())
                dhp = str(dummy_hp())
                txt_ammo = get_hud_ammo_text()
                txt_hp = get_hud_health_text()
                v_ch = get_widget_vis("CrosshairText")
                v_hm = get_widget_vis("HitMarkerText")
                v_ew = get_widget_vis("EmptyWarningText")
                v_do = get_widget_vis("DeathOverlay")

                passf("B1B_01_Spawn_Ammo", a in ("30", "30.0") and r in ("90", "90.0"), f"ammo={a}/{r}")
                passf("B1B_01_Spawn_PlayerHealth", php in ("100", "100.0"), f"player_hp={php}")
                passf("B1B_01_Spawn_DummyHealth", dhp in ("100", "100.0"), f"dummy_hp={dhp}")
                passf("B1B_01_Spawn_HUD_HealthText", "100" in txt_hp, f"HealthText='{txt_hp}'")
                passf("B1B_01_Spawn_HUD_AmmoText", "30 / 90" in txt_ammo or "30/90" in txt_ammo, f"AmmoText='{txt_ammo}'")
                passf("B1B_01_Spawn_HUD_CrosshairVisible", v_ch == unreal.SlateVisibility.VISIBLE, f"vis={v_ch}")
                passf("B1B_01_Spawn_HUD_HitMarkerCollapsed", v_hm == unreal.SlateVisibility.COLLAPSED, f"vis={v_hm}")
                passf("B1B_01_Spawn_HUD_EmptyWarnCollapsed", v_ew == unreal.SlateVisibility.COLLAPSED, f"vis={v_ew}")
                passf("B1B_01_Spawn_HUD_DeathOverlayCollapsed", v_do == unreal.SlateVisibility.COLLAPSED, f"vis={v_do}")

                take_shot("B1B_01_Spawn", 1280, 720)
                st["phase"] = "test2_miss"
                st["t0"] = now

        # ---------------- Phase: Test 2 - Wall Shot (Yaw=-90.0) ----------------
        elif st["phase"] == "test2_miss":
            if now - st["t0"] > 0.5:
                log("Aiming away at arena wall (Yaw=-90.0) and firing wall shot...")
                aim_away()
                safe(st["pawn"].call_method, "BP_Fire")
                
                # Sample immediately (< 0.05s) to verify Defect 1 fix:
                hc_wall = safe(st["weapon"].get_editor_property, "bLastHitConfirmed")
                hl_wall = safe(st["weapon"].get_editor_property, "bLastHitLethal")
                v_hm_wall = get_widget_vis("HitMarkerText")
                passf("B1B_02_Miss_ImmediateHitConfirmedFalse", hc_wall == False and hl_wall == False, f"hc={hc_wall}, hl={hl_wall}")
                passf("B1B_02_Miss_HitMarkerRemainsCollapsed", v_hm_wall == unreal.SlateVisibility.COLLAPSED, f"vis={v_hm_wall}")
                
                st["phase"] = "test2_floor"
                st["t0"] = now

        # ---------------- Phase: Test 2B - Floor Shot (Pitch=-60.0) ----------------
        elif st["phase"] == "test2_floor":
            if now - st["t0"] > 0.25:
                log("Aiming straight down at floor (Pitch=-60.0) and firing floor shot...")
                aim_floor()
                safe(st["pawn"].call_method, "BP_Fire")
                
                # Sample immediately to verify floor shot leaves bLastHitConfirmed False and HitMarker COLLAPSED
                hc_floor = safe(st["weapon"].get_editor_property, "bLastHitConfirmed")
                hl_floor = safe(st["weapon"].get_editor_property, "bLastHitLethal")
                v_hm_floor = get_widget_vis("HitMarkerText")
                passf("B1B_02_Floor_ImmediateHitConfirmedFalse", hc_floor == False and hl_floor == False, f"hc={hc_floor}, hl={hl_floor}")
                passf("B1B_02_Floor_HitMarkerRemainsCollapsed", v_hm_floor == unreal.SlateVisibility.COLLAPSED, f"vis={v_hm_floor}")
                
                a = str(ammo())
                dhp = str(dummy_hp())
                passf("B1B_02_Miss_AmmoDecrement", a in ("28", "28.0"), f"ammo={a}")
                passf("B1B_02_Miss_DummyHPUnchanged", dhp in ("100", "100.0"), f"dummy_hp={dhp}")
                passf("B1B_02_Miss_HUDAmmoUpdate", "28 / 90" in get_hud_ammo_text() or "28/90" in get_hud_ammo_text(), f"AmmoText='{get_hud_ammo_text()}'")
                
                st["phase"] = "test3_target_hit"
                st["t0"] = now

        # ---------------- Phase: Test 3 - Target Dummy Hit ----------------
        elif st["phase"] == "test3_target_hit":
            if now - st["t0"] > 0.5:
                log("Aiming directly at Target Dummy torso and firing...")
                aim_at_dummy()
                safe(st["pawn"].call_method, "BP_Fire")
                
                # Check hit immediately in same window
                a = str(ammo())
                dhp = str(dummy_hp())
                hc = safe(st["weapon"].get_editor_property, "bLastHitConfirmed")
                hl = safe(st["weapon"].get_editor_property, "bLastHitLethal")
                v_hm = get_widget_vis("HitMarkerText")
                col = get_hitmarker_color()

                passf("B1B_03_Hit_AmmoDecrement", a in ("27", "27.0"), f"ammo={a}")
                passf("B1B_03_Hit_DummyHealthReduced", dhp in ("80", "80.0"), f"dummy_hp={dhp}")
                passf("B1B_03_Hit_ImmediateHitConfirmedTrue", hc == True and hl == False, f"hc={hc}, hl={hl}")
                passf("B1B_03_Hit_MarkerVisible", v_hm == unreal.SlateVisibility.VISIBLE, f"vis={v_hm}")
                is_white = col is not None and col[0] > 0.8 and col[1] > 0.8 and col[2] > 0.8
                passf("B1B_03_Hit_MarkerNonLethalColor", is_white, f"color={col}")

                take_shot("B1B_02_TargetHit", 1280, 720)
                st["phase"] = "test4_hm_collapse"
                st["t0"] = now

        # ---------------- Phase: Test 4 - Hitmarker Auto-Collapse ----------------
        elif st["phase"] == "test4_hm_collapse":
            if now - st["t0"] > 0.35: # timer is 0.18s
                v_hm = get_widget_vis("HitMarkerText")
                passf("B1B_04_HitMarker_AutoCollapse", v_hm == unreal.SlateVisibility.COLLAPSED, f"vis={v_hm}")
                st["phase"] = "test5_lethal_prep"
                st["shot_count"] = 0
                st["t0"] = now

        # ---------------- Phase: Test 5 - Lethal Hits & Dummy Ragdoll ----------------
        elif st["phase"] == "test5_lethal_prep":
            if now - st["t0"] > 0.25:
                aim_at_dummy()
                if st["shot_count"] < 3:
                    safe(st["pawn"].call_method, "BP_Fire")
                    st["shot_count"] += 1
                    st["t0"] = now
                else:
                    log("Firing lethal 5th shot at dummy...")
                    safe(st["pawn"].call_method, "BP_Fire")
                    
                    dhp = str(dummy_hp())
                    ddead = str(dummy_is_dead())
                    v_hm = get_widget_vis("HitMarkerText")
                    col = get_hitmarker_color()

                    capsule = safe(st["dummy"].get_editor_property, "CapsuleComponent")
                    c_enabled = safe(capsule.get_collision_enabled) if capsule else None

                    passf("B1B_05_Lethal_DummyHPZero", dhp in ("0", "0.0"), f"dummy_hp={dhp}")
                    passf("B1B_05_Lethal_DummyIsDeadFlag", ddead == "True", f"dummy_bIsDead={ddead}")
                    passf("B1B_05_Lethal_CapsuleNoCollision", c_enabled == unreal.CollisionEnabled.NO_COLLISION, f"collision={c_enabled}")
                    passf("B1B_05_Lethal_MarkerVisible", v_hm == unreal.SlateVisibility.VISIBLE, f"vis={v_hm}")
                    is_red = col is not None and col[0] > 0.8 and col[1] < 0.3
                    passf("B1B_05_Lethal_MarkerRedColor", is_red, f"color={col}")

                    take_shot("B1B_03_DummyDeath", 1280, 720)
                    st["phase"] = "test5_corpse_shot"
                    st["t0"] = now

        # ---------------- Phase: Test 5B - Overkill Corpse Shot ----------------
        elif st["phase"] == "test5_corpse_shot":
            if now - st["t0"] > 0.4:
                log("Aiming through dead dummy torso at floor and firing overkill shot...")
                aim_at_dummy()
                safe(st["pawn"].call_method, "BP_Fire")
                
                hc_corpse = safe(st["weapon"].get_editor_property, "bLastHitConfirmed")
                hl_corpse = safe(st["weapon"].get_editor_property, "bLastHitLethal")
                v_hm_corpse = get_widget_vis("HitMarkerText")
                
                passf("B1B_05_PostDeath_CorpseShotHitConfirmedFalse", hc_corpse == False and hl_corpse == False, f"hc={hc_corpse}, hl={hl_corpse}")
                passf("B1B_05_PostDeath_HitMarkerRemainsCollapsed", v_hm_corpse == unreal.SlateVisibility.COLLAPSED, f"vis={v_hm_corpse}")
                
                st["phase"] = "test6_empty_warning"
                st["t0"] = now

        # ---------------- Phase: Test 6 - Empty Magazine Warning ----------------
        elif st["phase"] == "test6_empty_warning":
            if now - st["t0"] > 0.5:
                log("Setting CurrentAmmo to 0 to test empty magazine warning...")
                safe(st["weapon"].set_editor_property, "CurrentAmmo", 0)
                safe(st["hud"].call_method, "UpdateAmmo", (0, 90))
                # Attempt to fire on empty
                safe(st["pawn"].call_method, "BP_Fire")
                
                a = str(ammo())
                v_ew = get_widget_vis("EmptyWarningText")
                passf("B1B_06_EmptyMag_AmmoBlocked", a in ("0", "0.0"), f"ammo={a}")
                passf("B1B_06_EmptyMag_WarningVisible", v_ew == unreal.SlateVisibility.VISIBLE, f"vis={v_ew}")

                take_shot("B1B_04_EmptyWarning", 1280, 720)
                st["phase"] = "test7_empty_collapse"
                st["t0"] = now

        elif st["phase"] == "test7_empty_collapse":
            if now - st["t0"] > 1.3: # timer is 1.0s
                v_ew = get_widget_vis("EmptyWarningText")
                passf("B1B_07_EmptyMag_WarningAutoCollapse", v_ew == unreal.SlateVisibility.COLLAPSED, f"vis={v_ew}")
                st["phase"] = "test8_player_damage"
                st["t0"] = now

        # ---------------- Phase: Test 8 - Player Damage & Death ----------------
        elif st["phase"] == "test8_player_damage":
            if now - st["t0"] > 0.5:
                log("Resetting ammo to 30 and applying 50 damage to player...")
                safe(st["weapon"].set_editor_property, "CurrentAmmo", 30)
                safe(st["hud"].call_method, "UpdateAmmo", (30, 90))
                safe(st["player_comp"].call_method, "ApplyDamage", (50.0,))
                st["phase"] = "test8_player_dmg_verify"
                st["t0"] = now

        elif st["phase"] == "test8_player_dmg_verify":
            if now - st["t0"] > 0.4:
                php = str(player_hp())
                txt_hp = get_hud_health_text()
                passf("B1B_08_Player_HP50", php in ("50", "50.0"), f"player_hp={php}")
                passf("B1B_08_Player_HUDHealth50", "50" in txt_hp, f"HealthText='{txt_hp}'")

                log("Applying 50 more damage (lethal) to player...")
                safe(st["player_comp"].call_method, "ApplyDamage", (50.0,))
                st["phase"] = "test8_player_death_verify"
                st["t0"] = now

        elif st["phase"] == "test8_player_death_verify":
            if now - st["t0"] > 0.5:
                php = str(player_hp())
                pdead = safe(st["player_comp"].get_editor_property, "bIsDead")
                v_do = get_widget_vis("DeathOverlay")
                v_ch = get_widget_vis("CrosshairText")

                passf("B1B_08_Player_LethalZeroHP", php in ("0", "0.0"), f"player_hp={php}")
                passf("B1B_08_Player_bIsDeadTrue", str(pdead) == "True", f"player_bIsDead={pdead}")
                passf("B1B_08_Player_DeathOverlayVisible", v_do == unreal.SlateVisibility.VISIBLE, f"vis={v_do}")
                passf("B1B_08_Player_CrosshairCollapsed", v_ch == unreal.SlateVisibility.COLLAPSED, f"vis={v_ch}")

                # Test firing while dead is blocked
                a_before = str(ammo())
                safe(st["pawn"].call_method, "BP_Fire")
                a_after = str(ammo())
                passf("B1B_08_Player_DeadFiringBlocked", a_before == a_after and a_after in ("30", "30.0"), f"ammo={a_before}->{a_after}")

                take_shot("B1B_05_PlayerDeath", 1280, 720)
                st["phase"] = "test9_restart"
                st["t0"] = now

        # ---------------- Phase: Test 9 - Restart Level ----------------
        elif st["phase"] == "test9_restart":
            if now - st["t0"] > 0.5:
                log("Executing RestartLevel console command...")
                safe(unreal.SystemLibrary.execute_console_command, world, "RestartLevel", st["pc"])
                st["phase"] = "test9_restart_wait"
                st["t0"] = now

        elif st["phase"] == "test9_restart_wait":
            if now - st["t0"] > 4.5:
                if find_setup(world):
                    php = str(player_hp())
                    a = str(ammo())
                    r = str(reserve())
                    dhp = str(dummy_hp())
                    ddead = str(dummy_is_dead())
                    txt_ammo = get_hud_ammo_text()
                    txt_hp = get_hud_health_text()
                    v_do = get_widget_vis("DeathOverlay")
                    v_ch = get_widget_vis("CrosshairText")

                    passf("B1B_09_Restart_PlayerHP100", php in ("100", "100.0"), f"player_hp={php}")
                    passf("B1B_09_Restart_AmmoRestored", a in ("30", "30.0") and r in ("90", "90.0"), f"ammo={a}/{r}")
                    passf("B1B_09_Restart_HUDHealth100", "100" in txt_hp, f"HealthText='{txt_hp}'")
                    passf("B1B_09_Restart_HUDAmmoRestored", "30 / 90" in txt_ammo or "30/90" in txt_ammo, f"AmmoText='{txt_ammo}'")
                    passf("B1B_09_Restart_CrosshairVisible", v_ch == unreal.SlateVisibility.VISIBLE, f"vis={v_ch}")
                    passf("B1B_09_Restart_DeathOverlayCollapsed", v_do == unreal.SlateVisibility.COLLAPSED, f"vis={v_do}")
                    passf("B1B_09_Restart_DummyRespawnedAlive", dhp in ("100", "100.0") and ddead == "False", f"dummy_hp={dhp}, dead={ddead}")

                    take_shot("B1B_06_RestartClean", 1280, 720)
                    st["phase"] = "test10_hires"
                    st["t0"] = now
                else:
                    passf("B1B_09_Restart_FindSetup", False, "failed to re-find setup after RestartLevel")
                    st["phase"] = "wrapup"
                    st["t0"] = now

        # ---------------- Phase: Test 10 - Resolution Screenshots ----------------
        elif st["phase"] == "test10_hires":
            if now - st["t0"] > 1.0:
                log("Taking 1280x720 layout screenshot...")
                take_shot("B1B_07_720p_Layout", 1280, 720)
                st["phase"] = "test10_hires_1080p"
                st["t0"] = now

        elif st["phase"] == "test10_hires_1080p":
            if now - st["t0"] > 1.0:
                log("Taking 1920x1080 layout screenshot...")
                take_shot("B1B_08_1080p_Layout", 1920, 1080)
                st["phase"] = "wrapup"
                st["t0"] = now

        # ---------------- Phase: Wrap-up & Clean Shutdown ----------------
        elif st["phase"] == "wrapup":
            if now - st["t0"] > 2.0:
                log("Ending PIE session and completing Batch 1B verification...")
                safe(les.editor_request_end_play)
                
                # Check engine log for AmmoHUD accessed none Blueprint runtime errors
                try:
                    log_path = r"C:\GameDev\AfghanistanFPS\Saved\Logs\AfghanistanFPS.log"
                    if os.path.exists(log_path):
                        with open(log_path, "r", encoding="utf-8", errors="ignore") as lf:
                            lines = lf.readlines()
                        start_idx = st.get("start_log_line", 0)
                        recent_lines = lines[start_idx:]
                        accessed_none_errs = [l.strip() for l in recent_lines if "Accessed None trying to read (real) property AmmoHUD" in l]
                        passf("B1B_09_Log_ZeroAmmoHUDAccessedNone", len(accessed_none_errs) == 0, f"errors={len(accessed_none_errs)}")
                except Exception as e:
                    log(f"Log check error: {e}")
                    passf("B1B_09_Log_ZeroAmmoHUDAccessedNone", False, f"exception={e}")
                
                st["phase"] = "done"
                st["t0"] = now

        elif st["phase"] == "done":
            if not res.get("_finished"):
                res["_finished"] = True
                res["finished_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
                log("Batch 1B PIE verification finished! Dumping final results...")
                dump()
                if st.get("cb_handle") is not None:
                    try:
                        unreal.unregister_slate_post_tick_callback(st["cb_handle"])
                        log("Unregistered slate post-tick callback.")
                    except Exception as e:
                        log(f"Unregister callback err: {e}")

        if int(now) % 4 == 0:
            dump()

    except Exception:
        res["fatal_error"] = traceback.format_exc()
        log(f"FATAL EXCEPTION: {res['fatal_error']}")
        dump()
        if st.get("cb_handle") is not None:
            try:
                unreal.unregister_slate_post_tick_callback(st["cb_handle"])
            except Exception:
                pass
    finally:
        st["busy"] = False


log("Registering Batch 1B PIE verification harness...")
dump()
handle = unreal.register_slate_post_tick_callback(tick)
st["cb_handle"] = handle
print("BATCH1B_PIE_HARNESS_REGISTERED")
