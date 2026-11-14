# PHASE 1 KNOWN ISSUES — Afghanistan FPS
Date: 2026-11-14 (final autonomous session)

## RESOLVED this session
- ~~BPI_Interactable assignment~~ — user assigned manually; runtime
  does_implement_interface verified True (zcode_int_verify.json).
- ~~Interact implementation~~ — event override + scale toggle (1.0 → 1.25 → 1.0 per
  user spec) implemented, compiled, saved; verified in PIE via interface call
  (zcode_tog_test.json: visible_state_toggle_works=true).
- ~~Walk/sprint speed measurement~~ — measured on clean map: 415/615 cm/s avg over
  1.5 s (acceleration ramp; MaxWalkSpeed 450/700 enforced + restored) (zcode_speed.json).
- ~~SkyLight recapture~~ — scripted recapture succeeded, map saved (zcode_sky3.json);
  confirm the Lumen warning is gone on next PIE open.
- ~~_Scratch probe deletion~~ — all 7 probe assets deleted via EditorAssetLibrary
  (delete returned True). Note below on deferred on-disk unlink.
- ~~E/Shift/Ctrl dead in PIE~~ — **ROOT CAUSE: the 4 Phase 1 key mappings were never
  persisted to IMC_Default.** `imc.map_key()` + `EditorAssetLibrary.save_asset()`
  modifies the in-memory IMC (verification showed 16 rows) but silently fails to
  serialize the added mappings — the .uasset on disk kept only the 12 template
  mappings (confirmed: fresh-session read showed 12; git never showed IMC_Default as
  modified; file size stayed 7787 B across three separate map_key attempts by two
  agents). **FIX**: build the full 16-entry EnhancedActionKeyMapping list and write it
  via `imc.set_editor_property("mappings", full_list)` — this DOES serialize (file
  grew 7787→14736 B; git shows IMC_Default modified; commit 9fa95ae). Debug display
  (10 PrintStrings on all input events + E-chain diagnostics) added to
  BP_PlayerFoundation for the user's confirmation pass (zcode_debug_display.json).

## Open (manual items remaining)
1. **Live keyboard acceptance pass** [manual — key injection from automation
   conclusively blocked; 4 mechanisms attempted, see RESOLVED note on the root cause
   for why the keys were dead before]. With the IMC mappings now persisted, PIE the
   test map and: WASD walk, Shift sprint, Ctrl crouch, E interact (cube 2.2 m ahead
   of spawn), R after death. **Debug display added**: pressing W/Space/Shift/Ctrl/E/R
   now prints "INPUT: <event> fired" on screen — if a print appears, the input event
   fired; if sprint/crouch/interact still misbehave with prints showing, the issue is
   in the gameplay logic (report which prints appeared).

## Low
3. **BP_PlayerFoundation EventGraph contains cosmetic orphans** (unconnected pure math
   nodes from iterative builds; compiler drops them). Optional manual tidy.
4. **`Content/AfghanFPS/Interaction/` contains empty Door/JumpPad/Materials/Test
   folders** (origin unclear — not created by either agent session). Safe to delete
   in editor.
5. **_Scratch deferred deletion**: delete_asset returned True for all 7 probes, but 6
   .uasset files remained on disk while the editor held them loaded (deferred unlink).
   They unlink when the editor closes; folder is git-untracked so the repo is
   unaffected. Verify visually next editor open.
6. **OnHealthChanged/OnDeath python-side verification** limited (DelegateBase lacks
   add_callable in 5.8). Both dispatchers ARE wired and called in BPC_Health's graphs;
   death state transition verified end-to-end in PIE. Manual visual confirmation of
   the "YOU DIED"/HP prints advised.

## Backup remote failure (worked around)
7. **`git push backup main` fails**: the bare mirror on
   `D:/GameDev/AfghanistanFPS_Backups/GitMirror/AfghanistanFPS.git` rejects pushes —
   `git receive-pack`/fetch gets "Permission denied" writing objects into its
   quarantine (`objects/tmp_objdir-incoming-*`), on varying object paths. Manual
   PowerShell writes to the exact same paths succeed → environmental (likely AV/EDR
   intercepting git.exe child-process writes to D:). LFS objects DID upload (11/11).
   **Workaround applied**: full-history git bundles on D:
   `D:\GameDev\AfghanistanFPS_Backups\AfghanistanFPS_main_final.bundle` (3.99 MB,
   verified "complete history", tip 99c78f3) plus the earlier
   `..._2026-11-14b.bundle` (tip 7353a7e). Restore with
   `git clone <bundle> <dir>` or `git fetch <bundle> main`. LFS objects are already
   on the mirror's LFS storage (11/11 uploaded) — bundle + LFS together restore the
   project fully. Retry `git push backup` next session; if still failing,
   investigate the machine's AV/exclusion list for git.exe.
   **Final bundle**: `D:\...\AfghanistanFPS_Backups\AfghanistanFPS_main_v4.bundle`
   (3.81 MB, verified complete history, tip c2868cb, 2026-11-14).

## Environment notes (for the next agent)
- Shell on this machine is **cmd.exe** (no head/unix tools); PowerShell available.
- `git status` sees .uasset/.umap as clean via **Git LFS** (committed blobs are
  LFS pointers ~129 B; working files are real). Don't misread clean status as
  "asset unchanged".
- `UInputMappingContext` mappings: read via `default_key_mappings` → `.mappings`
  (the `mappings` property returns empty).
- Editor Python node creation: `add_call_function_node` with invalid FQNs silently
  creates wrong nodes — verify by title/pins, or prefer palette
  `create_node_from_name`. `find_input_pin`/`find_output_pin` wrappers return broken
  instances for some pins — iterate `list_input_pins`/`list_output_pins` instead.
- K2 pin for SetActorEnableCollision is named `bNewActorEnableCollision`.
- HitResult in python has no direct members — use `hit.to_dict()["hit_actor"]`.
- Editor auto-exits after `-ExecutePythonScript` unless the script sets
  `set_keep_python_script_alive(True)`. Long-lived harnesses: register a slate
  post-tick callback, write JSON incrementally (a late crash otherwise loses evidence).
- EditorAssetLibrary calls (load_asset, does_asset_exist, get_all_level_actors)
  misbehave while PIE is running — end PIE first, then run editor-side operations
  from the Cmd console.
- Level/actor spawns immediately after `load_level` inside a slate tick fail silently —
  wait several ticks between world operations.
- set_control_rotation drives the FPS camera; set_actor rotation alone does not
  (camera follows controller rotation in the template).
