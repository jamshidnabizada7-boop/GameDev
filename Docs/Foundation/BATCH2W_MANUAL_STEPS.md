# BATCH 2W — Manual Blueprint Steps
## Smooth First-Person Camera Lean for BP_PlayerFoundation

**Date:** 2026-11-17  
**Git HEAD:** ac07d52 (do not commit/push)  
**Backups verified:** D:\GameDev\Backups\Batch2R_PreChanges_2026-11-16, Checkpoint_2026-11-17_134319

---

## PYTHON AUTOMATION RESULTS (already done — do not redo)

| Item | Status |
|---|---|
| `TargetCameraX` float var added, default = -13.5 | IMPLEMENTED |
| `TargetCameraY` float var added, default = 5.9 | IMPLEMENTED |
| `LeanInterpSpeed` float var — already existed | ALREADY EXISTED |
| `TargetCameraZ` float var — already existed (default = 69.4) | ALREADY EXISTED |
| `LeanCameraY` float var — already existed (default = 0.0) | ALREADY EXISTED |
| `LeanAlpha` float var — already existed | ALREADY EXISTED |
| `LeanMaxOffset` float var — already existed (CDO = 35.0) | ALREADY EXISTED |
| `compile_blueprint` after adding vars | COMPILED: True |
| Asset saved | IMPLEMENTED |

---

## EXISTING NODE MAP (from live Batch 2R measurements)

### EventGraph — Tick chain
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 163 | Event Tick | — | Branch (165) |
| 165 | Branch (IsFalling check) | Tick | Set FallVelocityZ (168) |
| **168** | **Set FallVelocityZ** | Branch (165) | **EMPTY ← Tick tail** |

### EventGraph — IA_LeanLeft (Q key)
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 154 | IA_LeanLeft (Enhanced Input) | — | — |
| **155** | **Set LeanAlpha = -1.0** | IA_LeanLeft.Started | **EMPTY** |
| **156** | **Set LeanAlpha = 0.0** | IA_LeanLeft.Completed | **EMPTY** |

### EventGraph — IA_LeanRight (F key)
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 157 | IA_LeanRight (Enhanced Input) | — | — |
| **158** | **Set LeanAlpha = +1.0** | IA_LeanRight.Started | **EMPTY** |
| **159** | **Set LeanAlpha = 0.0** | IA_LeanRight.Completed | **EMPTY** |

### EventGraph — IA_Crouch SetRelLoc nodes
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 36 | Set MaxWalkSpeed (crouch, 200) | Set bIsSprinting (35) | **Set Relative Location (100) — empty after** |
| 42 | Set MaxWalkSpeed (uncrouch, 450) | UnCrouch (40) | **Set Relative Location (101) — empty after** |
| **100** | **Set Relative Location (X=35.0, Y=0.0, Z=28.92)** | Set MaxWalkSpeed (36) | **EMPTY** |
| **101** | **Set Relative Location (X=-13.5, Y=0.0, Z=69.4)** | Set MaxWalkSpeed (42) | **EMPTY** |

### EventGraph — HandleDeath chain
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 47 | HandleDeath (Custom Event) | — | Set bIsDead (52) |
| 52 | Set bIsDead | HandleDeath | Set bIsSprinting (53) |
| 53 | Set bIsSprinting | Set bIsDead | DisableMovement (54) |
| 54 | DisableMovement | Set bIsSprinting | PrintString (55) |
| **55** | **PrintString** | DisableMovement | ShowDeathFeedback (130) |
| **130** | **ShowDeathFeedback** | PrintString (55) | **EMPTY ← Death tail** |

### EventGraph — IA_Restart chain
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 6 | IA_Restart | — | PrintString (90) |
| 90 | PrintString | IA_Restart.Started | Branch (44) |
| 44 | Branch (then) | PrintString (90) | PrintString (104) → ExecuteConsoleCommand chain |
| 104 | PrintString | Branch (44) then | ExecuteConsoleCommand (93 chain) |
| 93–99 | ExecuteConsoleCommand chain | — | SetOwnerNoSee (106) |
| 106 | SetOwnerNoSee | 99 ExecConsoleCmd | Branch (114) |
| 114 | Branch | SetOwnerNoSee | CreateWidget HUD (115) |
| **118** | **UpdateAmmo** | AddToViewport | UpdateHealth (129) |
| **129** | **UpdateHealth** | UpdateAmmo (118) | **EMPTY ← Restart tail** |

### SetPlayerStance function graph
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 9 | Set bIsCrawling | SetCapsuleHalfHeight (8) | Set Relative Location (11) |
| **11** | **Set Relative Location (X=-13.5, Y=0.0, Z=69.4)** | Set bIsCrawling (9) | **EMPTY ← Stand tail** |
| 19 | Set bIsCrawling | SetCapsuleHalfHeight (18) | Set Relative Location (21) |
| **21** | **Set Relative Location (X=35.0, Y=0.0, Z=28.92)** | Set bIsCrawling (19) | **EMPTY ← Crouch tail** |
| 27 | Set bIsCrawling | SetCapsuleHalfHeight (26) | Set Relative Location (29) |
| **29** | **Set Relative Location (X=50.0, Y=0.0, Z=5.0)** | Set bIsCrawling (27) | **EMPTY ← Prone tail** |

### EventGraph — BeginPlay chain
| idx | Node | Predecessor | then-pin |
|---|---|---|---|
| 46 | Event BeginPlay | — | Bind Event to On Death (48) |
| 48 | Bind Death | BeginPlay | Bind Health Changed (125) |
| 125 | Bind Health Changed | Bind Death | InitializeHealth (49) |
| 49 | InitializeHealth | Bind Health Changed | ExecuteConsoleCommand (93) |
| 93–99 | ExecuteConsoleCommand chain | — | SetOwnerNoSee (106) |
| **118** | **UpdateAmmo** | AddToViewport (117) | **UpdateHealth (129)** |
| **129** | **UpdateHealth** | UpdateAmmo | **EMPTY ← BeginPlay tail** |

---

## Q/F CONFLICT GUARD — ASSESSMENT

`IA_LeanLeft` and `IA_LeanRight` are independent Enhanced Input Actions. Each has Started and Completed triggers. If both Q and F are held simultaneously, both Set LeanAlpha events fire. The last one processed by UE's input system writes the final value. This results in unpredictable sign when both are held.

**Verdict:** LOW RISK for a tactical FPS (players rarely hold both keys). **No new nodes required** unless you want explicit mutual-exclusion. If you do want it:
- After Set LeanAlpha in LeanLeft.Started: add a `Get LeanAlpha` → compare == -1.0 guard before writing, OR  
- Replace both pairs with a single axis action on the input mapping.

The existing wiring is not an error — it is "last-wins" behavior.

---

## MANUAL STEPS — ALL NEW NODES

Open `BP_PlayerFoundation` in the Blueprint Editor. Use the instructions below section by section. Right-click anywhere in empty graph space to add a node unless otherwise noted.

---

### SECTION A — EventGraph: Tick Lean Chain

**Location:** EventGraph, near Event Tick cluster (around grid coordinates [100, 4000])  
**Attach after:** `Set FallVelocityZ` (node 168) — the node with an empty `then` pin at the end of the Tick chain.

Place new nodes to the **right** of node 168, starting at approximately X+200 from it.

---

#### A1 — FInterp To

```
GRAPH: EventGraph
ACTION: Right-click in empty space to the right of "Set FallVelocityZ" → search "FInterp To"
NODE: K2Node_CallFunction — Math|Float|FInterp To (function: FMath::FInterpTo)
PINS:
  Current  ← drag from: Get LeanCameraY (new VariableGet node, see A1a)
  Target   ← drag from: float * float Multiply node output (see A1b)
  DeltaTime ← drag from: Event Tick "DeltaSeconds" output pin
  InterpSpeed ← drag from: Get LeanInterpSpeed (new VariableGet, see A1c)
RETURNS: float → wire to Set LeanCameraY (A2)
```

**A1a — Get LeanCameraY getter:**
```
ACTION: Right-click → search "Get LeanCameraY" → select the getter
Wire: ReturnValue → FInterp To "Current" pin
```

**A1b — Multiply (LeanAlpha × LeanMaxOffset):**
```
ACTION: Right-click → search "float * float" (or type "*")
Wire:
  Top input  ← Get LeanAlpha (new VariableGet)
  Bottom input ← Get LeanMaxOffset (new VariableGet)
  ReturnValue → FInterp To "Target" pin
```

**A1c — Get LeanInterpSpeed getter:**
```
ACTION: Right-click → search "Get LeanInterpSpeed" → select getter
Wire: ReturnValue → FInterp To "InterpSpeed" pin
```

---

#### A2 — Set LeanCameraY

```
GRAPH: EventGraph
ACTION: Right-click → search "Set LeanCameraY" → select the SETTER (not getter)
NODE: K2Node_VariableSet for LeanCameraY
PINS:
  execute ← wire from Set FallVelocityZ "then" pin  ← THIS IS THE EXECUTION CONNECTION
  LeanCameraY (value input) ← wire from FInterp To return value (A1)
  then → wire to Set Relative Location (A3) execute pin
```

**Execution wire:** Drag from `Set FallVelocityZ.then` → `Set LeanCameraY.execute`

---

#### A3 — Make Vector

```
GRAPH: EventGraph
ACTION: Right-click → search "Make Vector"
NODE: K2Node_MakeStruct — Make Vector (MakeVector)
PINS:
  X ← Get TargetCameraX (new VariableGet)
  Y ← Get LeanCameraY (new VariableGet — reads the JUST-UPDATED value from A2's output or fresh getter)
  Z ← Get TargetCameraZ (new VariableGet)
RETURNS: Vector → wire to Set Relative Location "New Location" (A4)
```

> NOTE: Wire Y from a NEW `Get LeanCameraY` getter placed AFTER Set LeanCameraY — not before it.  
> Alternatively: wire `Set LeanCameraY`'s output value pin (the small white pin next to the variable name on the setter) directly to Make Vector Y. This avoids needing a second getter.

---

#### A4 — Get FirstPersonCamera (variable getter)

```
GRAPH: EventGraph
ACTION: Right-click → search "Get FirstPersonCamera" (or drag from My Blueprint panel → Variables → FirstPersonCamera)
NODE: K2Node_VariableGet for FirstPersonCamera
PINS:
  ReturnValue → wire to Set Relative Location "Target" pin (A5)
```

---

#### A5 — Set Relative Location (called on FirstPersonCamera)

```
GRAPH: EventGraph
ACTION: Drag from the "Get FirstPersonCamera" output pin → release in empty space → search "Set Relative Location"
  (This auto-wires Target. Alternatively right-click → search "SetRelativeLocation" → then manually wire Target.)
NODE: K2Node_CallFunction — Components|SceneComponent|Set Relative Location
PINS:
  execute ← wire from Set LeanCameraY "then" pin
  Target  ← wire from Get FirstPersonCamera (A4)
  New Location ← wire from Make Vector return value (A3)
  bSweep = false (leave unchecked / default)
  bTeleport = false (leave unchecked / default)
  then → (leave empty — end of Tick chain)
```

**Full execution chain for Tick lean:**
```
Set FallVelocityZ.then → Set LeanCameraY.execute → Set Relative Location.execute
```
(Set LeanCameraY fires first; Set Relative Location fires after, reading the updated Y.)

---

### SECTION B — SetPlayerStance: Set TargetCamera variables after each SetRelLoc

The 3 SetRelLoc nodes (idx 11, 21, 29) each have an empty `then` pin.  
After each, add a chain of 3 Set nodes: TargetCameraX, TargetCameraY, TargetCameraZ.

> IMPORTANT: `SetRelativeLocation` is a **call function** node (orange), not a variable set. Its `then` pin is the exec output at the bottom of the node.

---

#### B1 — After Stand SetRelLoc (idx 11, NewLocation = X=-13.5, Y=0, Z=69.4)

```
GRAPH: SetPlayerStance
LOCATION: Right of the SetRelLoc node that has NewLocation = (X=-13.5, Y=0.0, Z=69.4)

Add node: Set TargetCameraX
  execute ← Stand SetRelLoc "then" pin
  TargetCameraX value = -13.5
  then → Set TargetCameraY

Add node: Set TargetCameraY
  execute ← Set TargetCameraX "then"
  TargetCameraY value = 0.0
  then → Set TargetCameraZ

Add node: Set TargetCameraZ
  execute ← Set TargetCameraY "then"
  TargetCameraZ value = 69.4
  then → (leave empty)
```

**Execution chain:**
```
Set bIsCrawling(9).then → SetRelLoc(11).execute → SetRelLoc(11).then
  → Set TargetCameraX(-13.5).then → Set TargetCameraY(0.0).then → Set TargetCameraZ(69.4)
```

---

#### B2 — After Crouch SetRelLoc (idx 21, NewLocation = X=35.0, Y=0, Z=28.92)

```
GRAPH: SetPlayerStance
LOCATION: Right of the SetRelLoc node that has NewLocation = (X=35.0, Y=0.0, Z=28.92)

Add node: Set TargetCameraX
  execute ← Crouch SetRelLoc "then" pin
  TargetCameraX value = 35.0
  then → Set TargetCameraY

Add node: Set TargetCameraY
  execute ← Set TargetCameraX "then"
  TargetCameraY value = 0.0
  then → Set TargetCameraZ

Add node: Set TargetCameraZ
  execute ← Set TargetCameraY "then"
  TargetCameraZ value = 28.92
  then → (leave empty)
```

---

#### B3 — After Prone SetRelLoc (idx 29, NewLocation = X=50.0, Y=0, Z=5.0)

```
GRAPH: SetPlayerStance
LOCATION: Right of the SetRelLoc node that has NewLocation = (X=50.0, Y=0.0, Z=5.0)

Add node: Set TargetCameraX
  execute ← Prone SetRelLoc "then" pin
  TargetCameraX value = 50.0
  then → Set TargetCameraY

Add node: Set TargetCameraY
  execute ← Set TargetCameraX "then"
  TargetCameraY value = 0.0
  then → Set TargetCameraZ

Add node: Set TargetCameraZ
  execute ← Set TargetCameraY "then"
  TargetCameraZ value = 5.0
  then → (leave empty)
```

---

### SECTION C — EventGraph IA_Crouch chain: Set TargetCamera after SetRelLoc nodes

The EventGraph has two SetRelLoc nodes that fire on `IA_Crouch` (nodes 100 and 101). Both have empty `then` pins.

---

#### C1 — After IA_Crouch SetRelLoc (idx 100, X=35.0, Y=0, Z=28.92)

```
GRAPH: EventGraph
LOCATION: Right of the SetRelLoc node wired after Set MaxWalkSpeed (crouch, 200 speed)
  — this node's NewLocation = (X=35.0, Y=0.0, Z=28.92)

Add node: Set TargetCameraX = 35.0
  execute ← SetRelLoc(100) "then"
  then → Set TargetCameraY

Add node: Set TargetCameraY = 0.0
  then → Set TargetCameraZ

Add node: Set TargetCameraZ = 28.92
  then → (empty)
```

---

#### C2 — After IA_Crouch SetRelLoc (idx 101, X=-13.5, Y=0, Z=69.4)

```
GRAPH: EventGraph
LOCATION: Right of the SetRelLoc node wired after Set MaxWalkSpeed (uncrouch, 450 speed)
  — this node's NewLocation = (X=-13.5, Y=0.0, Z=69.4)

Add node: Set TargetCameraX = -13.5
  execute ← SetRelLoc(101) "then"
  then → Set TargetCameraY

Add node: Set TargetCameraY = 0.0
  then → Set TargetCameraZ

Add node: Set TargetCameraZ = 69.4
  then → (empty)
```

---

### SECTION D — BeginPlay initialization

**Purpose:** Initialize TargetCameraX/Y/Z and lean vars from the camera's actual state at game start, so the first Tick lean fires correctly before any SetPlayerStance call.

**Location:** EventGraph, after the last node in the BeginPlay chain.  
The BeginPlay chain currently ends at `UpdateHealth` (node 129, empty `then`).

```
GRAPH: EventGraph
LOCATION: Right of UpdateHealth (node 129, end of BeginPlay/Restart chain)

Add node: Set TargetCameraX = -13.5
  execute ← UpdateHealth(129) "then"
  then → Set TargetCameraY

Add node: Set TargetCameraY = 0.0
  execute ← Set TargetCameraX "then"
  then → Set TargetCameraZ

Add node: Set TargetCameraZ = 69.4
  execute ← Set TargetCameraY "then"
  then → Set LeanCameraY

Add node: Set LeanCameraY = 0.0
  execute ← Set TargetCameraZ "then"
  then → Set LeanAlpha

Add node: Set LeanAlpha = 0.0
  execute ← Set LeanCameraY "then"
  then → (empty)
```

> NOTE: This chain also runs on restart (node 129 is shared between BeginPlay and IA_Restart paths — both converge at UpdateAmmo → UpdateHealth → tail).

---

### SECTION E — Death reset

**Purpose:** On HandleDeath, reset lean state so the death camera (DisableMovement active) does not retain a mid-lean position.

**Location:** EventGraph, after `ShowDeathFeedback` (node 130, empty `then`).

```
GRAPH: EventGraph
LOCATION: Right of ShowDeathFeedback (node 130, end of HandleDeath chain)

Add node: Set LeanAlpha = 0.0
  execute ← ShowDeathFeedback(130) "then"
  then → Set LeanCameraY

Add node: Set LeanCameraY = 0.0
  execute ← Set LeanAlpha "then"
  then → (empty)
```

> Do NOT reset TargetCameraX/Y/Z here — the camera will not be moving on death, and the death animation or spectate camera may need the last valid stance position.

---

## COMPILE AND SAVE

After completing all manual steps:

1. Click **Compile** (toolbar, top-left of BP editor)
2. Verify: green checkmark, no errors in Output Log
3. Click **Save** (Ctrl+S)
4. Run PIE (`Play` button, not Simulate) with player possession

---

## ACCEPTANCE TEST (PIE with player)

| Test | Expected result |
|---|---|
| Press Q — hold — release | Camera slides left ~35 cm, snaps back smoothly |
| Press F — hold — release | Camera slides right ~35 cm, snaps back smoothly |
| Press Q while crouched | Camera leans left from crouch eye height (Z=28.92), not standing height |
| Press Q while prone | Camera leans left from prone eye height (Z=5.0) |
| Transition Standing → Crouch while leaning | Camera height updates; lean offset adjusts (no Z fight) |
| Die (take enough damage) | Camera stops leaning, stays at death position |
| Press Restart after death | Camera returns to standing, no lean |
| Hold Q + F simultaneously | Camera follows last-pressed key (acceptable; not a bug) |

---

## STATUS LABELS

| System | Status |
|---|---|
| TargetCameraX, TargetCameraY variables | IMPLEMENTED |
| All variables compiled into BP | COMPILED: True |
| Tick lean chain (FInterpTo → SetRelLoc) | USER TEST REQUIRED — nodes not creatable via Python |
| SetPlayerStance TargetCamera updates | USER TEST REQUIRED |
| IA_Crouch TargetCamera updates | USER TEST REQUIRED |
| BeginPlay / Restart lean init | USER TEST REQUIRED |
| Death lean reset | USER TEST REQUIRED |
| Node creation via Python | BLOCKED: `add_call_function_node`, `add_variable_set_node`, `add_variable_get_node` not exposed in UE 5.8.4 headless Python binding (EdGraph.nodes not iterable, no node-factory APIs available) |

---

## VARIABLE DEFAULTS REFERENCE

| Variable | Default | Purpose |
|---|---|---|
| LeanAlpha | 0.0 | -1.0 = left, +1.0 = right, 0.0 = center |
| LeanCameraY | 0.0 | Current interpolated Y offset (socket-local) |
| LeanMaxOffset | 35.0 | Max Y offset in socket-local cm |
| LeanInterpSpeed | 8.0 | FInterp speed; higher = snappier |
| TargetCameraX | -13.5 | Standing X. Set per stance by SetPlayerStance |
| TargetCameraY | 0.0 | Baseline Y (CDO socket centre-line; not lean offset) |
| TargetCameraZ | 69.4 | Standing Z. Set per stance by SetPlayerStance |

---

## KNOWN LIMITS

- **Lean axis unverified in live PIE.** Prior static analysis confirms +Y = right in socket-local space. If lean direction is inverted at runtime, negate LeanAlpha before multiplying (or swap the -1.0/+1.0 assignments on Q/F).
- **LeanMaxOffset CDO = 35.0** was confirmed in Batch 2R. Adjust to taste via Details panel; 35 cm is a noticeable but not extreme lean.
- **Q+F simultaneous = last-wins.** Not a bug for tactical gameplay; can be made explicit with a mutual-exclusion guard if desired (see Q/F assessment above).
- **BeginPlay chain is shared with IA_Restart** — both paths converge at node 129 (`UpdateHealth`). The lean init nodes added after 129 will fire on both game start and player restart. This is correct and intentional.
- **TargetCameraY is set to 0.0 by all SetPlayerStance branches.** The CDO default 5.9 (socket lateral offset) is intentionally not carried through stance transitions, matching the existing SetRelLoc nodes which all write Y=0.0.
