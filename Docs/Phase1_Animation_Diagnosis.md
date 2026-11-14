# PHASE 1 ANIMATION DIAGNOSIS — Afghanistan FPS
Agent: ZCode — 2026-11-14. READ-ONLY inspection; no animation or gameplay assets modified.

## 1. Existing animation architecture

**Pawn**: `BP_PlayerFoundation` (Character)
- `CollisionCylinder` capsule: half-height 96, radius 34
- `CharacterMesh0` (SkeletalMeshComponent — the BP also exposes it as the variable
  `FirstPersonMesh`):
  - Skeletal mesh: **`/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`** (FULL-BODY mannequin)
  - Anim class: **`/Game/FirstPerson/Anims/ABP_FP_Copy.ABP_FP_Copy_C`**
  - `OwnerNoSee = False` — the full body renders for the player
  - instance relative transform: (0, 0, 0), rotation (0,0,0) — centered on the capsule
- `FirstPersonCamera`: instance-relative location ≈ (2.8, 5.9, ~65) — sits inside the
  body mesh's volume, slightly behind/above its center

**Animation Blueprints (3 found):**
| Asset | AnimGraph | Contents |
|---|---|---|
| `ABP_FP_Copy` (assigned) | **3 nodes, EMPTY** | No state machine, no sequences, no blendspaces |
| `ABP_Unarmed` (`/Game/Characters/Mannequins/Anims/Unarmed/`) | Full system | Main States → Locomotion → Idle (`MM_Idle`), Walk/Run (**Blendspace `BS_Idle_Walk_Run`** driven by GroundSpeed), Jump (`MM_Jump`), Fall Loop (`MM_Fall_Loop`), Land (`MM_Land`) + transitions; EventGraph (36 nodes) sets GroundSpeed |
| `ABP_Copycat` (`/Game/DemoTemplate/Animated/`) | Full system | Same structure as ABP_Unarmed |

All three target skeleton **`SK_Mannequin`** — fully compatible with SKM_Manny_Simple.

**Sequence inventory** (all present on disk, none currently played by the player mesh):
Unarmed: Walk 8, Jog 8, Jump 5, Idle 1, Attack 4 · **Death 6** · Rifle: Walk/Jog 8+8,
Jump 6, HitReact 8, AIM 3 · Pistol: Walk/Jog 8+8, Jump 3, AIM 3 · = 97+ assets.

## 2. What actually plays during PIE (screenshot evidence, `anim_*.png`)

| State | Observed |
|---|---|
| Idle | **No animation** — ABP_FP_Copy's empty AnimGraph outputs a static pose; body is below the eye-line so nothing is visible at level pitch |
| Walk | **No visible animation** — screenshot identical to idle |
| Sprint | **No different animation** — same static pose (no sprint-specific state exists in ABP_FP_Copy) |
| Crouch | Capsule + camera change only (camera-lower fires via the input chain); **no crouch animation** |
| Jump/Fall/Land | Physics work (mesh airborne at z −10.5 during jump) but **no jump/fall/land poses** |
| Transitions | None occur — there is no state machine in ABP_FP_Copy |

## 3. Working states
Movement physics, sprint speed change, crouch capsule (96→40), jump/fall/land physics,
damage/death/restart — all verified. Only the *visual* animation layer is missing.

## 4. Missing or placeholder states
- Idle / Walk / Jog / Jump / Fall / Land poses — **not playing** (empty AnimGraph)
- Sprint-specific animation — **missing** (the Blendspace in ABP_Unarmed would handle
  walk↔jog by GroundSpeed automatically once assigned)
- Crouch animation — **missing** (capsule-only)
- Death animation — **missing** (6 Death sequences exist, unused)
- HitReact — **missing** (8 Rifle HitReact sequences exist, unused)

## 5. Camera / mesh presentation issues (root causes measured)

| Symptom (user reports) | Measured root cause |
|---|---|
| Arms mainly visible looking downward | `OwnerNoSee = False` on the full-body mesh + camera pitches down into the body volume |
| Torso/arms occupy excessive camera space | The camera (rel ≈ 2.8, 5.9, 65) sits **inside the mesh's body volume**; at crouch or downward pitch the torso fills the frame |
| Poses unusually close to the camera | Same — plus the static pose from the empty AnimGraph looks unnatural when clipped by the camera |
| Crouch "shows a side" | Crouch camera dropped to rel z 28.9 while x = −13.5 → camera inside the body, seeing its side profile |

These are **mesh-position/visibility + empty-animation problems**, not input or
gameplay problems. All gameplay systems remain untouched and verified.

## 6. Minimum recommended fixes (for approval — not yet implemented)

1. **Assign the real locomotion ABP**: set `CharacterMesh0 → Anim Class` to
   `ABP_Unarmed` (drag-select in the mesh component's Class Defaults). Its Idle /
   Walk-Run blendspace / Jump / Fall / Land states then play automatically —
   GroundSpeed is already fed by the CharacterMovement. ~2 minutes, no other changes.
2. **Set `OwnerNoSee = True`** on CharacterMesh0: the player stops seeing their own
   torso (clean first-person view at every pitch; body still renders to others for
   future co-op). This removes all three presentation complaints at once.
3. Optional, later: wire one of the 6 Death sequences into the death flow (BPC death
   → mesh montage), and design an arms-only first-person rig in Phase 2.
4. Do NOT touch ABP_Unarmed/ABP_Copycat (they are complete) and do not touch
   ABP_FP_Copy (unused once fix 1 lands; keep as a placeholder).

Trade-off to approve: with fix 2 the player no longer sees their own arms (the current
"arms" are actually the full body mesh clipping into the camera). A dedicated arms rig
is the proper Phase 2 solution.

## 7. Evidence required for final animation approval

After fixes 1–2 are applied (and only then), the following PIE captures:
1. Idle ≥3 s — visible idle breathing pose (frame difference proves playback)
2. Walk ≥2 s — walk cycle visibly animating (Blendspace at ~450 cm/s)
3. Sprint ≥2 s — Jog part of the blendspace at ~700 cm/s (different stride than walk)
4. Crouch held ≥2 s — capsule 96→40 on HUD + crouched locomotion pose + camera drop
5. Jump — MM_Jump → MM_Fall_Loop → MM_Land sequence visibly played
6. Level-gaze screenshots at multiple pitches — no body/torso in camera space
7. Regression: one full damage → death → R → 100 loop (HUD proof) to show gameplay
   was untouched
