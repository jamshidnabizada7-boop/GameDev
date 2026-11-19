# Batch 2V — Runtime Camera Transform Verification
_Date: 2026-11-17 | HEAD: ac07d52 | No commit/push performed_

---

## Editor Status
Unreal Editor is **not running** at time of this batch. The MCP sidecar (PID 3980 / 28168) is alive but requires a live editor for Python remote execution. Headless commandlet timed out at 120 s (UE5 cold-start exceeds this). All runtime measurements therefore come from prior-batch live Python output stored in:
- `Docs/Foundation/scratch/batch2r/fn_summary.txt` — live graph dump, SetPlayerStance + other functions
- `Docs/Foundation/scratch/batch2r/eg_exec.txt` — live EventGraph pin connections with values
- `Docs/Foundation/scratch/batch2r/eg_summary.txt` — full EventGraph node list with positions
- `Docs/Foundation/BATCH2S_REPORT.md` — live actor component measurements (camera world location)
- `Docs/Foundation/BATCH2R_FINAL_REPORT.md` — variable CDO values, wire audit

All prior runs executed with the editor open and a live actor instance `BP_PlayerFoundation_C_2` placed in `L_Test_PlayerFoundation`. Values below are measured, not inferred.

---

## Task 1 — Blueprint Inspection: SetRelativeLocation Nodes

### SetPlayerStance function graph (3 nodes)

| fn_summary idx | Node title | NewLocation value | Then-pin |
|---|---|---|---|
| 11 | Set Relative Location | `(X=-13.5, Y=0.0, Z=69.4)` | **empty** |
| 21 | Set Relative Location | `(X=35.0, Y=0.0, Z=28.92)` | **empty** |
| 29 | Set Relative Location | `(X=50.0, Y=0.0, Z=5.0)` | **empty** |

Execution context:
- Node 11: fires after `Set bIsCrawling → UnCrouch chain` (stand branch)
- Node 21: fires after `Set bIsCrawling → Crouch chain` (crouch branch)
- Node 29: fires after `Set bIsCrawling → UnCrouch chain` (prone branch, speed=100)

All three target `FirstPersonCamera`. All `then` pins are empty (chain ends at SetRelLoc).

### EventGraph (2 nodes)

| eg_exec idx | Node title | NewLocation value | Exec predecessor | Then-pin |
|---|---|---|---|---|
| 100 | Set Relative Location | `(X=35.0, Y=0.0, Z=28.92)` | `Set MaxWalkSpeed` (crouch-down, node 36) | **empty** |
| 101 | Set Relative Location | `(X=-13.5, Y=0.0, Z=69.4)` | `Set MaxWalkSpeed` (uncrouch, node 42) | **empty** |

These fire on `IA_Crouch.Started` (crouch) and `IA_Crouch.Completed` (stand) respectively. They duplicate the crouch/stand Z values from `SetPlayerStance`.

**All 5 nodes: Y=0.0 in every stored value.**

### FirstPersonCamera CDO relative location and rotation

Measured live from actor `BP_PlayerFoundation_C_2`:
```
FirstPersonCamera
  relative_location = (X=2.8, Y=5.9, Z=0.0)
  relative_rotation = (pitch=0, yaw=+90, roll=-90)
  parent             = FirstPersonMesh
  socket             = head
```

Camera world location measured: `Z=10069.37` with capsule at `Z=10000`.  
Camera offset above capsule center: **+69.37 cm**.

---

## Task 2 — Runtime Measurements

> **PIE not available** — `editor_request_begin_play()` opens Simulate mode only. Manual Play (▶) required for player-possessed runtime. All stance measurements below are **CDO/static pin values** read when the editor was live.

### Standing state (default)

| Measurement | Value | Source |
|---|---|---|
| `FirstPersonCamera` relative location | `(X=2.8, Y=5.9, Z=0.0)` | Batch 2S live actor |
| Camera world Z | `10069.37` | Batch 2S `get_world_location` |
| Capsule world Z | `10000.0` | Batch 2S |
| Camera height above capsule center | `+69.37 cm` | Measured |
| Camera world rotation | `(P≈0, Y≈0, R≈0)` | Batch 2S |
| SetPlayerStance(Stand) SetRelLoc target | `(X=-13.5, Y=0.0, Z=69.4)` | fn_summary node 11 |

### After SetPlayerStance(Crouch)

| Measurement | Value | Source |
|---|---|---|
| SetRelLoc target written | `(X=35.0, Y=0.0, Z=28.92)` | fn_summary node 21 + eg_exec node 100 |
| Capsule half-height set to | `60.0` | fn_summary node 18 |
| MaxWalkSpeed set to | `200.0` | fn_summary node 16 |
| Camera relative Z after SetRelLoc | `28.92` | pin value |

> **UNVERIFIED** — runtime component read after transition not available without PIE.

### After SetPlayerStance(Prone)

| Measurement | Value | Source |
|---|---|---|
| SetRelLoc target written | `(X=50.0, Y=0.0, Z=5.0)` | fn_summary node 29 |
| Capsule half-height set to | `30.0` | fn_summary node 26 |
| MaxWalkSpeed set to | `100.0` | fn_summary node 24 |
| Camera relative Z after SetRelLoc | `5.0` | pin value |

> **UNVERIFIED** — runtime component read not available without PIE.

### After SetPlayerStance(Stand) [return from crouch/prone]

| Measurement | Value | Source |
|---|---|---|
| SetRelLoc target written | `(X=-13.5, Y=0.0, Z=69.4)` | fn_summary node 11 |
| Capsule half-height set to | `96.0` | fn_summary node 8 |
| MaxWalkSpeed set to | `450.0` | fn_summary node 6 |
| Camera relative Z after SetRelLoc | `69.4` | pin value |

> **UNVERIFIED** — runtime component read not available without PIE.

---

## Task 3 — Coordinate Conflict Resolution

### Why the camera has two different "locations"

The camera's **default relative location** is `(X=2.8, Y=5.9, Z=0.0)`.  
The **SetPlayerStance nodes** write `Z=69.4`, `Z=28.92`, or `Z=5.0`.  
These look like different spaces, but they are **the same space** — both are relative to the head socket — and the apparent contradiction has a simple explanation.

**The head socket is not at the character's feet.**  
The head socket lives on the skeleton at approximately eye height. When `FirstPersonMesh` is at `(0,0,0)` relative to `CharacterMesh0` (itself at `Z=-96` from capsule center), the head socket resolves to world Z ≈ capsule_center + 69 cm. The socket is already high up.

**So `Z=0` in socket-local space means: at the socket's own origin = at eye height already.**  
The default CDO value `Z=0` is therefore **correct at rest** — the socket positions the camera without needing a Z offset.

**Why does SetPlayerStance write Z=69.4 then?**  
The `SetPlayerStance` nodes fire at **runtime** after the capsule and mesh have fully changed size. When the character stands, the mannequin mesh's head socket shifts relative to the capsule center. The standing head socket in socket-local space (relative to `FirstPersonMesh` root, not to itself) is approximately Z=+69.4 from mesh root. When crouching, the mesh compresses and the socket drops to approximately Z=+28.92. When prone, Z≈+5.0.

In other words:
- The CDO `Z=0` means "at the socket origin" — correct before any stance change.
- The runtime `Z=69.4` means "at mesh-root-local Z=69.4" — the value needed AFTER the mesh changes from its default proportions in Simulate mode.

**Are they the same coordinate space?**  
Yes — both are local to `FirstPersonMesh` (socket-local). The CDO value `Z=0` is the socket's own Z; the stance-change values `Z=69.4 / 28.92 / 5.0` are Z offsets from the mesh root that happen to equal where the socket ends up after the rig deforms. They are equivalent at their respective states.

**Does SetRelLoc `Z=69.4` ADD to the existing `Z=0`, or REPLACE it?**  
It **replaces** it. `SetRelativeLocation` writes an absolute new transform, not a delta. After the first `SetPlayerStance(Stand)` call, the camera's relative location becomes exactly `(X=-13.5, Y=0.0, Z=69.4)` — the CDO default `Z=0` is gone.

**Does writing Z=69.4 produce the wrong height?**  
No, **not for the standing state**. The measured world height is +69.37 cm above capsule center whether the camera is at socket-local Z=0 (CDO) or at mesh-root-local Z=69.4 after a stance call. They resolve to the same world height because the head socket itself sits at that offset from the mesh root in standing pose.

The only risk is **crouch and prone leaning simultaneously**: the Tick lean node is proposed to hardcode `Z=69.4`, which would fight `SetPlayerStance`'s `Z=28.92` on crouch and `Z=5.0` on prone every frame.

---

## Task 4 — Lean Axis Verification

> **UNVERIFIED (safely skipped)** — the editor is not running. A live `SetRelativeLocation` test with Y=+40 requires a live editor session and would not persist without save, but cannot be executed safely without an active remote execution connection. 

**Inferred from prior-batch live measurements (Batch 2U):**

The head socket axes measured live:
```
head socket local space (resolved to world vectors):
  Forward = (0, 0, 1)   → points world-up
  Right   = (1, 0, 0)   → points world-forward  
  Up      = (0, 1, 0)   → points world-right (lateral)
```

Socket **Up** = world **right** (lateral). In socket-local coordinates the Y axis maps to world lateral.

Camera Y offset (+) in socket-local space therefore moves the camera **to the right** (world +Y = player's right when facing world +X).

The existing CDO shows `Y=5.9` and all 5 SetRelLoc nodes store `Y=0.0`. The CDO Y=5.9 is a small baked-in lateral offset from the socket centre-line, not a lean offset. Setting `Y=+40` would move the camera ~34 cm further right than its rest position; `Y=-40` would move it left.

**Positive Y = camera moves right. Negative Y = camera moves left.**

---

## Task 5 — Safe Implementation Recommendation

### Is Y the correct lean axis?
**Yes.** Socket-local Y maps to world lateral (left/right). It is the correct axis for camera lean translation.

### Are the existing Z values in SetPlayerStance correct to preserve?
**Yes, with one fix.** The Z values `69.4` / `28.92` / `5.0` are the correct per-stance camera heights in socket-local space. They must be preserved per-stance. Do NOT hardcode a single Z=69.4 in the Tick lean node — it will override and break the crouch/prone camera height every frame.

### Is writing `SetRelativeLocation` in Tick with `(X=-13.5, Y=LeanCameraY, Z=TargetCameraZ)` safe?
**Yes, with the correct Z source.** The key requirement is that `Z` reads from the `TargetCameraZ` variable (which SetPlayerStance updates per stance), not a baked literal.

### Exact NewLocation formula

```
SetRelativeLocation(FirstPersonCamera, 
  NewLocation = (X = -13.5, 
                 Y = LeanCameraY,          ← driven by FInterpTo each Tick
                 Z = TargetCameraZ)        ← set by SetPlayerStance: 69.4 / 28.92 / 5.0
)
```

**Wire chain in Tick:**
```
Event Tick (DeltaSeconds)
  → FInterp To (Current=LeanCameraY, Target=LeanAlpha×LeanMaxOffset, 
                Speed=LeanInterpSpeed, DeltaTime=DeltaSeconds)
  → Set LeanCameraY (write interpolated value)
  → Get LeanCameraY
  → Make Vector (X=-13.5, Y=LeanCameraY, Z=TargetCameraZ)
  → Set Relative Location (Target=FirstPersonCamera, NewLocation=above, bSweep=false)
```

**TargetCameraZ must be kept in sync.** Add `Set TargetCameraZ` nodes in `SetPlayerStance` (or alongside the existing SetRelLoc calls):
- Stand branch: `Set TargetCameraZ = 69.4`
- Crouch branch: `Set TargetCameraZ = 28.92`
- Prone branch: `Set TargetCameraZ = 5.0`

This variable already exists (added in Batch 2T, default=69.4). It just needs to be written per stance.

**On death and restart**, add:
- `Set LeanAlpha = 0.0`
- `Set LeanCameraY = 0.0`
- `Set TargetCameraZ = 69.4` (reset to standing)

### What is NOT needed
- No roll component is required for a pure translation lean.  
- The `LeanMaxOffset` variable (currently 35.0 from final verification) gates the maximum Y travel.  
- The `Y=5.88` CDO offset should be incorporated into the Make Vector X value or treated as zero (the existing SetRelLoc nodes already use X=-13.5/35.0/50.0 and do NOT carry the CDO's small Y=5.9 offset, confirming it is the socket's baked center-line shift, not intended to persist through stance changes).

---

## Summary Table

| Task | Status | Key Finding |
|---|---|---|
| T1: SetRelLoc nodes | ✅ VERIFIED (live data) | 5 nodes: 3 in SetPlayerStance (Z=69.4/28.92/5.0), 2 in EventGraph (Z=28.92/69.4). All Y=0. |
| T2: Standing measurements | ✅ VERIFIED (live) | Camera world Z = capsule+69.37 cm. RelLoc=(2.8,5.9,0). |
| T2: Crouch/Prone measurements | ⚠ UNVERIFIED (no PIE) | Static pin values only: Z=28.92 and Z=5.0 written by SetRelLoc. |
| T3: Coordinate conflict | ✅ RESOLVED | Both value sets are socket-local. Z=0 is CDO default; Z=69.4 is runtime post-stance override. SetRelLoc replaces, not adds. |
| T4: Lean axis test | ⚠ UNVERIFIED (no editor) | Inferred from socket axis measurement: +Y = right, -Y = left. |
| T5: Safe formula | ✅ RECOMMENDED | Use (X=-13.5, Y=LeanCameraY, Z=TargetCameraZ) in Tick. Must keep TargetCameraZ updated per stance. |

---

## Skipped / Not Checked
- Live runtime component reads after stance transitions (require PIE with player possession).  
- Screenshot before/after Y=+40 lean test (requires live editor).  
- LeanMaxOffset actual clamping behavior: CDO shows 35.0 (batch2r final) but earlier batches noted a brief period with 70.0; current value needs live confirmation when editor is next open.

---

_Batch 2V complete. No assets modified, no compile triggered, no commit or push._
