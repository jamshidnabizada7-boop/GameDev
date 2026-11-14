# PHASE 1 AGENT HANDOVER — Afghanistan FPS
Updated: 2026-11-14 · Agent: ZCode (GLM) continuation session
If this session hits a usage limit, READ THIS FIRST, then Phase1_Test_Results.md.

## Where things stand
The Phase 1 player foundation is **implemented and PIE-verified, including the
interaction system AND the input-mapping persistence fix** (2026-11-14 final
autonomous session). Git HEAD: 9f2d97e. Remaining: a short **manual acceptance pass**
with the on-screen debug display (see Phase1_Next_Steps.md) — everything
machine-verifiable has been verified.

**CRITICAL FIX (2026-11-14, commit 9fa95ae)**: the 4 Phase 1 key mappings
(LeftShift/LeftCtrl/E/R) were never persisted to IMC_Default — `imc.map_key()` +
`save_asset()` silently fails to serialize them (three sessions hit this). Fixed via
`imc.set_editor_property("mappings", full_16_entry_list)` — verified persisting in a
fresh editor session (zcode_final_verify.json: 16 mappings on disk). **A debug
display (10 on-screen prints) was added to BP_PlayerFoundation** — pressing
W/Space/Shift/Ctrl/E/R during PIE now prints "INPUT: <event> fired" on screen.

Verified in real PIE (evidence: `C:\Users\PC\Documents\Default Project\zcode_phase1\*.json`):
- BP_PlayerFoundation_C spawns at PlayerStart via BP_GM_FoundationTest override.
- BPC_Health initializes at 100; damage volume applies 25 per entry.
- Full sequence verified: 100 → 75 → 50 → 25 → 0 → bIsDead=True →
  RestartLevel → HP 100, alive.
- Crouch: bIsCrouched True, capsule 96→40, crouch-move ~190 cm/s; uncrouch restores 96.
- Jump: +89.6 cm rise, clean landing.
- Collision: pawn stopped by interact cube and walls.
- IMC_Default: **16 mappings on disk, verified in a fresh editor session**
  (zcode_final_verify.json).
- Interaction: BPI_Interactable assigned (runtime does_implement=True), Interact
  event + scale-toggle (1.0 → 1.25 → 1.0 per user spec) implemented/compiled/saved;
  verified in PIE via interface call (zcode_tog_test.json:
  visible_state_toggle_works=true). Player trace chain verified (camera trace hits
  the cube, blocking_hit=True at 116 cm). All 8 IA event nodes verified with correct
  InputAction references (zcode_ia_pins2.json).
- Walk/sprint measured on clean map: 415 / 615 cm/s avg over 1.5 s (accel ramp;
  MaxWalkSpeed 450/700 enforced + restored to 450 after test).
- SkyLight recaptured in editor world; map saved (zcode_sky3.json).
- _Scratch probes (7 assets) deleted via EditorAssetLibrary (deferred unlink on
  editor close).

## Key facts that will save you hours
- Editor Python entry points:
  - Launch with script: `UnrealEditor.exe <uproject> -windowed -resx=1600 -resy=900
    -nosplash -nop4 -ExecutePythonScript="C:/.../script.py"`
    (UE 5.8 force-enables Python at runtime; editor EXITS after the script unless
    the script sets `unreal.EditorPythonScripting.set_keep_python_script_alive(True)`).
  - Live editor (launched without the flag): type
    `py "C:/.../script.py"` into the editor's bottom **Cmd** console box.
    This is the most reliable way to run scripts repeatedly in one session.
- Shell on this machine is cmd.exe (no head/grep). PowerShell works.
- Git LFS: committed .uasset blobs are ~129-byte pointers; `git status` clean ≠ unchanged.
- Read IMC mappings via `imc.get_editor_property("default_key_mappings")` →
  `.get_editor_property("mappings")`.
- Blueprint graph APIs (proven): `BEL.find_event_graph/find_graph`,
  `unreal.BlueprintGraphEditor.get_graph_editor(g)`, `ge.list_all_nodes()`,
  `ge.create_node_from_name("<palette path>")`, `ge.add_call_function_node(fqn)`,
  `ge.add_get/set_member_variable_node`, `ge.add_branch_node`,
  `ge.add_custom_event_node(name)`, `BEL.find_then_pin/find_execute_pin/find_input_pin`,
  `pin.try_create_connection`, `pin.set_pin_value`.
- PITFALLS:
  - `find_input_pin/find_output_pin` return broken pin instances for some pins —
    always fetch pins by iterating `list_input_pins/list_output_pins`.
  - `add_call_function_node` with an unknown FQN silently creates a WRONG node
    (e.g., Multiply_VectorFloat → "Seconds * FrameRate"). Verify by title, or use
    palette `create_node_from_name` ("Game|Damage|ApplyDamage" worked; the FQN did not,
    flaky per session). The compiler strips invalid-FQN nodes on compile.
  - SetActorEnableCollision pin is `bNewActorEnableCollision` (not "Value").
  - Promotable operators adapt type on first connection — connect a Vector first.
  - `get_all_level_actors` never returns WorldSettings; find it via
    `unreal.load_object(None, "<map>.<map>:PersistentLevel.WorldSettings")`.
  - Spawning actors immediately after `load_level` in the same tick fails silently —
    wait a few ticks (slate post-tick callback with a phase counter).
  - PIE control: `LevelEditorSubsystem.editor_request_begin_play/editor_request_end_play`;
    PIE world via `UnrealEditorSubsystem.get_game_world()`.
  - PlayerController has no `console_command` in Python — use
    `unreal.SystemLibrary.execute_console_command(world, "RestartLevel", pc)`.
  - Delegate python binding (add_callable) is unavailable in 5.8 — verify dispatchers
    indirectly via state transitions.
  - Multi-phase scripts: write the JSON evidence incrementally after EVERY phase;
    a mid-run crash otherwise loses everything.
- Computer Use lessons: bind windows by `{name: "AfghanistanFPS - Unreal Editor"}`;
  Slate exposes almost no accessibility tree (element_count 1) — screenshots +
  coordinates only; keyboard shortcuts (Ctrl+Space content drawer, Alt+F4) are more
  reliable than coordinate clicks; popup windows must be bound via window_id.

## What remains (details in Phase1_Next_Steps.md)
1. [manual, ~1 min] Assign BPI_Interactable to BP_TestInteractable via
   Class Settings → Interfaces → Add. Then run
   `py .../zcode_interact_impl.py` (write it per vol_v3 pattern: add_event_override →
   toggle logic → compile/save → does_implement verify).
2. [manual] SkyLight recapture in the test map.
3. [manual] One live WASD/Shift/Ctrl/E/R pass in PIE.
4. Commit checkpoint + push to `backup` remote (next commit message:
   `Phase1-01: player foundation, input, test map, damage/death/restart PIE-verified`).
5. Cleanup: _Scratch BPs, empty folders, cosmetic orphan nodes.
6. Phase 1 remainder: animation visual check, final 30-item manual pass, reports.

---

## SESSION ADDENDUM (2026-11-14 late): input-chain break + repair

**User's 08:15 physical test**: all input prints fired, ZERO gameplay (sprint/crouch/jump/interact dead). Root cause #2: the debug PrintStrings (commit 31e9f94) connected to the events' exec OUTPUT pins, which hold ONE link each — every connect REPLACED the wire into the gameplay chain. Full exec-wire dump proved: Jump / 5 guard Branches / Cast had empty exec inputs; IA_Move.Triggered chain survived because its print hit the unused Started pin (that is why WASD kept working while Space/Shift/Ctrl/E/R died).

**Fix**: zcode_chain_fix.py reconnected 8 wires (each print.then -> severed chain head, + trace.then -> Cast.execute); compiled BS_UP_TO_DATE; saved. Verified structurally (post-fix dump: every head fed) and functionally in PIE (zcode_final_run2.json): sprint velocity 700 cm/s / walk 450 cm/s / MWS restored; capsule 96-40-96; jump rise 89.6 cm; trace hits BP_TestInteractable_C at 171 cm; damage 100->75->50->25->0 -> dead -> RestartLevel -> 100. Cube big-scale corrected 2.0 -> 1.25 (zcode_cube_repair.json, PIE toggle 1.0 -> 1.25 -> 1.0).

**New pitfalls for the next agent (full list in Phase1_Known_Issues.md)**:
- Blueprint exec out pins hold ONE connection — inserting debug nodes must be IN SERIES with re-wiring, never a fresh connect from a busy pin. Always re-dump the exec topology after any wiring change.
- BEL pin wrappers: methods only (list_connected_pins / get_owning_node / try_create_connection / set_pin_value); get_editor_property fails on them. list_connected_pins and list_graphs return non-list sequences — convert with list().
- Struct pin literals read as "2, 2, 2"; set_pin_value accepts "(X=1.25,Y=1.25,Z=1.25)". NEVER use pin export_text/import_text for edits — they round-trip the whole node (path included) and digit-replacing corrupts it.
- Characters ignore set_actor_rotation (bUseControllerRotationYaw) — use player_controller.set_control_rotation(Rotator(roll, pitch, yaw)).
- EditorAssetLibrary.load_asset can return None mid-PIE — cache generated_class() before begin_play.
- A missing module import (e.g. time) in an -ExecutePythonScript run kills the script before the first JSON dump; the editor exits silently — check Saved/Logs for LogPython Error lines.

**Status**: all 30 acceptance items machine-verified; the user's manual key pass is the only open item.

## Environment
- Project: `C:\GameDev\AfghanistanFPS` (branch main, remote `backup` →
  `D:/GameDev/AfghanistanFPS_Backups/GitMirror/AfghanistanFPS.git`)
- Engine: `C:\Program Files\Epic Games\UE_5.8` (5.8.4)
- Previous agent scripts/evidence: `C:\Users\PC\Documents\Default Project\phase1_tests\`
- This session's scripts/evidence: `C:\Users\PC\Documents\Default Project\zcode_phase1\`
- Recovery snapshot: `D:\GameDev\AfghanistanFPS_Backups\MilestoneSnapshots\Phase1_Recovery_2026-11-13`
  (pre-change state, 3032 files verified)
- Reports: `C:\GameDev\AfghanistanFPS\Docs\Phase1_*.md`


## SESSION ADDENDUM 2 (2026-11-14, video analysis): E-trace + black screen fixed

The user's 72 s gameplay video showed (1) "INTERACT: cast FAILED" on every E press
even at the cube, (2) full-black viewport 44-55 s. Frame-by-frame ffmpeg analysis +
graph dump root-caused both:
1. Trace End = CameraLoc + Forward*MakeVector(0,0,0) — the multiplier's literal pins
   were NEVER set, so the ray had zero length and could never hit anything. Set to
   (250,250,250) (= scalar 250; forward is normalized). DrawDebugType=ForDuration so
   each E press draws the ray. Fresh-PIE verified: trace from camera (self NOT ignored)
   hits BP_TestInteractable_C at 205.4 cm — the template capsule does not self-block.
2. The interact cube floated ~1 m above the floor (placed before the floor was lowered
   to -100). A crouched player fits under it and the crouched camera (~z 9-20) entered
   the mesh (z 0..100) -> black. Cube lowered to (220,-800,-50); map saved.

Also added: 7 ExecuteConsoleCommand DisplayAll nodes chained on the FREE
InitializeHealth.then pin (bIsSprinting, bIsCrouched, CurrentHealth, bIsDead,
MaxWalkSpeed, Velocity, CapsuleHalfHeight) — a live gameplay HUD with zero changes to
existing wires. Evidence: zcode_interact_fix.json, zcode_interact_verify.json;
video frames in vidframes/ (sheet_*, dense_*).

**New pitfalls**: (a) pure-math node literals default to 0 — verify pin VALUES not
just links; (b) set_pin_value does not dirty the package — force-save with
save_loaded_asset(bp, False) and check the file mtime; (c) pin export_text/import_text
round-trips the whole node — never digit-replace it.


## SESSION ADDENDUM 3 (2026-11-14, second video): crouch UX + skylight + restart clarity

Second user video (post-fix4) + HUD readouts proved: crouch chain INTACT and Crouch()
works — the user TAPPED Ctrl (momentary crouch) and the template camera never lowered,
so crouching was invisible. Camera now lowers on crouch (rel z 69.4 -> 28.9, wired on
the crouch chain's free MWS.then pin) and restores on uncrouch. The user was ALIVE the
whole video (HUD health constant 100) — R did nothing BY DESIGN; prints now announce
"RESTARTING LEVEL" vs "R ignored: you are still ALIVE". The black flicker = BLACK
SKYLIGHT CUBEMAP from a -NullRHI recapture; recaptured with real rendering
(brightness 78.7 -> 129.2). A mid-session BS_ERROR (self-unconnected
SetRelativeLocation) was introduced and repaired in the same session; final status
BS_UP_TO_DATE. HEAD 2ba10da (Phase1-fix5).
