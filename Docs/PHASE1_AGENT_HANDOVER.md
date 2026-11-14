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

## Environment
- Project: `C:\GameDev\AfghanistanFPS` (branch main, remote `backup` →
  `D:/GameDev/AfghanistanFPS_Backups/GitMirror/AfghanistanFPS.git`)
- Engine: `C:\Program Files\Epic Games\UE_5.8` (5.8.4)
- Previous agent scripts/evidence: `C:\Users\PC\Documents\Default Project\phase1_tests\`
- This session's scripts/evidence: `C:\Users\PC\Documents\Default Project\zcode_phase1\`
- Recovery snapshot: `D:\GameDev\AfghanistanFPS_Backups\MilestoneSnapshots\Phase1_Recovery_2026-11-13`
  (pre-change state, 3032 files verified)
- Reports: `C:\GameDev\AfghanistanFPS\Docs\Phase1_*.md`
