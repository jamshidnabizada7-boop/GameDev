# FOUNDATION ACTION REGISTER — AfghanistanFPS
Task F1.1 — Complete Gameplay Action Inventory and Gap Analysis (READ-ONLY AUDIT)
Author: ZCode (GLM) · Date: 2026-11-14 · Baseline commit: `04bf7cd` (branch `main`)
Engine: UE 5.8 (installed build `C:\Program Files\Epic Games\UE_5.8`, verified via registry; Phase 1 sessions logged 5.8.4)

**Audit method and evidence rules.** This was a documentation and filesystem audit: git
history, `Config/*.ini`, the full `Content/` asset tree, and the Phase 1 evidence
documents (`Phase1_Test_Results.md`, `Phase1_Implementation_Report.md`,
`Phase1_Known_Issues.md`). **No engine session was run in this task** (read-only
guarantee); every "IMPLEMENTED AND TESTED" classification is inherited from the Phase 1
30/30 acceptance record with its USER/PIE/STRUCT evidence classes. An asset existing is
never treated as the feature working. "Observed in video" refers to the owner-supplied
graybox reference recording, used as a functional quality bar only — not as proof of the
reference's internal implementation.

---

## PART 1 — EXISTING FOUNDATION STATUS

### 1.1 Environment (verified this audit)
| Item | Value |
|---|---|
| Engine | UE 5.8, registry build path `C:/Program Files/Epic Games/UE_5.8` |
| Project | `C:\GameDev\AfghanistanFPS`, branch `main`, HEAD `04bf7cd` |
| Working tree | Clean except untracked `Docs/Characters/GROM_01/ConceptArt/` (owner-added; preserved untouched) |
| GitHub | `github` remote = jamshidnabizada7-boop/GameDev, synced through `04bf7cd` |
| Backup | D: git-bundle mirror working; `git push backup` blocked by AV/EDR (known environmental issue) |
| Dev PC | RTX 3060 · Ryzen 5 7500F · 16 GB RAM — lightweight graybox implementations required |

### 1.2 Implemented systems (Phase 1, all preserved)
| System | Assets | Status | Evidence |
|---|---|---|---|
| Movement: walk 450 / sprint 700 / crouch 200 cm/s, jump 420 Z, stance conflict guards | `BP_PlayerFoundation` + `IMC_Default` | IMPLEMENTED AND TESTED | Phase1 30/30 items 4–13 (USER+PIE) |
| Look/camera rig (controller rotation, template camera) | `BP_PlayerFoundation` camera | IMPLEMENTED AND TESTED | items 5, 14 |
| Crouch camera transition (69.4 → 28.9 rel z) | crouch/uncrouch chains | IMPLEMENTED AND TESTED | fix5, user video t≈36–40 s |
| Health/damage/death (100 HP, 25/entry, clamp 0, OnHealthChanged/OnDeath, DisableMovement) | `BPC_Health` + AnyDamage + HandleDeath | IMPLEMENTED AND TESTED | items 19–25 |
| Restart flow (R gated on bIsDead → RestartLevel) | IA_Restart chain | IMPLEMENTED AND TESTED | item 26 |
| Interaction core (250 cm camera trace → BPI_Interactable → Interact) | `BPI_Interactable`, `BP_TestInteractable` | IMPLEMENTED AND TESTED | items 16–18 + trace fix (205.4 cm hit) |
| Damage volume trigger | `BP_TestDamageVolume` | IMPLEMENTED AND TESTED | walk-in 100→75→50→25→0 |
| Debug HUD (7 DisplayAll lines: sprint/crouch/HP/dead/MWS/velocity/capsule) | DisplayAll chains on free pin | IMPLEMENTED AND TESTED | all user videos |
| Diagnostics prints in series, silencible via `DisableAllScreenMessages` | BP chains | IMPLEMENTED AND TESTED | final session |
| GameMode + test map (walls, crouch tunnel, cube, volume, lighting) | `BP_GM_FoundationTest`, `L_Test_PlayerFoundation` | IMPLEMENTED AND TESTED | items 1–3, 12 |
| Enhanced Input framework | 8 IA_*, `IMC_Default` (16 mappings), `IMC_MouseLook` | IMPLEMENTED AND TESTED (K&M) | item 28, fresh-session verify |
| Body animation (idle sway, 8-dir walk/jog blendspaces, jump/fall/land states) | `ABP_Unarmed` on `SKM_Manny_Simple` | PARTIALLY IMPLEMENTED | bone-delta verification; **no crouch anim** (documented placeholder); death anims unwired |
| PIE evidence harness (slate post-tick sampling → JSON) | documented methodology + scripts | PARTIALLY IMPLEMENTED | not packaged as a repeatable project test |

### 1.3 Known content facts relevant to the register
- **No weapon assets exist** (no meshes, no FP arms, no sockets) — the entire Category C/D/E stack starts from zero. The UE 5.8 FirstPerson template folder contains only `BP_FirstPersonCharacter` etc. + the EMPTY `ABP_FP_Copy` + unwired `CtrlRig_FPWarp`.
- **The full UE 5.8 Muse mannequin animation library ships in `Content/Characters/Mannequins/Anims/`** and is reusable as graybox placeholders: Rifle (fire/reload/dry-fire/equip/ADS idle + 8-dir walk/jog + jump set + 13 HitReact variants + `AO_Rifle` aim offsets), Pistol (same family + fire montage + `AO_Pistol`), Death (6 directional variants), Unarmed (ABP + attacks `MM_Attack_01–03`/charged). Meshes: `SKM_Manny_Simple`, `SKM_Quinn_Simple`; rigs `CR_Mannequin_Body/FootIK/Procedural`, pose asset `PA_Mannequin`. **These are third-person full-body clips — they do not animate first-person arms directly** (see Part 7).
- `DemoTemplate/_Core/BP_SaveData.uasset` (unwired template save object) exists as a reference for Category N.
- Template door assets exist (`BP_DoorFrame_Unlockable`, `BP_DoorTrigger`) — unwired demo content.
- Empty folders remain in `Content/AfghanFPS/Interaction/` (Door/JumpPad/Materials/Test — origin unclear, known issue #4).
- `Lvl_IntroRoom` still contains the crashing demo `ABP_Copycat` — unused, but must never be set as startup map again.
- Automation constraints carried forward (shape QA design, Part 10): CUA key injection into PIE is conclusively blocked → physical key passes stay with the owner; editor must be fully restarted after agent-side changes; only one editor instance at a time; `map_key` does not persist (use `set_editor_property("mappings", …)`); exec-out pins hold exactly one wire; `set_pin_value` needs forced dirty save.

### 1.4 Statuses carried into the register
Open items from Phase 1: physical re-verification of crouch camera view (owner), optional
diagnostic silencing, optional restore-test cleanup. None block the register.

---

## PART 2 — REFERENCE VIDEO ACTION INVENTORY

Observed features mapped to register action IDs (the video is the quality bar for these):

| # | Observed in reference | Register actions |
|---|---|---|
| 1 | First-person weapon and arms | B-1, B-7, C-1, C-2 |
| 2 | Rifle ADS | E-1, B-6, K-8, E-4, E-6 |
| 3 | Weapon firing and recoil | D-1, D-3, E-2, E-3, K-5 |
| 4 | Muzzle effects | L-1 |
| 5 | Cartridge ejection | L-2 |
| 6 | Reload animation | C-3, L-6 |
| 7 | Pistol handling | C-1/C-3/C-8 (pistol variant), C-6, C-7 |
| 8 | Weapon switching | C-6, C-5 |
| 9 | Shooting at enemy mannequins | G-1, G-2, D-4 |
| 10 | Enemy hit reactions | F-2, G-7 |
| 11 | Enemy deaths | F-3, G-8 |
| 12 | Blood and impact effects | L-3, L-4 |
| 13 | Close-range grabbing/takedown actions | H-1, H-2, H-3 |
| 14 | First-person hand interactions | H-3, B-8 |
| 15 | Grenade-like object during close interaction | I-1, I-2, J-4 (pickup flow) |
| 16 | Combat around simple obstacles | J-3, A-8 (existing graybox geometry) |

---

## PART 3 — ADDITIONAL RECOMMENDED ACTIONS (beyond the reference video)

Added because they are required for a realistic military FPS or a shippable game, even
though the graybox reference does not demonstrate them:

| Action | Why added | Priority |
|---|---|---|
| A-5 Prone | Core military-stance triad (stand/crouch/prone) for the intended game | P2 |
| A-6 Lean/peek | Standard cover-combat skill; pairs with J-3 obstacles | P2 |
| A-9 Fall damage | Consistency of the damage system across the world | P2 |
| A-10 Foot IK / step-up | Stairs/rubble are everywhere in Afghan terrain graybox | P2 |
| C-4 Dry fire | Empty-weapon feedback loop closes the ammo loop | P1 |
| D-2 Fire modes | AK-pattern weapons imply auto/semi; historical authenticity | P1 |
| F-7 Damage typing (headshot/limb multipliers) | Expected combat depth; cheap on top of D-4 | P1 |
| H-2 Quick melee | Melee fallback when dry; uses existing Unarmed attack anims | P1 |
| J-2 Doors | Any interior mission space needs them; template reference exists | P1 |
| J-4 Pickups | Ammo/health economy for combat pacing | P1 |
| K-6 Damage feedback (flash/vignette) | Player must perceive being hit without looking at HUD | P2 |
| K-7 Death camera | Closes the death flow visually | P2 |
| L-7 Footsteps | Core presence/immersion; cheap (AudioComponent + surface type) | P1 |
| M-8 Hitmarker | Instant hit confirmation; supports aiming training | P1 |
| N-2 Save/checkpoint | Any mission structure needs persistence | P2 |
| N-3/N-4 Game state + pause | Shippable game shell | P2 |
| O-2/O-3 Rebinding + sensitivity options | Input customization baseline (owner asked for this category) | P2 |
| P-3..P-6 Performance/QA items | 16 GB RAM machine; quality assurance category | P1–P2 |

---

## PART 4 — COMPLETE IMPLEMENTATION GAP ANALYSIS

Format for every action card (STEP 3's 16 fields):
**ID · name · category · priority · status · complexity · observed-in-video** in the header line;
then description / required systems / animation / audio-VFX / input / dependencies /
acceptance test / PASS evidence.

Statuses: `I&T` = IMPLEMENTED AND TESTED · `IBU` = IMPLEMENTED BUT UNVERIFIED ·
`PARTIAL` = PARTIALLY IMPLEMENTED · `NI` = NOT IMPLEMENTED · `BLOCKED` (none currently).

### Category A — Player locomotion & stance (10 actions)

#### A-1 — Walk (WASD)
**P0 · I&T · Low · Video: (implicit baseline)**
- Description: 8-direction ground movement at MaxWalkSpeed 450.
- Systems: existing CharacterMovement + IA_Move chain. Anim: ABP_Unarmed walk blendspaces (verified).
- Audio/VFX: footsteps missing (L-7). Input: WASD → IA_Move (exists).
- Dependencies: none. Acceptance: hold W, HUD velocity ≈ 450 cm/s.
- PASS evidence: Phase 1 items 4/11 (USER + PIE measurement 450 cm/s).

#### A-2 — Sprint
**P0 · I&T · Low · Video: (implicit)**
- Description: Shift-held sprint 700 cm/s, guards vs dead/crouch, restores on release.
- Systems: existing sprint chain. Anim: jog blendspace (documented as sprint placeholder).
- Audio/VFX: faster footsteps (L-7). Input: LeftShift → IA_Sprint (exists).
- Dependencies: A-1. Acceptance: MWS 700 while held, 450 after release.
- PASS evidence: Phase 1 items 8/9 (USER + PIE).

#### A-3 — Crouch (hold)
**P0 · I&T · Low · Video: (implicit)**
- Description: Ctrl-hold crouch; capsule 96→40; cancels sprint; camera lowers.
- Systems: existing crouch chains. Anim: **no crouch anim — documented placeholder (B-4)**.
- Audio/VFX: none required at graybox. Input: LeftCtrl → IA_Crouch (exists).
- Dependencies: none. Acceptance: capsule readout 96↔40; view lowers/restores.
- PASS evidence: Phase 1 items 10/11 + fix5 camera transition (USER + PIE).

#### A-4 — Jump
**P0 · I&T · Low · Video: (implicit)**
- Description: Space jump, JumpZ 420, land state in ABP_Unarmed.
- Systems: existing Jump/StopJumping chains. Anim: MM_Jump/MM_Fall_Loop/MM_Land (verified by bone deltas).
- Audio/VFX: landing thud optional (P2). Input: Space → IA_Jump (exists).
- Dependencies: none. Acceptance: rise ≈ 89 cm, clean landing, anim states play.
- PASS evidence: Phase 1 items 6/7 + anim bone deltas.

#### A-5 — Prone
**P2 · NI · Medium · Video: no**
- Description: prone stance (capsule ~35 cm, prone speed), entered via Z (proposed).
- Systems: stance state machine on BP player; MaxWalkSpeedProne; custom capsule interp.
- Anim: placeholder = crouch capsule + camera floor-level; real prone anim later art task.
- Audio/VFX: none. Input: new IA_Prone. Dependencies: stance refactor (see B-9).
- Acceptance: capsule shrinks to prone height, cannot stand under 50 cm ceiling, blocked while sprinting.
- PASS evidence: [PIE] capsule height + speed transitions + [USER] key test.

#### A-6 — Lean/peek
**P2 · NI · Medium · Video: no**
- Description: Q/E lateral lean ±30° with camera roll, blocked while sprinting.
- Systems: lean state on player; camera boom lateral offset (interp); stance guard.
- Anim: none at graybox (camera-only). Audio/VFX: none.
- Input: IA_LeanLeft/Right (Q/E). Dependencies: K-1 camera rig.
- Acceptance: camera offsets ±30°, returns to zero on release, blocked in narrow space.
- PASS evidence: [PIE] camera relative offset sampled; [USER] Q/E pass.

#### A-7 — Landing (anim + feedback)
**P2 · PARTIAL · Low · Video: no**
- Description: landing already has anim state; missing camera dip + thud.
- Systems: reuse ABP land state; add camera dip (optional later). Anim: MM_Land (exists).
- Audio/VFX: thud (optional). Input: none. Dependencies: none beyond A-4.
- Acceptance: land state plays from ≥ 300 cm falls. PASS evidence: [PIE] anim state transition.

#### A-8 — Low-clearance stance blocking
**P0 · I&T · Low · Video: no (graybox geometry)**
- Description: engine capsule blocking under 100 cm tunnel; UnCrouch clearance check.
- Systems: engine behavior, verified. Acceptance: standing blocked, crouch passes.
- PASS evidence: Phase 1 item 12 (STRUCT + tunnel geometry).

#### A-9 — Fall damage
**P2 · NI · Low-Medium · Video: no**
- Description: damage on high-impact landings (threshold velocity).
- Systems: land-velocity check → BPC_Health.ApplyDamage. Anim: none.
- Audio/VFX: impact grunt optional. Input: none. Dependencies: F-1 (exists), A-4.
- Acceptance: falling ≥ 700 cm deals scaled damage; 0 from jump height. PASS evidence: [PIE] damage events at thresholds.

#### A-10 — Foot IK / step-up
**P2 · NI · Medium · Video: no**
- Description: feet conform to slopes/stairs; step-up over small ledges.
- Systems: `CR_Mannequin_FootIK` rig (asset exists, unwired) + CharacterMovement step settings.
- Anim: rig asset exists. Audio/VFX: none. Input: none. Dependencies: body anim system.
- Acceptance: feet planted on 30° slope; steps up 40 cm ledges. PASS evidence: [PIE] foot bone heights sampled.

### Category B — First-person arms & animation (9 actions)

#### B-1 — First-person arms/body visibility (weapon and arms in view)
**P0 · NI · Medium-High · Video: YES (feature 1)**
- Description: visible FP arms holding the weapon in camera; currently the mannequin body is hidden (OwnerNoSee workaround) and nothing renders in hands.
- Systems: FP arms mesh (graybox: simple blocky arms or mannequin upper-body) attached to camera/hand sockets; OwnerNoSee policy for full body.
- Anim: placeholder static grip pose (posed via socket) before real FP anims.
- Audio/VFX: none. Input: none.
- Dependencies: C-1 (weapon mesh), B-7 (sockets).
- Acceptance: arms + weapon visible in all stances (stand/crouch/jump), no clipping through walls at graybox level.
- PASS evidence: [USER] video shows arms+weapon; [PIE] screen capture.

#### B-2 — Idle sway/breathing
**P0 · PARTIAL · Low · Video: YES (implicitly)**
- Description: subtle weapon/arms motion at rest. Body idle sway verified; no weapon-level sway.
- Systems: anim layer or sine-driven weapon offset. Anim: body sway exists (ABP_Unarmed).
- Audio/VFX: none. Input: none. Dependencies: B-1.
- Acceptance: visible subtle motion while standing still. PASS evidence: [USER] visual.

#### B-3 — Walk/sprint arm motion
**P0 · PARTIAL · Medium · Video: YES (implicitly)**
- Description: arms/weapon bob and lag with locomotion. Body locomotion verified; weapon bob absent.
- Systems: weapon bob (spring/curve driven) + anim layer. Anim: body walk/jog exists.
- Audio/VFX: none. Input: none. Dependencies: B-1, A-1/A-2.
- Acceptance: weapon bob at walk, stronger at sprint, zero when still. PASS evidence: [USER] visual + [PIE] offset sampling.

#### B-4 — Crouch arms animation
**P1 · NI · Low · Video: no**
- Description: arms adapt to crouch stance (known Phase 1 placeholder).
- Systems: stance-driven anim state. Anim: placeholder = lower camera + static pose; real anim later.
- Audio/VFX: none. Input: none. Dependencies: B-1, A-3.
- Acceptance: no visible popping when toggling crouch. PASS evidence: [USER] visual.

#### B-5 — Jump/fall/land arm motion
**P1 · PARTIAL · Medium · Video: no**
- Description: arms track body jump states. Body states verified; arms not present yet.
- Systems: anim state follow. Anim: MM_Jump/Fall/Land exist (body). Input: none.
- Audio/VFX: none. Dependencies: B-1.
- Acceptance: arms do not detach/clip during full jump cycle. PASS evidence: [USER] visual.

#### B-6 — Aim-offset look system
**P0 · NI · Medium · Video: YES (ADS implies)**
- Description: arms/weapon track pitch/yaw aim.
- Systems: AimOffset node + `AO_Rifle`/`AO_Pistol` + AO_CU/CD/CC assets (exist, unwired).
- Anim: aim-offset assets exist (body-level; FP-arm adaptation in Part 7). Input: mouse (exists).
- Audio/VFX: none. Dependencies: B-1, C-1.
- Acceptance: weapon points where camera looks across ±90° pitch. PASS evidence: [USER] visual sweep.

#### B-7 — Weapon attach sockets (hand + muzzle)
**P0 · NI · Medium · Video: YES (feature 1)**
- Description: weapon parented to hand socket; muzzle socket for FX/traces.
- Systems: socket setup on arms/weapon; attachment node chain; muzzle socket convention `Muzzle`.
- Anim: n/a. Audio/VFX: n/a (enables L-1/L-2). Input: none. Dependencies: C-1.
- Acceptance: weapon follows arm through all anims without offset drift. PASS evidence: [PIE] socket transform samples.

#### B-8 — Left-hand IK on weapon
**P1 · NI · Medium-High · Video: YES (feature 14)**
- Description: left hand planted on handguard during idle/ADS; animates during reload.
- Systems: FBIK/TwoBoneIK node on left hand → weapon grip target; `CtrlRig_FPWarp` reference exists unwired.
- Anim: IK chain. Audio/VFX: none. Input: none. Dependencies: B-7, B-6.
- Acceptance: hand stays on handguard in idle/walk/ADS; releases correctly during reload. PASS evidence: [USER] visual.

#### B-9 — Stance-state upper-body layering
**P1 · NI · High · Video: (implicit)**
- Description: locomotion lower body + aim/weapon upper body layered by stance/state.
- Systems: Layered blend per bone; state enum (Hip/ADS/Reload/Sprint) shared with weapon system.
- Anim: layering graph. Audio/VFX: none. Input: none. Dependencies: B-3, B-6, C-3.
- Acceptance: sprint + turn + aim transitions without pose snapping. PASS evidence: [USER] visual.

### Category C — Weapon handling (10 actions)

#### C-1 — Weapon render/equip (FP weapon + arms)
**P0 · NI · Medium · Video: YES (features 1, 7)**
- Description: graybox rifle (and pistol) rendered in hands; equip state on spawn.
- Systems: `BP_Weapon_Base` (proposed): StaticMesh + sockets + owner; weapon manager on player.
- Anim: static grip (graybox). Audio/VFX: equip sound (L-6, placeholder). Input: none (auto-equip).
- Dependencies: B-7. Acceptance: rifle visible from spawn in correct pose.
- PASS evidence: [PIE] screen capture + [USER] visual.

#### C-2 — Hip idle pose
**P0 · NI · Low · Video: YES (feature 1)**
- Description: neutral carry pose while not aiming. Systems: idle anim/pose.
- Anim: static pose at graybox. Audio/VFX: none. Input: none.
- Dependencies: C-1, B-2. Acceptance: distinct hip vs ADS poses. PASS evidence: [USER] visual.

#### C-3 — Reload
**P0 · NI · Medium · Video: YES (feature 6)**
- Description: timed reload: mag swap, reserve decrement, fire blocked during reload.
- Systems: reload state on weapon (timer + can-fire gate); ammo transaction.
- Anim: **`MM_Rifle_Reload` / `MM_Pistol_Reload` assets exist** (body-level placeholders; Part 7 note).
- Audio/VFX: mag-out/in foley placeholders (L-6). Input: IA_Reload (R — proposed remap; R currently = restart, see N-1 note).
- Dependencies: C-8, D-8, B-9. Acceptance: reload refills mag from reserve, blocked when full, correct duration.
- PASS evidence: [PIE] ammo counters across reload + [USER] key pass.

#### C-4 — Dry fire (empty click)
**P1 · NI · Low · Video: YES (implied by pistol handling)**
- Description: trigger pull on empty mag → click sound, no shot, prompts reload.
- Systems: fire gate branch. Anim: MM_*_DryFire assets exist. Audio/VFX: click SFX placeholder.
- Input: same fire key. Dependencies: D-1, D-8. Acceptance: empty weapon fires no shot, plays click.
- PASS evidence: [PIE] no damage trace fired + [USER] audio-visual.

#### C-5 — Equip/holster (draw) transition
**P1 · NI · Medium · Video: YES (feature 8, switching implies)**
- Description: timed draw/holster when switching weapons; fire blocked during transition.
- Systems: equip state + timer. Anim: MM_*_Equip assets exist. Audio/VFX: draw foley placeholder.
- Input: part of switch. Dependencies: C-6, B-9. Acceptance: switch shows draw time; fire gated.
- PASS evidence: [PIE] state timing + [USER] visual.

#### C-6 — Weapon switching (1/2 keys, scroll)
**P0 · NI · Medium · Video: YES (feature 8)**
- Description: two-slot switching (rifle/pistol at graybox) with equip transition.
- Systems: weapon manager (array of 2), switch input, C-5 state.
- Anim: equip anims exist. Audio/VFX: swap foley. Input: IA_NextWeapon (1/2/scroll).
- Dependencies: C-1, C-5, C-7 (minimal slots). Acceptance: switch < 0.8 s, HUD reflects active weapon.
- PASS evidence: [PIE] active-weapon flag + [USER] key pass.

#### C-7 — Weapon loadout/slot system
**P1 · NI · Medium · Video: YES (implied)**
- Description: data-driven loadout (2 slots graybox → expandable): weapon class, ammo type, reserve.
- Systems: struct array + save-friendly data. Anim: n/a. Audio/VFX: n/a. Input: none.
- Dependencies: C-6, D-8. Acceptance: adding a third weapon requires no logic rewrite (data only).
- PASS evidence: [STRUCT] config-driven switch test with 3 weapons.

#### C-8 — Ammo state tracking (mag/reserve per weapon)
**P0 · NI · Low-Medium · Video: YES (reload implies)**
- Description: per-weapon mag + reserve counts,HUD-visible.
- Systems: weapon ammo struct + change dispatchers (mirror BPC_Health pattern).
- Anim: n/a. Audio/VFX: n/a. Input: none. Dependencies: C-1.
- Acceptance: HUD shows mag/reserve; decrements on fire; reload transactions correct.
- PASS evidence: [PIE] counter states across fire/reload.

#### C-9 — World weapon pickup/drop
**P2 · NI · Medium · Video: no (pickup of grenade-like object observed → J-4)**
- Description: weapons as world items; swap on pickup.
- Systems: world-item actor + interact interface reuse (BPI_Interactable exists).
- Anim: n/a. Audio/VFX: pickup foley. Input: E (exists). Dependencies: J-1, C-7.
- Acceptance: pickup swaps slots; dropped weapon stays with state. PASS evidence: [PIE] item swap.

#### C-10 — Weapon inspection
**P3 · NI · Low · Video: no**
- Description: optional idle check animation (immersion flavor). Systems: one anim state + input.
- Anim: later art. Audio/VFX: none. Input: IA_Inspect (F, proposed). Dependencies: B-9.
- Acceptance: plays once, cancellable by fire/ADS. PASS evidence: [USER] visual.

### Category D — Firearms & ammunition (9 actions)

#### D-1 — Fire (hitscan rifle)
**P0 · NI · Medium · Video: YES (feature 3)**
- Description: LMB fires: camera-center hitscan trace, damage application to hit actor, muzzle FX, shell, sound, recoil.
- Systems: fire function on `BP_Weapon_Base` (trace ByChannel Bullet; ignore owner), damage via F-7 typing → BPC_Health on targets.
- Anim: fire anim exists (body). Audio/VFX: L-1/L-2/L-5 placeholders. Input: IA_Fire (LMB, hold = auto).
- Dependencies: C-1, C-8, B-7, F-2 (for reaction), D-3.
- Acceptance: trace hits dummy at N m; dummy HP drops by damage; all feedback fires same frame.
- PASS evidence: [PIE] damage values + hit actor + [USER] visual/audio pass.

#### D-2 — Fire modes (auto/semi/burst)
**P1 · NI · Low-Medium · Video: YES (rifle behavior)**
- Description: per-weapon fire mode; graybox = rifle auto + pistol semi.
- Systems: mode enum + auto loop (timer) vs per-click. Anim: n/a. Audio/VFX: none.
- Input: LMB (mode from weapon data). Dependencies: D-1, D-3.
- Acceptance: auto fires while held at ROF; semi fires once per press; no double-fire. PASS evidence: [PIE] shot timestamps.

#### D-3 — Trigger/ROF logic
**P0 · NI · Low · Video: YES (feature 3)**
- Description: rate-of-fire enforcement (e.g., 700 RPM rifle), no shot spam via input.
- Systems: fire timer/cooldown on weapon. Anim: n/a. Audio/VFX: n/a.
- Input: LMB. Dependencies: D-1. Acceptance: measured ROF ≈ spec ±5%.
- PASS evidence: [PIE] shot interval sampling.

#### D-4 — Spread & ballistics model (hitscan)
**P0 · NI · Medium · Video: YES (feature 3)**
- Description: trace deviation from aim center by state (see E-3); simple hitscan (instant).
- Systems: cone-spread generator; optional trace visualization for QA.
- Anim: n/a. Audio/VFX: none. Input: none. Dependencies: D-1, E-3.
- Acceptance: standing hip spread ≈ spec; ADS ≈ near-zero; crouch tighter. PASS evidence: [PIE] hit-point scatter vs target rings.

#### D-5 — Projectile option (for launchers later)
**P3 · NI · High · Video: no**
- Description: projectile-based firing path (grenade launcher, RPG later). Systems: projectile actor + movement.
- Anim: n/a. Audio/VFX: none. Input: n/a. Dependencies: I-3 shares tech.
- Acceptance: projectile travels with gravity, damages on impact. PASS evidence: [PIE] trajectory.

#### D-6 — Damage falloff / limb multipliers
**P2 · NI · Low-Medium · Video: no**
- Description: damage scales by distance + hit zone (head/chest/limb).
- Systems: bone-name lookup + falloff curve. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: D-1, F-7. Acceptance: headshot ×2.5, falloff ≥ 50 m. PASS evidence: [PIE] damage by zone/distance.

#### D-7 — Surface penetration
**P3 · NI · High · Video: no**
- Description: bullets pass thin covers with damage reduction. Systems: multi-trace penetration.
- Anim: n/a. Audio/VFX: exit-hole decal. Input: none. Dependencies: D-1.
- Acceptance: 10 cm graybox wall lets bullets through at reduced damage. PASS evidence: [PIE] dual-hit trace.

#### D-8 — Ammo depletion & tracking
**P0 · NI · Low · Video: YES (reload implies)**
- Description: shots consume mag; depletion forces dry state; reserve economy.
- Systems: ammo transactions in fire/reload (C-8). Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: C-8, D-1. Acceptance: mag 30 → 0 via 30 shots; dry gate active. PASS evidence: [PIE] counters.

#### D-9 — Fire gating (state machine correctness)
**P0 · NI · Low-Medium · Video: YES (implicitly)**
- Description: cannot fire while dead/reloading/switching/inspecting; sprint may lower weapon (design choice).
- Systems: central weapon state enum checked by fire gate. Anim: n/a. Audio/VFX: n/a. Input: n/a.
- Dependencies: D-1, C-3, C-5, F-1. Acceptance: no shot fires in any blocked state; resumes after.
- PASS evidence: [PIE] state-gate matrix test.

### Category E — Aiming & recoil (7 actions)

#### E-1 — ADS (aim down sights)
**P0 · NI · Medium-High · Video: YES (feature 2)**
- Description: RMB hold: weapon to eye, FOV/camera shift, reduced spread, sensitivity scale.
- Systems: ADS state on weapon+player; camera FOV interp; weapon transform target poses (hip↔ADS).
- Anim: `AO_Rifle`/ADS idle assets exist (body-level). Audio/VFX: ADS foley optional.
- Input: IA_ADS (RMB hold). Dependencies: C-1, B-7, B-6, K-8, E-4, E-6, E-5.
- Acceptance: smooth hip↔ADS ≤ 0.25 s, crosshair hidden, spread near-zero, no pose snapping.
- PASS evidence: [PIE] FOV + weapon transform samples + [USER] visual.

#### E-2 — Recoil (pattern + kick)
**P0 · NI · Medium · Video: YES (feature 3)**
- Description: per-shot camera pitch/yaw pattern + weapon kick-back, recovery over time.
- Systems: recoil data table (per weapon); camera add-rotation with recovery; weapon spring kick.
- Anim: weapon anim kick. Audio/VFX: none. Input: none. Dependencies: D-1, K-5.
- Acceptance: sustained fire climbs then recovers; pattern reproducible; ADS reduces climb.
- PASS evidence: [PIE] control rotation samples across a 10-shot burst.

#### E-3 — Dynamic spread by state
**P0 · NI · Low-Medium · Video: YES (implicitly)**
- Description: spread values for stand/walk/sprint/crouch/jump/ADS.
- Systems: spread table + state lookup. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: D-4. Acceptance: sprint ≫ walk > stand > crouch ≈ ADS. PASS evidence: [PIE] scatter measurement per state.

#### E-4 — ADS sensitivity scaling
**P1 · NI · Low · Video: YES (ADS feel)**
- Description: look sensitivity scales (e.g., ×0.6) while ADS, proportional to FOV.
- Systems: look-input multiplier in camera chain. Anim: n/a. Audio/VFX: none. Input: exists.
- Dependencies: E-1. Acceptance: mouse feels slower & precise while ADS. PASS evidence: [USER] feel test.

#### E-5 — Crosshair + ADS behavior (merged HUD/aiming)
**P0 · NI · Low-Medium · Video: YES (aiming)**
- Description: hip crosshair (expands with spread); hidden at ADS.
- Systems: UMG crosshair widget (M-2). Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: M-2, E-3. Acceptance: crosshair grows while moving/sprinting, hides at ADS.
- PASS evidence: [USER] visual + [PIE] widget state flags.

#### E-6 — ADS FOV/zoom interpolation
**P1 · NI · Low-Medium · Video: YES (feature 2)**
- Description: smooth FOV 90 → e.g. 65 while ADS.
- Systems: camera FOV interp on ADS state. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: E-1. Acceptance: no FOV pop; interpolates over ADS time. PASS evidence: [PIE] FOV timeline.

#### E-7 — ADS sway
**P2 · NI · Low-Medium · Video: no**
- Description: subtle held-breath sway while aiming; optional breath-hold later.
- Systems: sine noise on camera/aim while ADS. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: E-1. Acceptance: visible but controllable drift; disabled option available. PASS evidence: [PIE] rotation noise sampling.

### Category F — Damage & hit reactions (8 actions)

#### F-1 — Player damage intake
**P0 · I&T · Low · Video: (baseline)**
- Description: AnyDamage → BPC_Health.ApplyDamage; dispatchers; death at 0; movement/interact gates.
- Systems: exists (`BPC_Health` 100/100, OnHealthChanged/OnDeath). Acceptance: existing 30/30 items 19–26.
- PASS evidence: Phase 1 (USER + PIE: 100→75→…→0→restart).

#### F-2 — Enemy hit reactions (anim/state)
**P0 · NI · Low-Medium · Video: YES (feature 10)**
- Description: damageable actor plays directional hit reaction (light/med/heavy × direction).
- Systems: hit-reaction montage/state on target BP + BPC_Health reuse; **13 `MM_HitReact_*` assets exist** (Lgt/Med/Hvy, Front/Back).
- Anim: assets exist (unwired). Audio/VFX: none. Input: none.
- Dependencies: G-1, G-2, D-1. Acceptance: frontal hit plays frontal reaction, no retrigger overlap.
- PASS evidence: [PIE] montage/state sampling + [USER] visual.

#### F-3 — Enemy death (anim/state)
**P0 · NI · Low-Medium · Video: YES (feature 11)**
- Description: 0 HP → directional death anim, collision to ragdoll-ish state, gates all AI behavior.
- Systems: death state on target BP; **6 `MM_Death_*` assets exist**; disable logic.
- Anim: assets exist (unwired). Audio/VFX: none (blood = L-3). Input: none.
- Dependencies: F-2, G-2, G-8. Acceptance: correct directional death per hit direction; one-shot only.
- PASS evidence: [PIE] state transition + [USER] visual.

#### F-4 — Player death visual
**P1 · PARTIAL · Low · Video: no**
- Description: death flow works (gates + restart); missing visual (camera drop/fade or body anim).
- Systems: death camera K-7 (proposed). Anim: MM_Death_* reusable later. Audio/VFX: none.
- Input: none. Dependencies: F-1, K-7. Acceptance: death visibly distinct from alive state.
- PASS evidence: [USER] visual of death presentation.

#### F-5 — Directional damage indicator
**P2 · NI · Low-Medium · Video: no**
- Description: HUD arc/arrow pointing to damage source.
- Systems: compute attacker direction on AnyDamage → widget. Anim: n/a.
- Audio/VFX: none. Input: none. Dependencies: M-2, F-1. Acceptance: indicator matches source side.
- PASS evidence: [PIE] angle math + [USER] visual.

#### F-6 — Floating damage numbers
**P3 · NI · Low · Video: no**
- Description: optional damage feedback at hit point. Systems: widget spawn at hit location.
- Anim: n/a. Audio/VFX: none. Input: none. Dependencies: D-1, M-2.
- Acceptance: number matches applied damage. PASS evidence: [PIE] value match.

#### F-7 — Damage typing (headshot/limb multipliers)
**P1 · NI · Low-Medium · Video: no**
- Description: damage struct (base, zone multiplier, falloff) replacing raw floats.
- Systems: damage struct + bone classification on hit. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: D-1, D-6. Acceptance: headshot mult applies; logs show typed damage. PASS evidence: [PIE] typed damage values.

#### F-8 — Health recovery (regen or pickups)
**P2 · NI · Low-Medium · Video: no**
- Description: graybox: health pickup (via J-4); regen deferred to design decision.
- Systems: heal function on BPC_Health (exists conceptually via Initialize/Reset) + pickup actor.
- Anim: n/a. Audio/VFX: pickup FX. Input: E. Dependencies: F-1, J-4.
- Acceptance: pickup restores to 100, capped. PASS evidence: [PIE] HP curve.

### Category G — Enemy AI & combat behavior (10 actions)

#### G-1 — Static target dummies (spawnable enemy BP)
**P0 · NI · Low-Medium · Video: YES (feature 9)**
- Description: `BP_TargetDummy` (mannequin `SKM_Manny_Simple`/`Quinn` + `ABP_Unarmed`) standing targets, damageable, killable.
- Systems: Character or pawn BP + AnimClass assignment + capsule collision + BPC_Health.
- Anim: ABP_Unarmed idle (verified working on player — reuse). Audio/VFX: none. Input: none.
- Dependencies: G-2. Acceptance: dummies spawn, stand with idle anim, take damage, die with F-3.
- PASS evidence: [PIE] spawn + damage + death chain.

#### G-2 — Enemy health wiring (BPC_Health reuse)
**P0 · NI · Low · Video: YES (features 9–11)**
- Description: attach existing `BPC_Health` to dummies; death dispatcher → F-3.
- Systems: component add + dispatcher binding in dummy BP. Anim: n/a. Audio/VFX: none.
- Input: none. Dependencies: G-1, F-1 (component exists & proven).
- Acceptance: dummy HP 100, dies at 0 exactly once. PASS evidence: [PIE] HP curve per dummy.

#### G-3 — Perception (sight)
**P1 · NI · Medium · Video: no (dummies are static)**
- Description: AI sight sense (cone + LOS) detecting player.
- Systems: AIPerception component (sight config) on AI controller.
- Anim: n/a. Audio/VFX: alert sound optional. Input: none. Dependencies: G-4 (controller).
- Acceptance: detection at spec range/FOV; broken by walls; debug draws verified.
- PASS evidence: [PIE] perception-state sampling with/without LOS.

#### G-4 — Patrol/idle behaviors
**P2 · NI · Medium · Video: no**
- Description: patrol route walking + idle pauses before alert.
- Systems: Behavior Tree or simple state machine + patrol splines/points.
- Anim: walk blendspaces exist. Audio/VFX: footsteps optional. Input: none.
- Dependencies: G-9. Acceptance: loops route at walk speed; pauses at points. PASS evidence: [PIE] location sampling.

#### G-5 — Engage & fire at player
**P1 · NI · Medium-High · Video: no**
- Description: alerted AI faces player and fires (hitscan, simplified) with cooldowns/accuracy.
- Systems: AI fire logic + player damage intake (F-1 exists) + line-of-sight check.
- Anim: `MM_Rifle_Fire` etc. available. Audio/VFX: enemy gunshot. Input: none.
- Dependencies: G-3, F-1. Acceptance: player takes spec damage per hit at spec cadence; takes cover → fire pauses.
- PASS evidence: [PIE] damage events + [USER] survival pass.

#### G-6 — Cover usage
**P2 · NI · High · Video: no**
- Description: AI selects cover points when damaged/alerted.
- Systems: cover-point actors + EQS queries. Anim: crouch anims exist. Audio/VFX: none.
- Input: none. Dependencies: G-5, G-9. Acceptance: AI moves to nearest valid cover under fire. PASS evidence: [PIE] cover occupancy.

#### G-7 — Flinch/stagger AI integration
**P1 · NI · Low-Medium · Video: YES (feature 10)**
- Description: hits interrupt AI actions briefly (aim/shot delay), scaled by damage.
- Systems: on-damage interrupt hooks in AI logic. Anim: F-2 reactions. Audio/VFX: none.
- Input: none. Dependencies: F-2, G-5. Acceptance: shot cadence visibly disrupted by hits. PASS evidence: [PIE] interrupt timing.

#### G-8 — Death handling & despawn/respawn
**P1 · NI · Low-Medium · Video: YES (feature 11)**
- Description: dead AI leaves combat (colliders off/state), optional respawn timer for graybox drills.
- Systems: death state hooks + respawn actor. Anim: F-3. Audio/VFX: none. Input: none.
- Dependencies: F-3, G-2. Acceptance: dead AI takes no damage, blocks nothing, respawns per spec. PASS evidence: [PIE] state lifecycle.

#### G-9 — NavMesh movement (patrol/chase)
**P1 · NI · Medium · Video: no**
- Description: navmesh on test map; AI walk/chase via AI MoveTo.
- Systems: NavMeshBoundsVolume on map + AIController + MoveTo. Anim: walk blendspaces exist.
- Audio/VFX: none. Input: none. Dependencies: G-1 (controller). Acceptance: AI paths around the interact cube without getting stuck.
- PASS evidence: [PIE] path sampling around obstacle.

#### G-10 — Alertness propagation
**P2 · NI · Medium · Video: no**
- Description: shooting sound alerts nearby AI (heard-sense); states idle→suspicious→combat.
- Systems: perception hearing + state enum. Anim: n/a. Audio/VFX: none. Input: none.
- Dependencies: G-3, G-5. Acceptance: unsuppressed shot alerts AI within radius. PASS evidence: [PIE] state transitions on noise events.

### Category H — Melee & close-quarters interactions (4 actions)

#### H-1 — Contextual takedown/grab
**P1 · NI · High · Video: YES (feature 13)**
- Description: prompt behind/near unaware enemy → grab-takedown sequence (camera lock, enemy dispatch).
- Systems: interaction query (range + angle + enemy state) + scripted sequence (camera + anim montage + kill).
- Anim: placeholder = enemy collapse (MM_Death_*) + player hold pose; real paired anims are a later art task.
- Audio/VFX: takedown foley placeholder. Input: IA_Takedown (E-contextual or F, proposed).
- Dependencies: G-1/F-3, B-1, H-3 (optional polish).
- Acceptance: prompt appears in valid zone only; sequence locks input; enemy dies; player released.
- PASS evidence: [PIE] state machine trace + [USER] visual.

#### H-2 — Quick melee (rifle butt / bash)
**P1 · NI · Low-Medium · Video: YES (close-quarters actions)**
- Description: V (proposed) short-range melee shove: small damage + knockback.
- Systems: sphere trace + damage impulse; melee state gates fire briefly.
- Anim: **`MM_Attack_01–03` exist** (Unarmed library). Audio/VFX: whoosh/impact placeholders. Input: IA_Melee.
- Dependencies: D-9 (state gates), F-7. Acceptance: hit registers ≤ 150 cm; cooldown enforced; interrupts ADS.
- PASS evidence: [PIE] damage + cooldown sampling.

#### H-3 — First-person hand interaction (grab/pickup IK)
**P2 · NI · Medium-High · Video: YES (features 14, 15)**
- Description: hand reaches/grabs objects (pickups, grenade hold) with IK reach.
- Systems: FBIK reach to object socket; release return. Anim: IK-driven. Audio/VFX: grab foley.
- Input: E-contextual (exists). Dependencies: B-8, J-4.
- Acceptance: hand visibly reaches to pickup before item attaches; no freeze frames. PASS evidence: [USER] visual.

#### H-4 — Weapon/camera collision (warp IK)
**P2 · NI · Medium-High · Video: no**
- Description: weapon folds against walls when close (avoids clipping).
- Systems: trace from camera → weapon pose offset + CtrlRig warp (`CtrlRig_FPWarp` exists unwired).
- Anim: IK warp. Audio/VFX: none. Input: none. Dependencies: B-1, B-7.
- Acceptance: pressing against wall folds weapon, no clip-through. PASS evidence: [USER] visual against test wall.

### Category I — Throwable equipment (6 actions)

#### I-1 — Grenade selection/hold
**P1 · NI · Medium · Video: YES (feature 15, grenade-like object)**
- Description: G (proposed) selects grenade: lower weapon, grenade mesh in hand.
- Systems: equipment slot in loadout (C-7 struct); hold state.
- Anim: placeholder hold pose; body anims available. Audio/VFX: pin foley later. Input: IA_Grenade.
- Dependencies: C-7, B-9, H-3 (polish). Acceptance: weapon lowers, grenade visible in hand, cancels cleanly.
- PASS evidence: [PIE] state flags + [USER] visual.

#### I-2 — Throw (arc + release)
**P1 · NI · Medium · Video: YES (feature 15 implied)**
- Description: hold to charge, release to throw along camera arc; weapon re-equips.
- Systems: charge timer + throw strength curve; spawn projectile with initial velocity.
- Anim: throw pose placeholder. Audio/VFX: throw whoosh. Input: hold/release IA_Grenade.
- Dependencies: I-1, I-3. Acceptance: thrown trajectory matches charge; inventory decrements.
- PASS evidence: [PIE] projectile velocity + count.

#### I-3 — Grenade projectile + fuse
**P1 · NI · Medium · Video: YES (feature 15 implied)**
- Description: physics projectile with fuse timer and bounce.
- Systems: projectile actor (physics collision) + fuse timer + detonate.
- Anim: n/a. Audio/VFX: bounce clink (placeholder). Input: none.
- Dependencies: I-2. Acceptance: bounces believably, detonates at fuse regardless of motion.
- PASS evidence: [PIE] detonation timing.

#### I-4 — Explosion damage/radius
**P1 · NI · Medium · Video: no (fx quality bar)**
- Description: radial damage falloff to player AND dummies; kills within inner radius.
- Systems: ApplyDamage radial sweep + falloff curve; reuse BPC_Health on all victims.
- Anim: n/a. Audio/VFX: **explosion VFX/SFX (L-9)** placeholder required. Input: none.
- Dependencies: I-3, F-1, F-2. Acceptance: damage matches distance table; player can self-damage.
- PASS evidence: [PIE] damage vs distance samples.

#### I-5 — Cook/pin mechanics
**P3 · NI · Medium · Video: no**
- Description: hold to cook with fuse risk; design decision deferred. Systems: fuse transfer logic.
- Anim: hold pose. Audio/VFX: fuse hiss. Input: same grenade key. Dependencies: I-1..I-4.
- Acceptance: post-cook throw detonates in reduced time. PASS evidence: [PIE] fuse timing.

#### I-6 — Smoke/flash placeholder
**P3 · NI · Medium · Video: no**
- Description: equipment variety for graybox tactics. Systems: same chain, different payload (opacity volume/flash effect).
- Anim: n/a. Audio/VFX: smoke VFX placeholder. Input: same. Dependencies: I-4 chain.
- Acceptance: smoke blocks AI sight (G-3) — good AI validation tool. PASS evidence: [PIE] perception blocked in cloud.

### Category J — Environmental interactions (7 actions)

#### J-1 — Generic interact core (trace + interface + prompt)
**P0 · I&T · Low · Video: YES (feature 14 implicitly)**
- Description: exists: 250 cm camera trace → BPI_Interactable → Interact; scale-toggle verified.
- Extensions for F2: add prompt text + focus highlight (P1 polish under M-2).
- Acceptance: existing 30/30 items 16–18. PASS evidence: Phase 1 (trace hit 205.4 cm; toggle 1.0↔1.25).

#### J-2 — Doors
**P1 · NI · Medium · Video: no**
- Description: graybox door: open/close (interact or auto), blocking collision when shut.
- Systems: door BP (rotating/sliding) + optional lock state; template `BP_DoorFrame_Unlockable`/`BP_DoorTrigger` exist as reference demo content (do not copy blindly — review licensing/quality).
- Anim: n/a (mover). Audio/VFX: creak placeholder. Input: E. Dependencies: J-1.
- Acceptance: door opens/closes, blocks passage when closed, cannot crush player. PASS evidence: [PIE] collision state + [USER] pass.

#### J-3 — Cover objects (physical obstacles)
**P0 · PARTIAL · Low · Video: YES (feature 16)**
- Description: test map already has walls/cube/tunnel as blocking geometry (verified); a cover *system* (cover points for AI) is G-6.
- Acceptance: existing collision proof. PASS evidence: Phase 1 item 15.

#### J-4 — Pickups (ammo/health)
**P1 · NI · Medium · Video: YES (feature 15, grenade-like object pickup)**
- Description: world pickups: ammo box, health pack, grenade; float/rotate graybox presentation optional.
- Systems: pickup actor + BPI_Interactable + inventory transaction hooks (C-8/F-8).
- Anim: n/a. Audio/VFX: pickup SFX placeholder. Input: E. Dependencies: J-1, C-8, F-8.
- Acceptance: each pickup type applies correct transaction once, then despawns.
- PASS evidence: [PIE] inventory/HP deltas.

#### J-5 — Levers/switchables
**P3 · NI · Low · Video: no**
- Description: interact toggles lights/signals. Systems: switch actor + target list. Template `BP_Light` exists as demo reference. Input: E. Dependencies: J-1.
- Acceptance: lever toggles target state. PASS evidence: [PIE] state flip.

#### J-6 — Loot containers
**P3 · NI · Medium · Video: no**
- Description: containers granting random/designed loot. Systems: container actor + loot table struct.
- Input: E. Dependencies: J-4, C-7. Acceptance: yields table-defined items. PASS evidence: [PIE] granted items.

#### J-7 — Breakable objects
**P3 · NI · Medium · Video: no**
- Description: destructible graybox props. Systems: fracture/swap-mesh on damage.
- Input: none. Dependencies: D-1. Acceptance: breaks under spec damage, leaves debris collision-off. PASS evidence: [PIE] destruction event.

### Category K — Camera & movement feedback (8 actions)

#### K-1 — Camera rig & look
**P0 · I&T · Low · Video: (baseline)**
- Description: template camera + controller rotation; verified in videos. PASS evidence: Phase 1 items 5/14.

#### K-2 — Crouch camera transition
**P0 · I&T · Low · Video: YES (implicitly)**
- Description: camera lowers 69.4→28.9 and restores; verified. PASS evidence: fix5 + user video (t≈36–40 s).

#### K-3 — Landing camera dip
**P2 · NI · Low · Video: no**
- Description: small camera dip on landing. Systems: spring dip on land event. Dependencies: A-4.
- Acceptance: dip scaled by fall height, capped. PASS evidence: [PIE] camera z sampling.

#### K-4 — Sprint FOV kick
**P2 · NI · Low · Video: no**
- Description: +5–8 FOV while sprinting. Systems: FOV interp on sprint state. Dependencies: A-2.
- Acceptance: smooth in/out; none while ADS. PASS evidence: [PIE] FOV timeline.

#### K-5 — Fire camera shake
**P0 · NI · Low-Medium · Video: YES (feature 3)**
- Description: per-shot camera shake (small, per weapon). Systems: camera shake (legacy BP shake or shimmy on recoil E-2).
- Anim: n/a. Audio/VFX: n/a. Input: none. Dependencies: D-1, E-2.
- Acceptance: visible kick each shot, no motion sickness at 10-shot burst. PASS evidence: [USER] visual.

#### K-6 — Damage feedback (flash/vignette)
**P2 · NI · Low-Medium · Video: no**
- Description: red flash/vignette on taking damage + heal fade. Systems: PPV/material vignette bound to BPC_Health dispatcher.
- Audio/VFX: hurt grunt optional. Input: none. Dependencies: F-1, M-2.
- Acceptance: feedback fires per damage event; intensity scales with HP loss. PASS evidence: [PIE] effect state + [USER] visual.

#### K-7 — Death camera
**P2 · NI · Low-Medium · Video: no**
- Description: on death: camera drop/tilt + fade → restart prompt readable.
- Systems: death camera sequence on HandleDeath. Anim: n/a. Audio/VFX: death sting optional.
- Input: none. Dependencies: F-1, N-1. Acceptance: death presentation clear; R prompt visible.
- PASS evidence: [USER] visual of full death loop.

#### K-8 — ADS camera transition
**P1 · NI · Low-Medium · Video: YES (feature 2)**
- Description: camera position/FOV shift while ADS (see E-1/E-6). Dependencies: E-1.
- Acceptance: no pop; consistent shoulder. PASS evidence: [PIE] transform timeline.

### Category L — Audio & visual effects (10 actions)

#### L-1 — Muzzle flash
**P0 · NI · Low · Video: YES (feature 4)**
- Description: per-shot flash at muzzle socket + brief light. Systems: Niagara placeholder at `Muzzle` socket (B-7).
- Dependencies: B-7, D-1. Acceptance: flash every shot, none while dead/reloading. PASS evidence: [USER] visual + [PIE] emitter spawn count.

#### L-2 — Shell ejection
**P0 · NI · Low-Medium · Video: YES (feature 5)**
- Description: cartridge ejected right-down with physics despawn. Systems: Niagara or small physics mesh pool; eject socket.
- Dependencies: B-7, D-1. Acceptance: one shell per shot; no accumulation. PASS evidence: [PIE] spawn count = shot count.

#### L-3 — Blood effects on hit
**P0 · NI · Low-Medium · Video: YES (feature 12)**
- Description: hit-surface burst on flesh (dummy) hits; placeholder = red particle burst + simple decal (graybox-appropriate, not gore).
- Systems: Niagara placeholder + decal; routed by surface type (see L-4). Dependencies: D-1, G-1.
- Acceptance: flesh hits show blood-type feedback; environment hits do not. PASS evidence: [PIE] per-surface routing.

#### L-4 — Surface impact FX
**P0 · NI · Low-Medium · Video: YES (feature 12)**
- Description: per-surface impact particles + decal (concrete graybox only at first).
- Systems: physical surface lookup + FX table. Dependencies: D-1. Acceptance: correct FX per surface type. PASS evidence: [PIE] routing table test.

#### L-5 — Gunshot SFX
**P0 · NI · Low · Video: YES (feature 3)**
- Description: per-weapon shot sound with distance attenuation.
- Systems: SoundCue/MetaSound placeholder (DemoTemplate has MetaSound infrastructure to study) + attenuation.
- Dependencies: D-1. Acceptance: distinct rifle/pistol sounds; attenuates with distance. PASS evidence: [USER] audio pass.
- Note: no usable weapon audio ships with UE 5.8 — needs a placeholder pack or synthesized MetaSound; licensing check per AGENTS.md before any imported audio.

#### L-6 — Reload/equip foley
**P1 · NI · Low · Video: YES (feature 6)**
- Description: mag out/in + draw sounds timed to states. Systems: anim-notify-driven cues (placeholders).
- Dependencies: C-3, C-5. Acceptance: sounds align with anim moments. PASS evidence: [USER] audio pass.

#### L-7 — Footsteps
**P1 · NI · Low-Medium · Video: no**
- Description: footsteps by speed/surface; louder at sprint. Systems: notify or velocity-driven loop + surface type.
- Dependencies: A-1/A-2. Acceptance: cadence matches speed; silent when crouch-still. PASS evidence: [USER] audio pass.

#### L-8 — Ambient bed
**P3 · NI · Low · Video: no**
- Description: wind/room tone. Systems: looping ambient actor. Dependencies: none.
- Acceptance: constant subtle bed in test map. PASS evidence: [USER] audio.

#### L-9 — Explosion VFX/SFX
**P1 · NI · Medium · Video: no**
- Description: grenade detonation FX. Systems: Niagara burst + light flash + sound + camera shake.
- Dependencies: I-4, K-5. Acceptance: explosion visible/audible; shake scaled by distance. PASS evidence: [USER] audio-visual.

#### L-10 — UI/interact SFX
**P3 · NI · Low · Video: no**
- Description: click/hover/pickup UI sounds. Systems: widget sound hooks. Dependencies: M-2.
- Acceptance: feedback on interact. PASS evidence: [USER] audio.

### Category M — HUD & inventory (8 actions)

#### M-1 — Debug HUD (DisplayAll)
**P0 · I&T · Low · Video: (QA tool)**
- Description: 7 live value lines — the shared ground truth used across all Phase 1 verification. PASS evidence: all user videos. Keep during F2; silence via DisableAllScreenMessages.

#### M-2 — UMG HUD framework
**P0 · NI · Medium · Video: YES (implicit, any HUD)**
- Description: real HUD widget (health, ammo, crosshair, prompts, hitmarker anchor) replacing/augmenting debug lines.
- Systems: UMG widget + HUD BP + bindings to player dispatchers (OnHealthChanged etc.).
- Anim: n/a. Audio/VFX: L-10 optional. Input: none. Dependencies: C-8, F-1.
- Acceptance: HUD values track game state live; no frame hitch on update. PASS evidence: [PIE] binding values + [USER] visual.

#### M-3 — Health display
**P0 · PARTIAL · Low · Video: (baseline)**
- Description: exists as debug line; becomes real HUD element in M-2.
- Acceptance: HP reflects damage instantly. PASS evidence: Phase 1 videos (100→75→…→0).

#### M-4 — Ammo counter
**P0 · NI · Low · Video: YES (reload implies)**
- Description: mag/reserve display per active weapon. Systems: widget bound to C-8 dispatchers.
- Dependencies: M-2, C-8. Acceptance: live decrement on fire; reload refill shown. PASS evidence: [PIE] counters.

#### M-5 — Weapon select UI
**P2 · NI · Low-Medium · Video: YES (feature 8 implicitly)**
- Description: minimal active-weapon indicator (slot names, highlight). Systems: small widget bound to weapon manager.
- Dependencies: C-6, M-2. Acceptance: highlight switches with C-6. PASS evidence: [PIE] flag + [USER] visual.

#### M-6 — Inventory system (data layer)
**P2 · NI · Medium-High · Video: YES (implied by pickups/weapons)**
- Description: player inventory data (weapons, ammo pools, grenades) feeding HUD and pickups; save-ready struct.
- Systems: inventory component (data-first; UI later). Anim: n/a. Audio/VFX: n/a. Input: none.
- Dependencies: C-7, C-8, J-4. Acceptance: transactions add/remove/consume correctly; no duplicates.
- PASS evidence: [PIE] inventory state dumps.

#### M-7 — Hitmarker
**P1 · NI · Low · Video: YES (implicit hit feedback)**
- Description: hit confirmation marker (+ variant on kill). Systems: widget event from damage trace result.
- Dependencies: D-1, M-2. Acceptance: appears per confirmed hit, none on miss. PASS evidence: [PIE] event counts.

#### M-8 — Objective/mission display
**P3 · NI · Medium · Video: no**
- Description: campaign-era feature. Systems: objective data + widget. Dependencies: M-2, N-2.
- Acceptance: (campaign phase) current objective displayed. PASS evidence: [USER] visual.

### Category N — Save/load & gameplay states (5 actions)

#### N-1 — Restart-level flow
**P0 · I&T · Low · Video: (baseline)**
- Description: R-when-dead → RestartLevel → HP 100 alive; gated while alive. PASS evidence: Phase 1 item 26 + videos.
- Note: when R is assigned to Reload (C-3), restart must move (e.g., Enter or hold-R) — decision logged in Part 5.

#### N-2 — Save/checkpoint
**P2 · NI · Medium · Video: no**
- Description: graybox save: player transform, health, inventory, world pickups state.
- Systems: SaveGame object (`DemoTemplate/_Core/BP_SaveData` exists as unwired template reference) + save/load functions.
- Anim: n/a. Audio/VFX: n/a. Input: none (auto-checkpoint). Dependencies: M-6, N-3.
- Acceptance: save → quit → reopen → state restored. PASS evidence: [PIE] round-trip state compare.

#### N-3 — Game state machine
**P2 · NI · Medium · Video: no**
- Description: states Boot/Menu/Playing/Paused/Dead; clean transitions.
- Systems: GameState/PlayerController state enum + events. Dependencies: none hard.
- Acceptance: menu→play→pause→play→dead→restart flows without leaks. PASS evidence: [PIE] state traces.

#### N-4 — Pause menu
**P2 · NI · Low-Medium · Video: no**
- Description: Esc pause: resume/restart/quit + later options. Systems: input mode switch + widget.
- Dependencies: N-3, M-2. Acceptance: game halts (timesplit), input unlocked for mouse. PASS evidence: [USER] pass.

#### N-5 — Settings persistence
**P3 · NI · Low-Medium · Video: no**
- Description: settings (sensitivity, volume, quality) saved to config/user data.
- Systems: settings save/load + apply. Dependencies: O-3, N-3. Acceptance: settings survive restart. PASS evidence: [PIE] round-trip.

### Category O — Accessibility & input customization (7 actions)

#### O-1 — Enhanced Input framework
**P0 · I&T · Low · Video: (baseline)**
- Description: Enhanced Input with IMC_Default (16 mappings) + 8 IAs; persistence verified fresh-session. PASS evidence: Phase 1 item 28 + root-cause fix 9fa95ae.

#### O-2 — Key rebinding UI
**P2 · NI · Medium · Video: no**
- Description: user remaps actions at runtime; persisted. Systems: rebind widget + IMC modification + save (N-5).
- Dependencies: O-1, N-5. Acceptance: rebind survives restart; conflicts handled. PASS evidence: [PIE] mapping dump after rebind.

#### O-3 — Sensitivity/invert options
**P2 · NI · Low · Video: no**
- Description: mouse sensitivity + invert Y. Systems: multiplier variables + settings UI.
- Dependencies: K-1, N-5. Acceptance: applied immediately; persists. PASS evidence: [USER] feel test.

#### O-4 — Subtitles/captions
**P3 · NI · Low-Medium · Video: no**
- Description: captions for voice/SFX cues (campaign era). Dependencies: N-5. Acceptance: caption toggle works. PASS evidence: [USER] visual.

#### O-5 — Colorblind/high-contrast modes
**P3 · NI · Medium · Video: no**
- Description: HUD/enemy-marker readability modes. Systems: material/HUD variants. Dependencies: M-2.
- Acceptance: marker distinguishable in deuteranopia sim. PASS evidence: [USER] visual sim.

#### O-6 — Aim assist toggle (single-player option)
**P3 · NI · Medium · Video: no**
- Description: optional gentle aim magnetism (mainly for gamepad). Systems: aim correction on fire. Dependencies: E-1, O-7.
- Acceptance: toggle changes assist on/off; default off for mouse. PASS evidence: [PIE] assist math test.

#### O-7 — Gamepad support
**P2 · IBU · Low-Medium · Video: no**
- Description: IMC_Default ships 12 template mappings (likely include gamepad stick/trigger) — present but never physically verified.
- Systems: verify/add gamepad IMC mappings; stick look curve. Dependencies: O-1.
- Acceptance: gamepad move/look/fire/interact all function. PASS evidence: [USER] physical gamepad pass (manual item).

### Category P — Debugging, performance & QA (6 actions)

#### P-1 — PIE evidence harness
**P0 · I&T · Medium · Video: n/a**
- Description: Python-driven PIE sampling harness writing JSON evidence (established in Phase 1; slate post-tick polling pattern documented).
- Acceptance: harness records state timelines for any new F2 feature. PASS evidence: zcode_*.json series.

#### P-2 — Diagnostic management
**P0 · I&T · Low · Video: n/a**
- Description: prints/HUD togglable via `DisableAllScreenMessages`; prints wired IN SERIES (never replacing exec wires). PASS evidence: final Phase 1 session.

#### P-3 — Performance baseline
**P1 · NI · Low · Video: n/a**
- Description: record stat unit/fps/anim budgets at each F2 gate on this hardware (1080p).
- Systems: `stat unit`/`stat anim` screenshots + budget table (target: 60 fps graybox; ≤ 12 ms frame).
- Acceptance: budget table per map. PASS evidence: stat screenshots per gate.

#### P-4 — Repeatable smoke test
**P1 · PARTIAL · Medium · Video: n/a**
- Description: formalize the PIE harness into a script checking core chains (move/fire/damage/restart) per session.
- Dependencies: P-1. Acceptance: one-command run outputs PASS/FAIL per chain. PASS evidence: harness output JSON.

#### P-5 — Memory/perf budget enforcement
**P2 · NI · Medium · Video: n/a**
- Description: caps for AI count, FX pools, physics shells on 16 GB RAM; asset memory audit.
- Dependencies: G-*, L-*. Acceptance: 10-dummy + full FX scenario within budget. PASS evidence: `stat memory` capture.

#### P-6 — Package build test
**P2 · NI · Medium · Video: n/a**
- Description: verify a Windows package builds and runs (before content scales).
- Acceptance: packaged game boots to test map and plays. PASS evidence: package run video/log.

---

## PART 5 — PRIORITIZED IMPLEMENTATION ORDER

Totals: **124 actions** — 15 IMPLEMENTED AND TESTED · 8 PARTIALLY IMPLEMENTED · 1 IMPLEMENTED BUT UNVERIFIED (gamepad O-7) · 100 NOT IMPLEMENTED. Priority split: **P0 = 47 · P1 = 32 · P2 = 29 · P3 = 16** (count includes every card; crosshair consolidated in E-5 and damage vignette in K-6, no duplicate cards).

### Development order — waves (reusable systems before isolated demos; Blueprint-first unless C++ has a clear advantage; every wave = compile → PIE → owner physical pass → commit/push)

**Wave F2.0 — "Weapon core + dummies" (the reference-video graybox loop; P0, est. 3–5 agent sessions)**
A-1..A-4, A-8, J-1, J-3, F-1, K-1, K-2, M-1, N-1, O-1 are DONE. New:
1. C-1 + B-7 + B-1 (graybox rifle + arms visibility) — the enabling trio.
2. D-1 + D-3 + D-8 + D-9 (fire, ROF, ammo, gating) — one weapon state machine.
3. E-1 + E-3 + E-5 (+E-2, K-5) — ADS, spread, crosshair, recoil, shake.
4. G-1 + G-2 + F-2 + F-3 + M-7 hitmarker — dummies react and die.
5. M-2 + M-3 + M-4 — real HUD (health + ammo + crosshair).
6. L-1 + L-2 + L-3 + L-4 + L-5 — placeholder FX/SFX pack.
Gate: full reference-loop acceptance (fire→hit→react→die→HUD→reload→switch).

**Wave F2.1 — Weapon handling completeness (P0/P1)**
C-3 reload, C-5/C-6 switching, C-7 slots, C-4 dry fire, B-6 aim offsets, B-8 left-hand IK, B-9 layering, E-4/E-6/K-8, D-2 fire modes, L-6 foley, H-4 weapon warp (basic).
Input decision: R currently = Restart (N-1). Recommend reload on R and restart on Enter (hold) — needs owner approval before implementation.

**Wave F2.2 — Close quarters + throwables (P1)**
H-1 takedown (contextual), H-2 melee, I-1..I-4 grenade chain, H-3 hand IK, L-9 explosion, J-4 pickups.

**Wave F2.3 — Enemy AI basics (P1)**
G-9 navmesh, G-3 perception, G-5 engage, G-7 flinch integration, G-8 death/respawn, G-10-lite alert by sound, G-4-lite patrol.

**Wave F2.4 — World & audio pass (P1/P2)**
J-2 doors, L-7 footsteps, P-3 perf baseline, P-4 smoke test formalization.

**Wave F2.5 — States, options, polish (P2)**
N-2 save/checkpoint, N-3/N-4 game state + pause, O-2/O-3 options, A-5 prone, A-6 lean, K-3/K-4/K-6/K-7 feedback set, F-5 damage indicator, F-7→D-6 damage typing/falloff, A-9 fall damage, A-10 foot IK, M-5 select UI, M-6 inventory, F-8 health pickup (if not in F2.2), G-6 cover, G-10 full, C-9 weapon pickup/drop, P-5 budgets, P-6 package test, O-7 gamepad verify.

**Backlog (P3)**: C-10 inspect, D-5 projectile, D-7 penetration, F-6 damage numbers, I-5 cook, I-6 smoke, J-5/J-6/J-7, L-8/L-10, M-8 objectives, N-5 settings, O-4/O-5/O-6.

**Sequencing rationale:** the P0 core is chosen to make a *complete reusable loop* (weapon system → damage system → target system → HUD → FX) rather than isolated showcases; everything later (takedowns, grenades, AI, saves) plugs into these five systems. Placeholder-first policy keeps every wave testable on the RTX 3060/16 GB machine without art production.

---

## PART 6 — TECHNICAL DEPENDENCIES

Dependency map (→ = depends on):

- **Weapon render stack:** B-1 → B-7 → C-1 → C-2; C-5/C-6 → C-7 → C-8.
- **Firing stack:** D-1 → {D-3, D-4→E-3, D-8, D-9}; E-2 → D-1; K-5 → E-2; L-1/L-2 → B-7 sockets; L-5 → D-1.
- **ADS stack:** E-1 → {B-6, B-7, E-4, E-5, E-6, K-8, E-3-spread}.
- **Target stack:** G-1 → G-2 (BPC_Health reuse) → {F-2 → G-7; F-3 → G-8}; M-7 hitmarker → D-1 + M-2.
- **HUD stack:** M-2 → {M-3, M-4, E-5 crosshair, M-5, M-7, F-5, K-6 vignette}; M-6 inventory → C-7/C-8 → J-4 pickups → {F-8 health, C-9}.
- **Melee/takedown stack:** H-2 → D-9 gating; H-1 → G-1/F-3 + B-1 (+H-3 optional); H-3 → B-8; H-4 → B-1/B-7.
- **Throwable stack:** I-1 → C-7 + B-9; I-2 → I-1 + I-3; I-4 → I-3 + BPC_Health (all victims); I-6 → I-4 + G-3 (sight blocking).
- **AI stack:** G-9 navmesh → {G-4 patrol, G-5 engage}; G-3 perception → {G-5, G-10}; G-6 cover → G-9 + G-5.
- **State stack:** N-3 → {N-2, N-4}; N-5 → O-2/O-3; N-2 → M-6 inventory data.
- **QA stack:** P-1 harness (exists) → P-4 smoke test → all waves; P-3/P-5/P-6 gate waves F2.3+.

Reusable assets/patterns to leverage: `BPC_Health` (all damageable actors), `BPI_Interactable` (all world interactions), `ABP_Unarmed` + mannequin library (dummies/AI), DisplayAll-style HUD ground truth, PIE JSON harness, `set_editor_property("mappings",…)` persistence pattern, exec-wire dump verification.

New input actions proposed (all require owner approval, none created in this audit): IA_Fire (LMB), IA_ADS (RMB), IA_Reload (R — remap decision with N-1), IA_NextWeapon (1/2/wheel), IA_Melee (V), IA_Grenade (G), IA_Takedown (E-contextual/F), IA_Prone (Z), IA_Lean (Q/E), plus restart remap (Enter).

---

## PART 7 — ANIMATION REQUIREMENTS

**Key finding:** the UE 5.8 Muse mannequin library (`Content/Characters/Mannequins/Anims/`) supplies third-person full-body clips that cover nearly every *body-level* need for dummies/AI and some player-body needs, but **first-person arms animation content does not exist anywhere in the project** and is the single largest animation gap.

| Need | Existing asset? | Plan |
|---|---|---|
| Dummy/AI idle | ✅ ABP_Unarmed + MM_Idle (proven on player) | Reuse directly |
| Dummy/AI locomotion | ✅ 8-dir walk/jog blendspaces (Unarmed/Rifle/Pistol) | Reuse |
| Dummy/AI jump set | ✅ Unarmed + Rifle jump sets | Reuse |
| Hit reactions | ✅ 13× MM_HitReact (Lgt/Med/Hvy, F/B) | Wire on damage (F-2) |
| Deaths | ✅ 6× MM_Death directional | Wire on death (F-3) |
| Fire/reload/equip/dry-fire (body) | ✅ Rifle + Pistol sets incl. fire montage | Body-layer placeholders for player, full for AI |
| Aim offsets | ✅ AO_Rifle / AO_Pistol + CU/CD/CC | Wire (B-6) |
| Melee attacks | ✅ MM_Attack_01–03 + charged | Wire (H-2) |
| FP arms idle/walk/sprint poses | ❌ none | Graybox: static posed arms mesh on sockets + weapon bob (code-driven); real FP arms anims are a later art production task |
| FP arms reload/ADS transitions | ❌ none | Graybox: procedural weapon transforms + IK (B-8); art later |
| Crouch body anim | ❌ documented Phase 1 placeholder | Camera+capsule only; add when art pass begins |
| Prone/lean body anims | ❌ none | P2; procedural first |
| Takedown paired anims | ❌ none | Graybox: compose from death/attack clips + camera lock; dedicated pair later |

**Placeholder policy:** graybox uses (a) existing mannequin clips for dummies/AI, (b) code-driven procedural motion (weapon bob, recoil kick, IK reaches) for FP arms, (c) cheap Niagara/MP4-free placeholder FX. Actions that **genuinely require later art production** (not placeholder-able): polished FP arms set, paired takedown anims, prone/lean refinement, final weapon meshes/models, real audio pack. These are logged for the (deferred) art phases — Phase 2 GROM modeling continues under its own approval track.

---

## PART 8 — AI REQUIREMENTS

- **Foundation path:** dummies first (G-1/G-2), then AIController + NavMesh (G-9), then perception (G-3), then engage (G-5). Blueprint-first; Behavior Trees adopted when G-4/G-6 arrive (P2) — a simple state machine is preferred for the first engage loop to stay debuggable on this machine.
- **Assets:** `SKM_Manny_Simple`/`SKM_Quinn_Simple` + `ABP_Unarmed`→`ABP_Rifle`-style layering later; enemy weapon = same `BP_Weapon_Base` simplified (D-1 reuse for AI fire).
- **Damage integration:** every enemy hosts `BPC_Health` (proven component) — death dispatchers already exist.
- **Performance guards (16 GB RAM):** ≤ 10 active AI in graybox tests; perception updates throttled (e.g., 5 Hz); no perception hearing before G-10; navmesh only on test maps initially. `stat anim`/`stat ai` checked each wave (P-3).
- **QA constraints carried from Phase 1:** AI verification via PIE sampling + debug draws (perception cones, paths); physical passes stay with the owner.

---

## PART 9 — QUALITY & PERFORMANCE REQUIREMENTS

1. **Targets:** 60 fps at 1080p on RTX 3060 + Ryzen 5 7500F for all graybox maps; ≤ 12 ms frame (stat unit); editor scalability modest (AGENTS.md).
2. **Memory:** 16 GB RAM — cap simultaneous Niagara systems (pool placeholders), cap AI at 10, pool shell/decal actors, audit `stat memory` per wave (P-5).
3. **Compile hygiene:** every Blueprint BS_UP_TO_DATE with zero node errors before commit; exec-wire dumps after any rewiring (Phase 1 lesson).
4. **Functional prototype vs final polish:** each gate accepts *placeholder* anim/FX/audio; polish tasks are logged, not blocking. Never mark PASS from asset existence (evidence classes [USER]/[PIE]/[STRUCT] mandatory).
5. **Regression list per wave:** movement speeds, crouch capsule, damage/death/restart, interaction trace, input persistence, zero BP errors — run at every gate.
6. **Persistence:** all new mappings/assets verified after full editor restart; commit+push after every verified wave.
7. **Editor session rules:** one editor instance; owner restarts editor after agent changes; SkyLight recapture only in rendering sessions; manual key passes owner-side (automation blocked).

---

## PART 10 — ACTION ACCEPTANCE TESTS (gate framework)

Every action's gate (from STEP 5), applied on top of the per-action acceptance lines above:

1. Correct gameplay behavior (spec values, not "it seems to work").
2. Correct state transitions (e.g., fire gate matrix: alive/reloading/switching/dead).
3. Functional placeholder animation present where the action needs visible feedback.
4. No broken input mappings (fresh-session mapping verify; the `set_editor_property` persistence pattern).
5. No blocking Blueprint errors (compile status + log scan).
6. Correct interaction with health/death systems (damage flows through BPC_Health dispatchers).
7. Appropriate sound/visual feedback when relevant (placeholder allowed).
8. Successful PIE test (harness JSON evidence, P-1).
9. Persistence after save/reopen (fresh editor session re-check).
10. No significant regression (Part 9.5 list re-run).
11. Documented QA evidence (evidence class + file path in the wave report).

**Placeholder-eligible now** (explicitly): FP arms visuals, all FX/audio, crouch/prone/lean body anims, takedown choreography. **Requires later art production:** final FP arms animation set, paired takedown, final weapon/character models, real audio pack — scheduled under the (deferred) art phases; not gates for graybox PASS.

**Evidence classes:** [USER] = owner physical key/visual pass · [PIE] = machine runtime measurement (JSON) · [STRUCT] = graph/asset structural verification. Phase 1 convention, kept.

---

## PART 11 — RECOMMENDED IMMEDIATE NEXT TASK

**Single recommended next task (after owner/ChatGPT approves this register):**

> **Task F2.0.1 — "Weapon foundation trio": create `BP_Weapon_Base` (graybox rifle: mesh + Muzzle/hand sockets), equip it on `BP_PlayerFoundation` with visible placeholder arms (B-1/B-7/C-1), and implement hitscan fire with ammo tracking and the fire-state gate (D-1/D-3/D-8/D-9), verified by PIE JSON evidence + owner physical pass.**

Why this first: it unlocks ADS, recoil, reload, dummies, hitmarker, and half the FX stack (everything hangs off the weapon system); it is a self-contained, testable increment that touches exactly one new BP + one existing BP + one new IA — the smallest change with the largest unlock. Note for the owner: the new IA_Fire (LMB) mapping is created with the persistence-safe `set_editor_property("mappings",…)` method from Phase 1 (the `map_key` silent-failure pitfall).

---

## خلاصه (دری / افغانی) — SUMMARY IN DARI

**ثبت جامع اعمال بازید (Foundation Action Register) — کار F1.1، تاریخ ۲۰۲۶/۱۱/۱۴**

- این یک **بررسی فقط-خواندنی** بود؛ هیچ سیستم بازی، هیچ Blueprint، هیچ مپ و هیچ ورودی عوض نشد. فقط اسناد ساخته/به‌روز شد.
- پروژه: UE 5.8، کامیت پایه `04bf7cd`، فاز ۱ تأییدشده، همه سیستم‌های فاز ۱ دست‌نخورده باقی‌اند.
- **مجموع اعمال شناسه‌شده: ۱۲۴ عمل** در ۱۶ دسته (A تا P).
  - **۱۵ عمل اجرا و تست‌شده** (راه‌رفتن، دویدن، خم‌شدن، پرش، برخورد با سقف کوتاه، سیستم سلامگی/آسیب، تعامل با E، دوربین، HUD اشکال‌زدایی، ری‌استارت، پایه ورودی، هارنس آزمایش PIE، مدیریت پیام‌های اشکال‌زدایی).
  - **۸ عمل نیمه‌اجراشده** (انیمیشن بدن، حالت مرگ بازیکن، موانع فیزیکی، نمایش سلامگی، تست خودکار).
  - **۱ عمل اجراشده ولی تأییدنشده** (گیم‌پد) و **۱۰۰ عمل موجود نیست** — همه بخش‌های اسلحه، تیراندازی، دشمنان، منفجره‌ها، HUD اصلی، ذخیره و تنظیمات از صفر شروع می‌شوند.
- کشف مهم: کتابخانه انیمیشن «منکن» UE 5.8 در پروژه موجود است (شلیک/ری‌لود/ADS/واکنش ضربه/مرگ/۸ جهت راه‌رفتن) — به‌عنوان جانگهدار برای دشمنان و بدن استفاده می‌شود؛ اما **انیمیشن دست اول‌شخص وجود ندارد** و برای فاز خاکستری از حرکت پروسیجرال استفاده خواهد شد.
- **اولویت‌ها:** ۴۷ عمل P0 (حلقه کامل ویدیوی مرجع)، ۳۲ عمل P1، ۲۹ عمل P2، ۱۶ عمل P3.
- **ترتیب پیشنهادی:** موج F2.0 (اسلحه + دشمن‌های خاکستری + HUD + افکت‌ها) → F2.1 (ری‌لود/تعویض سلاح) → F2.2 (نبرد نزدیک + نارنجک) → F2.3 (هوش مصنوعی پایه) → F2.4 (دنیا و صدا) → F2.5 (ذخیره/توقف/تنظیمات و پولیش).
- **قدم بعدی پیشنهادی:** کار F2.0.1 — ساخت `BP_Weapon_Base` (تفنگ خاکستری)، نمایش دست‌ها، تیراندازی hitscan با سیستم مهمات — بعد از **تأیید این ثبت توسط مالک و ChatGPT**.
- مدل‌سازی GROM_01 و کارهای هنری همچنان **متوقف‌شده** طبق تصمیم مالک؛ هیچ اصل تاریخی یا سندی پاک نشد.

*پایان خلاصه دری.*

---

*Register ends. This document is a planning artifact only — no gameplay asset was created or modified in Task F1.1 (verified: git tree contains only this file and the PROGRESS.md update).*
