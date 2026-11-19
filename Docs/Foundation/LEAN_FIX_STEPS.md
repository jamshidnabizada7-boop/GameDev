# Lean Fix — Editor Steps
**BP_PlayerFoundation** — open in Blueprint Editor before starting.
All variables already exist. You are only adding nodes and wires.
Compile + Save after every section to catch errors early.

---

## SECTION A — EventGraph: Tick lean chain
**Goal:** Every Tick, interpolate `LeanCameraY` toward `LeanAlpha × LeanMaxOffset`,
then call `SetRelativeLocation` on `FirstPersonCamera` with `(TargetCameraX, LeanCameraY, TargetCameraZ)`.

**Find:** `Set FallVelocityZ` (node 168) — it has an empty `then` pin. All new nodes go to its right.

### A1 — Multiply node (LeanAlpha × LeanMaxOffset)
1. Right-click empty space → search **float \* float** → add it
2. Left pin ← drag `Get LeanAlpha` getter
3. Right pin ← drag `Get LeanMaxOffset` getter

### A2 — FInterp To node
1. Right-click → search **FInterp To** → add it
2. `Current` ← drag `Get LeanCameraY` getter
3. `Target` ← wire from Multiply output
4. `DeltaTime` ← drag from `Event Tick` node's **DeltaSeconds** output pin
5. `InterpSpeed` ← drag `Get LeanInterpSpeed` getter

### A3 — Set LeanCameraY (setter)
1. Right-click → search **Set LeanCameraY** → choose the **setter** (purple node)
2. **Exec wire:** drag from `Set FallVelocityZ` → **then** pin → `Set LeanCameraY` → **execute** pin
3. Value input ← wire from `FInterp To` return value

### A4 — Make Vector
1. Right-click → search **Make Vector** → add it
2. `X` ← drag `Get TargetCameraX` getter
3. `Y` ← drag from `Set LeanCameraY` **output value pin** (small white/blue pin on the left of the setter label)
   - _Or place a second `Get LeanCameraY` getter — both are correct_
4. `Z` ← drag `Get TargetCameraZ` getter

### A5 — Get FirstPersonCamera
1. Right-click → search **Get FirstPersonCamera** → add getter
   - _or drag from My Blueprint → Variables → FirstPersonCamera_

### A6 — Set Relative Location (on FirstPersonCamera)
1. Drag from `Get FirstPersonCamera` output → release in empty space → search **Set Relative Location** → add it
2. `Target` is auto-wired from step above. If not, wire `Get FirstPersonCamera` → `Target`
3. `New Location` ← wire from `Make Vector` output
4. **Exec wire:** drag from `Set LeanCameraY` → **then** → `Set Relative Location` → **execute**
5. Leave `bSweep` and `bTeleport` unchecked (defaults)

**Full exec chain:**
```
Set FallVelocityZ → Set LeanCameraY → Set Relative Location
```

**Compile now. Fix any errors before continuing.**

---

## SECTION B — SetPlayerStance: record TargetCamera after each SetRelLoc
**Graph:** Open **SetPlayerStance** function tab.
Three SetRelLoc nodes each have an empty `then`. After each, add three Set nodes.

### B1 — Stand branch (SetRelLoc with X=-13.5, Z=69.4)
Wire from that node's `then` pin:
```
Set TargetCameraX = -13.5
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 69.4
```

### B2 — Crouch branch (SetRelLoc with X=35.0, Z=28.92)
Wire from that node's `then` pin:
```
Set TargetCameraX = 35.0
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 28.92
```

### B3 — Prone branch (SetRelLoc with X=50.0, Z=5.0)
Wire from that node's `then` pin:
```
Set TargetCameraX = 50.0
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 5.0
```

**Compile now.**

---

## SECTION C — EventGraph: IA_Crouch SetRelLoc nodes
**Graph:** Back in **EventGraph**.
Two SetRelLoc nodes fire when crouching/uncrouching (nodes 100 and 101). Both have empty `then`.

### C1 — After node 100 (X=35.0, Z=28.92) — this fires when CROUCHING
Wire from that node's `then`:
```
Set TargetCameraX = 35.0
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 28.92
```

### C2 — After node 101 (X=-13.5, Z=69.4) — this fires when UNCROUCHING
Wire from that node's `then`:
```
Set TargetCameraX = -13.5
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 69.4
```

**Compile now.**

---

## SECTION D — BeginPlay / Restart init
**Graph:** EventGraph.
Find `UpdateHealth` (node 129) — it has an empty `then`. This is shared by BeginPlay and Restart.

Wire from node 129's `then`:
```
Set TargetCameraX = -13.5
  → Set TargetCameraY = 0.0
    → Set TargetCameraZ = 69.4
      → Set LeanCameraY = 0.0
        → Set LeanAlpha = 0.0
```

---

## SECTION E — Death reset
**Graph:** EventGraph.
Find `ShowDeathFeedback` (node 130) — empty `then`. Wire from it:
```
Set LeanAlpha = 0.0
  → Set LeanCameraY = 0.0
```
_(Do NOT reset TargetCameraX/Y/Z here.)_

---

## FINAL COMPILE AND TEST

1. **Compile** — green checkmark, zero errors
2. **Save** (Ctrl+S)
3. **Play** (PIE, full possession — not Simulate)

| Action | Expected |
|---|---|
| Hold Q | Camera slides left ~35 cm smoothly |
| Release Q | Camera returns to center |
| Hold F | Camera slides right ~35 cm |
| Release F | Returns to center |
| Q while crouched | Leans from crouch height (Z≈28.92), not standing |
| Q while prone | Leans from prone height (Z≈5.0) |
| Take enough damage to die | Lean resets to center |
| Press Restart | Camera at standing height, no lean |

---

## IF LEAN DIRECTION IS INVERTED
Swap the values on the input nodes:
- `IA_LeanLeft.Started` → change Set LeanAlpha from **-1.0** to **+1.0**
- `IA_LeanRight.Started` → change Set LeanAlpha from **+1.0** to **-1.0**

---

## KNOWN LIMITS
- **Axis direction unverified in live PIE** — +Y socket-local should be right, but confirm with the swap above if wrong.
- **Q+F both held** = last-wins (not a bug for tactical play).
- **No wall penetration check** — basic check only; full collision is a follow-up task.
