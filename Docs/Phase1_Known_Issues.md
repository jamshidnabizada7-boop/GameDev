# PHASE 1 KNOWN ISSUES — Afghanistan FPS
Date: 2026-11-14 (input-chain repair session + final autonomous session)

## RESOLVED this session (input-chain repair)
- **Events fired but gameplay did nothing** (user physical test 2026-11-14 08:15:
  all 7 input prints appeared on screen; Shift sprint, Ctrl crouch, Space jump,
  E interact produced NO visible gameplay). **ROOT CAUSE #2: the input debug
  PrintStrings hijacked the events' exec output pins.** A Blueprint exec output
  pin holds exactly ONE outgoing connection; the debug-display script connected
  each print to the event's Started/Completed exec pin, silently REPLACING the
  existing wire into the gameplay chain. Every affected chain's first node was
  left dangling with an unconnected exec input (confirmed by full graph dump
  zcode_chain_dump4.json: Jump, 5 guard Branches, Set bIsSprinting, and
  Cast-to-BPI all had empty exec inputs; a full exec-wire topology dump listed
  165 real connections). WASD and mouse look survived because their prints
  targeted the unused "Started" pin while the real chains live on "Triggered"
  (and no print was added to the look events). The earlier machine "PASS"
  evidence never caught this because it drove gameplay by DIRECT function
  invocation (`call_method("Jump")`, `call_method("Crouch")`,
  `set_editor_property("max_walk_speed")`) — bypassing the event chains entirely.
  **FIX (zcode_chain_fix.json): 8 wires reconnected** — each print.then → its
  severed chain head: Jump; sprint-on Branch (→Set bIsSprinting true →MWS
  SprintSpeed 700); sprint-off Branch (→Set bIsSprinting false →MWS WalkSpeed
  450); crouch-on Branch (→Crouch→cancel sprint→MWS 450); crouch-off Branch
  (→UnCrouch→MWS 450); interact Branch (→camera trace→cast→Interact message);
  restart Branch (→ExecuteConsoleCommand RestartLevel); plus trace.then→
  Cast.execute gap. Compiled BS_UP_TO_DATE, saved. Post-fix PIE measurements
  (zcode_final_run2.json): walk velocity 450.0 cm/s; sprint velocity 700.0 cm/s;
  MWS restored to 450 after release; jump rise 89.6 cm; crouch capsule 96→40.0,
  uncrouch (open space) back to 96.0; crouch-move 188.8 cm/s (~200 spec);
  interact camera trace hits BP_TestInteractable_C at 171.3 cm (< 250 spec);
  damage: direct 25 → 75, volume entries 75→50→25→0 → bIsDead=True →
  RestartLevel → HP 100.0, alive.
- ~~Interact cube big-scale 2.0 → spec 1.25~~ — the BP_TestInteractable toggle
  node's big-scale value was "2, 2, 2" (changed by an earlier probe session);
  set to (X=1.25,Y=1.25,Z=1.25) via set_pin_value (zcode_cube_repair.json) and
  PIE-verified: 1.0 → 1.25 → 1.0. CAUTION: pin `export_text`/`import_text`
  round-trip exports the WHOLE NODE (including its path) — a naive text replace
  corrupts it (K2Node→"K1.25Node"); use set_pin_value for pin literals.
- ~~Volume damage no-op scare~~ — one measurement session showed 0 damage across
  repeated teleports; a clean-session diagnostic (zcode_damage_diag2.json)
  proved all three paths work: direct ApplyDamage(25)→75, teleport-in→50,
  walk-in→25. Volume asset verified: ActorBeginOverlap→ApplyDamage with
  BaseDamage=25.0 saved. The no-op session had a corrupted pre-state (failed
  actor rotations left the pawn wedged against the cube before the damage phase).
  UnCrouch is clearance-blocked while wedged against the interact cube — correct
  UE behavior, not a bug.

## RESOLVED (earlier session)
- ~~BPI_Interactable assignment~~ — user assigned manually; runtime
  does_implement_interface verified True (zcode_int_verify.json).
- ~~Interact implementation~~ — event override + scale toggle implemented,
  compiled, saved; verified in PIE via interface call (zcode_tog_test.json).
- ~~Walk/sprint speed measurement~~ — measured on clean map (zcode_speed.json).
- ~~SkyLight recapture~~ — scripted recapture succeeded, map saved (zcode_sky3.json).
- ~~_Scratch probe deletion~~ — all 7 probe assets deleted via EditorAssetLibrary.
- ~~E/Shift/Ctrl dead in PIE~~ — **ROOT CAUSE #1: the 4 Phase 1 key mappings were
  never persisted to IMC_Default.** `imc.map_key()` + save silently failed to
  serialize (file stayed 7787 B). **FIX**: `imc.set_editor_property("mappings",
  full_list)` serializes correctly (file grew 7787→14736 B; commit 9fa95ae).

## Open (manual items remaining)
1. **Live keyboard acceptance pass** [manual — key injection from automation
   conclusively blocked; 4 mechanisms attempted]. With BOTH root causes fixed
   (IMC mappings persisted + event chains rewired), PIE the test map and: WASD
   walk, Shift sprint (700 vs 450), Ctrl crouch (capsule shrinks), Space jump,
   E interact (cube 1.0→1.25→1.0), R after death (HP back to 100). The debug
   prints stay in series (event → print → gameplay), so each keypress shows its
   "INPUT: ..." print AND performs the action.

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
  No standalone Python — use the ZCode Node REPL (mcp__node_repl) for JSON analysis
  or UE's embedded python for anything in-engine.
- `git status` sees .uasset/.umap as clean via **Git LFS** (committed blobs are
  LFS pointers ~129 B; working files are real). Don't misread clean status as
  "asset unchanged".
- `UInputMappingContext` mappings: read via `default_key_mappings` → `.mappings`
  (the `mappings` property returns empty).
- **Blueprint exec output pins hold ONE outgoing connection.** Connecting anything
  new to an already-connected exec out pin silently REPLACES the existing wire.
  Never "add" a print by connecting from an event's exec pin that already feeds a
  chain — insert the print IN SERIES (event→print→chain head) and always re-wire
  the downstream link. Verify with a post-save exec-wire dump.
- **Reading exec wires with this environment's wrappers**: nodes from
  `BlueprintGraphEditor.list_all_nodes` are wrappers — `get_editor_property("pins")`
  does NOT work; use `BEL.list_input_pins/list_output_pins`. Pin wrappers expose
  methods only (`get_pin_name`, `get_pin_value`, `list_connected_pins`,
  `get_owning_node`, `try_create_connection`, `set_pin_value`) — no properties.
  `list_connected_pins`/`list_graphs` return non-python-list sequences — iterate
  via `list(obj)`, not `isinstance(obj, list)`.
- Struct pin values read back as compact literals ("2, 2, 2"); `set_pin_value`
  accepts "(X=1.25,Y=1.25,Z=1.25)". **Do NOT use pin `export_text`/`import_text`**
  — they export/import the WHOLE NODE (path included) and text-replacing digits
  corrupts the node.
- Editor Python node creation: `add_call_function_node` with invalid FQNs silently
  creates wrong nodes — verify by title/pins, or prefer palette
  `create_node_from_name`. `find_input_pin`/`find_output_pin` wrappers return broken
  instances for some pins — iterate `list_input_pins`/`list_output_pins` instead.
- K2 pin for SetActorEnableCollision is named `bNewActorEnableCollision`.
- HitResult in python has no direct members — use `hit.to_dict()["hit_actor"]`.
- Editor auto-exits after `-ExecutePythonScript` unless the script sets
  `set_keep_python_script_alive(True)`. Long-lived harnesses: register a slate
  post-tick callback, write JSON incrementally (a late crash otherwise loses evidence).
  A module-level NameError (e.g. missing `import time`) kills the script BEFORE the
  first dump — the editor exits silently with no JSON; check Saved/Logs for
  LogPython Error lines.
- EditorAssetLibrary.load_asset can transiently return None mid-PIE — cache
  generated_class() references BEFORE begin_play.
- `set_actor_rotation` on a Character is overridden every tick by
  bUseControllerRotationYaw — drive pawn facing via
  `player_controller.set_control_rotation(Rotator(roll,pitch,yaw))` instead.
  Rotator constructor order is (roll, pitch, yaw).
- Level/actor spawns immediately after `load_level` inside a slate tick fail silently —
  wait several ticks between world operations.
- set_control_rotation drives the FPS camera; set_actor rotation alone does not
  (camera follows controller rotation in the template).
- Crouch-blocked UnCrouch: UE blocks standing when clearance is insufficient —
  measure UnCrouch in open space, not wedged against geometry.
- Direct BPC function calls (`comp.call_method("ApplyDamage", (25.0,))`) return
  None on success — check the HP change, not the return value.
- **`set_pin_value` does NOT mark the package dirty** → `EditorAssetLibrary.save_asset`
  silently no-ops (returns True, writes nothing, mtime unchanged). After pin-only
  edits, force with `unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)`
  and CONFIRM the write by checking the file mtime afterward (this bit the
  BP_TestInteractable 1.25 fix — three save_asset calls returned True with zero
  bytes written).


## RESOLVED (video session, 2026-11-14 later)
- ~~E interaction always "cast FAILED"~~ — **ROOT CAUSE #3: zero-length trace.** The
  chain built End = CameraLoc + Forward * MakeVector(X=0,Y=0,Z=0) (defaults never
  set). Fixed to (250,250,250); DrawDebugType=ForDuration (visible ray); fresh-PIE
  verified hit BP_TestInteractable_C at 205.4 cm with self NOT ignored (the template
  capsule does not block the visibility ray). **Pattern to remember: verify pure-math
  node pin VALUES, not just their links — an unset literal silently zeroes the math.**
- ~~Black screen 44-55 s during physical play~~ — camera clipped inside the FLOATING
  interact cube (placed at z=50 before the floor was lowered to -100; crouched camera
  z ~9-20 entered the mesh). Cube lowered to the floor (220,-800,-50); map saved;
  no stance can clip the camera into it now.
- ~~set_pin_value not dirtying the package~~ — save_asset silently no-ops after
  pin-only edits; use save_loaded_asset(bp, only_if_is_dirty=False) and verify mtime.


## RESOLVED (second video session)
- ~~Black flicker during play~~ — SkyLight cubemap was BLACK: a prior recapture ran in
  a headless -NullRHI session (nothing rendered -> black capture, saved to the map).
  Rule: **SkyLight recapture requires a rendering editor session (no -NullRHI)**.
  Recaptured properly; level-gaze brightness 78.7 -> 129.2. Residual dark bands at
  wall bases are the sun's shadow, not a bug.
- ~~"Can't crouch"~~ — user TAPPED Ctrl (print evidence) + template camera never
  lowered = invisible crouch. Camera now lowers (rel z 69.4 -> 28.9) on crouch and
  restores on uncrouch. Chain verified intact machine-side; HOLD Ctrl to stay crouched.
- ~~"R not working"~~ — user was ALIVE (HUD: health never left 100). R restarts only
  when dead, by design. New prints make both outcomes visible.
- **New pitfall**: ge.create_node_from_name requires unreal.Vector2D for the position
  (unreal.Vector raises TypeError). A SetRelativeLocation node with an unconnected
  self pin compiles to BS_ERROR — always wire self (data pin via out_data, NOT the
  exec-only out_exec helper). Direct call_method("Crouch") does NOT execute the BP
  chain nodes downstream of the Crouch node — camera-lower effects can only be
  verified via the physical chain or full event simulation.


## RESOLVED (third video session)
- ~~Black screen (real root cause)~~ — the BOUNDED PostProcessVolume had AEM_MANUAL
  with auto_exposure_min_brightness=-10.0 (1/1024 exposure): the non-sunlit part of
  the room rendered near-black; walking across its boundary = "black then bright".
  PPV deleted; fill DirectionalLight (yaw 210) added; SkyLight 8.0 MOVABLE real-time
  capture. Midroom brightness 17.5 -> 116.6 verified by screenshot measurement.
- ~~Volume invisible/hard to find~~ — red emissive marker box + red PointLight glow
  now mark the damage zone; no shadow casting; kill chain re-verified with marker.
- ~~Saves silently failing~~ — a second open editor instance on the same project
  breaks save_asset/save_current_level (returns False). Close the other editor first.
- **Rule for the user**: after agent-side fixes, FULLY CLOSE AND REOPEN the editor
  before testing — PIE runs the editor's in-memory assets, not the disk.


## RESOLVED (fix8)
- ~~Volume unreachable on foot~~ — the red marker's collision silently stayed ON
  (StaticMeshComponent has no python-exposed collision_enabled property; the earlier
  set was a silent no-op) and blocked the pawn 50 cm short of the trigger. Fixed with
  actor-level SetActorEnableCollision(False); walk-through + damage verified
  (y=100->966 clean, HP 100->75->50->25->0->restart->100).
- **Pitfall**: component-level collision settings are not reliably python-settable —
  use actor.set_actor_enable_collision(False) and verify by MOVEMENT, not readback.


## FINAL SESSION ADDITIONS (2026-11-14, Phase 1 completion)
- ~~Crouch camera "shows a side"/body offset~~ — root cause chain: (1) full-body mesh
  owner-visible (fixed by SetOwnerNoSee runtime node), (2) camera moved forward in
  fix9 while the body stayed centered (documented trade-off; with OwnerNoSee active the
  forward camera gives a clean view). Crouch/uncrouch verified working by the user
  (capsule 96->40->96, prints firing).
- ~~Accessed None at boot~~ — ABP_Copycat on Lvl_IntroRoom (old startup map); fixed by
  startup-map change. Lvl_IntroRoom itself still contains that unmodified demo Blueprint
  — it is unused template content (documented, not repaired).
- **Debug output management**: all INPUT:/CROUCH:/INTERACT: prints are wired IN SERIES
  on the gameplay chains (never replacing connections) and can be silenced at any time
  with the console command 'DisableAllScreenMessages' (the DisplayAll HUD lines honor
  the same switch). No graph changes needed to toggle diagnostics.
- **Restore test**: fresh 'git clone' from GitHub into C:\GameDev\RestoreTest_AfghanFPS
  produced a complete project (160 content files, 47.9 MB; BP_PlayerFoundation.uasset
  434 KB real binary — LFS smudge OK; test map present; commit fc7d9b6). Non-destructive;
  the folder can be deleted at will.
