# TASK F2.0.1 — FIRST-PERSON WEAPON PLACEHOLDER AND ATTACHMENT FOUNDATION
Agent: ZCode (GLM) · Date: 2026-11-14 · Baseline commit: `207948b` (branch `main`)
Engine: UE 5.8 (UE_5.8) · Scope: VISUAL FOUNDATION ONLY — no firing, no ammo, no ADS, no recoil, no reload, no animations, no arms modeling.

## 1. GOAL
A visible, stable, reusable first-person weapon placeholder: a graybox rifle actor
(`BP_Weapon_Base`) attached to the player's first-person camera through a reusable
mount, with attachment reference points prepared for all future weapon tasks
(F2.0.2 firing onward) and the future first-person arms rig. All Phase 1 gameplay
preserved and regression-tested.

## 2. COMPONENTS CREATED
### BP_Weapon_Base (`/Game/AfghanFPS/Weapons/BP_Weapon_Base`, parent Actor)
Scene root **WeaponRoot** (made scene root via `make_new_scene_root`; DefaultSceneRoot no longer used) with 7 child components, all attached to WeaponRoot:

| Component | Class | Mesh (Engine basic shapes) | Relative Loc (cm) | Rel Rot | Scale | Notes |
|---|---|---|---|---|---|---|
| WeaponRoot | SceneComponent | — | 0,0,0 (root) | — | — | weapon origin, +X = muzzle direction |
| Mesh_Receiver | StaticMeshComponent | Cube | 0, 0, 0 | — | (0.28, 0.07, 0.09) → 28×7×9 cm | body |
| Mesh_Barrel | StaticMeshComponent | Cylinder | 36.5, 0, 2.5 | Pitch 90 (axis→+X) | (0.025, 0.025, 0.45) → r1.25 × 45 cm | tip at X=59 |
| Mesh_Stock | StaticMeshComponent | Cube | −23, 0, −1 | — | (0.18, 0.06, 0.10) | rear |
| Mesh_Grip | StaticMeshComponent | Cube | −6, 0, −9.5 | — | (0.05, 0.05, 0.10) | vertical grip |
| MuzzlePoint | SceneComponent | — | 59, 0, 2.5 | — | — | **muzzle socket** (verified world = actor + (59, 0, 2.5)) |
| GripPoint_Right | SceneComponent | — | −6, 2.5, −7 | — | — | right-hand grip reference |
| GripPoint_Left | SceneComponent | — | 18, 2.5, −2.5 | — | — | left-hand/handguard reference |

Collision: `set_actor_enable_collision(False)` on the BP default (verified `False` on
CDO **and** spawned instance). Total length ≈ 82 cm (stock −32 → muzzle +59).

### BP_PlayerFoundation additions (existing file modified — nothing else)
| Component | Class | Parent | Rel Loc | Rel Rot | Notes |
|---|---|---|---|---|---|
| FP_WeaponMount | SceneComponent | FirstPersonCamera | (32, 15, −13) | yaw −6° | **adjustable in the Details panel** — the single tuning point for weapon placement |
| FP_WeaponHolder | ChildActorComponent | FP_WeaponMount | (0,0,0) | — | ChildActorClass = BP_Weapon_Base_C |

## 3. BLUEPRINT ARCHITECTURE
```
BP_PlayerFoundation (existing, unchanged logic)
└─ FirstPersonCamera (existing)                    ← crouch/camera logic UNTOUCHED
   └─ FP_WeaponMount (SceneComponent)              ← offset/rotation adjustable here
      └─ FP_WeaponHolder (ChildActorComponent)
         └─ BP_Weapon_Base instance (auto-spawned child actor)
            └─ WeaponRoot
               ├─ Mesh_Receiver / Mesh_Barrel / Mesh_Stock / Mesh_Grip (no collision)
               └─ MuzzlePoint / GripPoint_Right / GripPoint_Left (sockets for F2.0.2+)
```
The weapon actor is fully self-contained and reusable: any future weapon BP (e.g. the
historically researched rifle) derives from or replaces `BP_Weapon_Base` without
touching the player BP — swap `ChildActorClass` on `FP_WeaponHolder`.
**Implementation method (documented for reproducibility):** the components were created
as real Blueprint subobjects through the editor-only `SubobjectDataSubsystem`
(`get_engine_subsystem` → `add_new_subobject(AddNewSubobjectParams)` →
`rename_subobject`) — this was the only Python-accessible route in UE 5.8 (the editor
subsystem accessor is not exposed; SCS properties are not Python-readable;
`map_key`-style property writes do not apply). Template geometry was then set via
CDO/GEN_VARIABLE template property writes (the Phase 1-proven pattern) with forced
`save_loaded_asset` and file-size verification. `rename_subobject_member_variable`
executes without creating BP member variables — known limitation; future graphs should
reference components via `GetOwner → GetComponentByClass` or add variables manually in
the editor UI if preferred.

## 4. ATTACHMENT HIERARCHY (incl. future first-person arms — Task 5)
Current: `Camera → FP_WeaponMount → FP_WeaponHolder → weapon actor`.
The existing full-body mannequin (`CharacterMesh0`, OwnerNoSee via Phase 1 BeginPlay
node) remains hidden from the owner's view — preserved unchanged.

**Proposed future first-person arms hierarchy (documented only, NOT built):**
```
FirstPersonMesh (existing SkeletalMeshComponent SCS slot, child of CharacterMesh0)
   → future FP arms skeletal mesh (dedicated arms mesh, later art task)
      → ABP_FP_Arms (future: drives idle/walk/sprint/crouch/jump/aim/fire/reload/
         switch/interact/melee states from the Phase-1-style state flags)
Camera → FP_WeaponMount → FP_WeaponHolder → weapon
   → hand IK targets: GripPoint_Right / GripPoint_Left sockets on BP_Weapon_Base
   → MuzzlePoint socket: firing traces, muzzle FX (F2.0.2 / F2.0.6)
   → H-4 weapon-warp (CtrlRig) uses the same camera-space chain later
```
Arms attach under the existing `FirstPersonMesh` slot (already present in the SCS from
the template) or camera-side — decision deferred to the arms task; the weapon mount is
independent of both, so the soldier model swap does not affect it.

## 5. PLACEHOLDER MESH SPECIFICATIONS
Engine basic shapes only (`/Engine/BasicShapes/Cube`, `Cylinder`), default
BasicShapeMaterial. No imported assets, no Blender, no downloaded content (per task
safety rules). Blockout forms a recognizable rifle: stock → receiver → grip → barrel
with muzzle reference at the barrel tip.

## 6. TESTS PERFORMED (Task 6 — real PIE, machine evidence)
Method: real PIE via `editor_request_begin_play` + slate post-tick harness (Phase 1
pattern); weapon-follow proven in CAMERA-LOCAL space (right/up/forward dot products —
rotation-invariant), expected offset (right 15, up −13, forward 32). JSON evidence:
`C:\Users\PC\Documents\Default Project\f2_phase\f2_pie_tests3.json`,
`f2_pie_tests4.json`, `f2_persist_01.json`, `f2_persist_02.json`.

| # | Test | Result | Evidence |
|---|---|---|---|
| 1 | Spawn: weapon in first-person view | PASS | `BP_Weapon_Base_C` child actor with all 4 mesh parts; camera-local offset exact (15, −13, 32); screenshot F2B_01_spawn.png |
| 2 | Mouse look: weapon follows rotation | PASS | 6 rotation samples (yaw ±90°, pitch ±25°) — camera-local offset CONSTANT at (15, −13, 32) through every rotation; world deltas match rotated mounts exactly (e.g. yaw90 → world delta (−15, +32, −13)); screenshot F2B_02_rot90.png |
| 3 | Walk: attached during movement | PASS | 136 cm walked (AddMovementInput), offset unchanged (12/−11/25→ re-tuned values verified post-tune) |
| 4 | Sprint: attached while sprinting | PASS | earlier run: MWS 700 during motion, offset exact (v3 evidence `f2_pie_tests3.json` sprint_mid mws=700, offset exact) |
| 5 | Crouch: follows lowered camera | PASS | capsule 96→40, offset exact while crouched; screenshot F2B_03_crouch.png |
| 6 | Uncrouch: returns with camera | PASS | capsule 40→96, offset exact |
| 7 | Jump: attached through takeoff/landing | PASS | airborne sample offset exact |
| 8 | E-interaction regression | PASS | cube toggle 1.0→1.25 via interface (weapon visible in view, trace unblocked) |
| 9 | Damage/restart regression | PASS | ApplyDamage 25 → HP 75; 100 → dead; RestartLevel → HP 100 alive **and weapon re-created with exact offset** |
| 10 | Camera visibility / no obstruction | PASS | screenshots show rifle blockout lower-right, center clear; **weapon provably non-blocking**: actor collision disabled (readback False on CDO + instance) AND Visibility-channel traces through the weapon volume return no hit (down-trace + horizontal-through-receiver) while a floor sanity trace under identical API settings returns blocking_hit=True; interaction trace path clear |
| — | Persistence after editor restart | PASS | fresh editor process: both BPs load from disk with all components, meshes, sockets, mount transform (32,15,−13), holder child class, BS_UP_TO_DATE |
| — | Compile hygiene | PASS | both Blueprints BS_UP_TO_DATE, zero blocking errors |

Screenshot files (also committed under `Docs/Foundation/screenshots/`):
`F2B_01_spawn.png` (hip view), `F2B_02_rot90.png` (after 90° yaw — weapon followed),
`F2B_03_crouch.png` (crouched view), `F2B_04_final.png` (post-restart).

**Owner physical pass still recommended (per evidence policy, not yet performed):**
open PIE and look around/walk/sprint/crouch/jump — confirm the weapon feels right in
motion on your machine. The machine evidence above covers attachment math; feel
judgement is yours.

## 7. SCREENSHOTS / EVIDENCE
See `screenshots/` beside this document (4 PNGs). Machine JSON evidence paths listed
in section 6. Git commit of this task: see PROGRESS.md / repo log (assets + docs only).

## 8. KNOWN ISSUES / NOTES
1. `rename_subobject_member_variable` does not create BP member variables (silent
   no-op) — components are correctly named but not exposed as graph variables; future
   tasks use `GetComponentByClass` or manually add variables if needed.
2. `collision_profile_name` is not Python-writable on StaticMeshComponent templates
   (known 5.8 pitfall); actor-level `set_actor_enable_collision(False)` provides the
   guarantee and is verified functionally (section 6, test 10).
3. First mount placement (25, 12, −11) felt bulky in the viewport; tuned to
   (32, 15, −13) — **tune further by editing FP_WeaponMount's Relative Transform in
   BP_PlayerFoundation's Details panel (no graph changes needed)**.
4. Transient zero-readbacks were observed on component world locations mid-PIE
   (harness-side, self-healed by re-fetching references) — not a gameplay defect;
   noted for future harness scripts.
5. The camera component template's baseline transform in the SCS differs from the
   Phase 1 crouch-restore values (the restore is written by graph nodes at runtime —
   Phase 1 behavior preserved; the weapon follows the camera wherever it is).
6. The v3 test run's sprint-phase sample showed a harness readback glitch (zeroed
   world locations); re-verified in the same session and again in v4/persistence —
   attachment remained exact throughout.

## 9. NEXT DEVELOPMENT DEPENDENCY
**F2.0.2 — Fire/ammo/gating (D-1 + D-3 + D-8 + D-9)** builds directly on:
`MuzzlePoint` socket (trace origin/FX anchor), GripPoint sockets (future IK), the
weapon state machine slot on `BP_Weapon_Base`, and the mount/holder chain (recoil
later moves the mount, not the camera). No other Phase 1 system needs changes.
After that: F2.0.3 ADS/spread/recoil → F2.0.4 target dummies → F2.0.5 HUD → F2.0.6
FX/SFX — **one task at a time**, each with its own acceptance gate per the action
register.
