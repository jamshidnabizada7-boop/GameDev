# PHASE 1 KNOWN ISSUES — Afghanistan FPS
Date: 2026-11-13/14

## Blocking (needs one manual GUI step)
1. **BPI_Interactable not implemented by BP_TestInteractable.**
   UE 5.8 exposes no Python API for Blueprint interface assignment
   (`add_interface` absent, `add_function_override` returns None, `implemented_interfaces`
   protected). Manual fix (≈1 min): open `BP_TestInteractable` → toolbar **Class Settings**
   → Details → **Interfaces → Add** → select `BPI_Interactable` → Compile → Save.
   Then, from the editor Cmd console, run:
   `py "C:/Users/PC/Documents/Default Project/zcode_phase1/zcode_vol_fix2.py"`
   — no; instead run the provided interact implementation step (see Handover, "Finish
   interaction"), which calls `BEL.add_event_override(bp, "Interact", IntPoint)` (works
   once the interface is assigned) and wires the scale-toggle + PrintString feedback.

## Medium
2. **SkyLight not recaptured.** Viewport shows "Cached lighting in Lumen and real-time
   sky capture lighting is going to be clipped". The earlier recapture attempt coincided
   with an editor crash, so it was removed from automation. Fix: select SkyLight →
   Details → **Recapture Sky** (manual), or run a scripted recapture next session.
3. **Live keyboard input from automation unverified.** OS-level key injection reached
   the editor but not the PIE game view reliably (focus semantics). Movement itself is
   proven via BP-function driving; do one manual WASD/Shift/Ctrl/E/R pass in PIE.
4. **Exact walk/sprint travel speeds** not measured on a clear path (test paths hit the
   interact cube / walls — collision itself verified). Manual: open the map, PIE, walk
   with `stat unit`/`showdebug` or a stopwatch; expect ~450 and ~700 cm/s.
5. **Damage volume re-arm chain removed.** The SetActorEnableCollision(false)→Delay→
   enable chain was removed after repeated automation failures; per-entry damage
   (walk in/out) satisfies Phase 1 ("not every frame"). If a cooldown is wanted, add
   it manually in the volume BP (Delay 2 → gate variable).

## Low
6. **BP_PlayerFoundation EventGraph contains cosmetic orphans** (unconnected pure math
   nodes from iterative builds; compiler drops them). Optional manual tidy.
7. **`Content/AfghanFPS/Interaction/` contained empty Door/JumpPad/Materials/Test
   folders** (origin unclear — not created by this session). Safe to delete in editor.
8. **_Scratch assets** (8 probe BPs) still present, intentionally left OUT of the
   Phase1-01 commit. No production references found; delete via editor asset tools
   after confirming the recovery snapshot is current.
9. **OnHealthChanged/OnDeath python-side verification** limited (DelegateBase lacks
   add_callable in 5.8). Both dispatchers ARE wired and called in BPC_Health's graphs;
   death state transition verified end-to-end in PIE.

## Backup remote failure (worked around)
10. **`git push backup main` fails**: the bare mirror on
    `D:/GameDev/AfghanistanFPS_Backups/GitMirror/AfghanistanFPS.git` rejects pushes —
    `git receive-pack` / fetch gets "Permission denied" writing objects into its
    quarantine (`objects/tmp_objdir-incoming-*`), on varying object paths. Manual
    PowerShell writes to the exact same paths succeed → environmental (likely AV/EDR
    intercepting git.exe child-process writes to D:). LFS objects DID upload (11/11).
    **Workaround applied**: full-history git bundle
    `D:\GameDev\AfghanistanFPS_Backups\AfghanistanFPS_main_2026-11-14.bundle`
    (3.8 MB, verified "complete history", tip 7353a7e). Restore with
    `git clone <bundle> <dir>` or `git fetch <bundle> main`. Retry `git push backup`
    next session — if still failing, investigate the machine's AV/exclusion list.

## Environment notes (for the next agent)
- Shell on this machine is **cmd.exe** (no head/unix tools); PowerShell available.
- `git status` sees .uasset/.umap as clean via **Git LFS** (committed blobs are
  LFS pointers ~129 B; working files are real). Don't misread clean status as
  "asset unchanged".
- `UInputMappingContext` mappings: read via `default_key_mappings` → `.mappings`.
- Editor Python node creation: `add_call_function_node` with invalid FQNs silently
  creates wrong nodes — verify by title/pins, or prefer palette
  `create_node_from_name`. `find_input_pin`/`find_output_pin` wrappers return broken
  instances for some pins — iterate `list_input_pins`/`list_output_pins` instead.
- K2 pin for SetActorEnableCollision is named `bNewActorEnableCollision`.
- Editor auto-exits after `-ExecutePythonScript` unless the script sets
  `set_keep_python_script_alive(True)`. Long-lived harnesses: register a slate
  post-tick callback, write JSON incrementally (a late crash otherwise loses evidence).
- Level/actor spawns immediately after `load_level` inside a slate tick fail silently —
  wait several ticks between world operations.
