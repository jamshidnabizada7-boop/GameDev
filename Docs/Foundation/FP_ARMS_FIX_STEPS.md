# FIRST-PERSON ARMS FIX STEPS
## Batch 2R Item 3: Fix FP Arms Visibility

**Investigation complete (editor live inspection, 2026-11-17).**

---

## Confirmed State

| Property | Value |
|---|---|
| Component | `FirstPersonMesh` |
| Mesh | `SKM_Manny_Simple` (full-body Manny) |
| `only_owner_see` | `True` ✓ |
| `hidden_in_game` | `False` ✓ |
| Relative location | 0, 0, 0 (relative to `CharacterMesh0`) |
| Anim BP | `ABP_FP_Copy` (CopyPoseFromMesh → Root) ✓ |
| Camera parent | `FirstPersonMesh` (head socket) |
| Weapon mount offset | X=32, Y=15, Z=-13 (relative to `FirstPersonCamera`) |

**Problem:** Full-body mesh renders from camera position. Head, torso, pelvis, and legs are visible in FP view.

**Root cause:** No dedicated FP arms mesh. `SKM_Manny_Simple` must have unwanted bones hidden at runtime.

**Python limitation:** UE 5.8.4 Python API cannot add new Blueprint graph nodes. Fix must be done manually in the editor.

---

## Fix: HideBoneByName in BeginPlay

### BeginPlay chain tail (confirmed live)
The BeginPlay exec chain ends at node `K2Node_CallFunction_72` ("**UpdateHealth**").

### Steps

1. Open `BP_PlayerFoundation` (double-click in Content Browser).
2. Open the **EventGraph** tab.
3. Find the **UpdateHealth** node at the end of the BeginPlay chain (search for it, it's the last node wired from BeginPlay).
4. Right-click after **UpdateHealth** → search for **"Get FirstPersonMesh"** → add it.
5. Drag off the `FirstPersonMesh` pin → search **"Hide Bone by Name"** → add it.
6. Set **Bone Name** = `pelvis`  |  **Physics Blend Weight** = `0.0`
7. Wire **UpdateHealth → then** → **HideBoneByName.execute**
8. Drag off **HideBoneByName.then** → add another **Hide Bone by Name** node.
9. Repeat step 5–8 for each bone below, chaining `then → execute`:

| Order | Bone Name |
|---|---|
| 1 | `pelvis` |
| 2 | `thigh_l` |
| 3 | `calf_l` |
| 4 | `foot_l` |
| 5 | `thigh_r` |
| 6 | `calf_r` |
| 7 | `foot_r` |
| 8 | `spine_01` |
| 9 | `spine_02` |
| 10 | `spine_03` |
| 11 | `neck_01` |
| 12 | `head` |

> **All `Physics Blend Weight` values = `0.0`**

10. Each `HideBoneByName` node uses the **same** `FirstPersonMesh` reference (you can re-use the same GetComponent output pin, or add a new Get each time).
11. Click **Compile** (top-left of Blueprint editor).
12. Click **Save**.

---

## Expected PIE Result

- Arms and shoulders visible from FP camera.
- Legs, torso, neck, and head hidden.
- Camera stable at eye level (camera is parented to head socket — hiding the bone doesn't move the socket).
- Weapon at X=32, Y=15, Z=-13 from camera (existing, no change needed).

---

## If Clipping Still Occurs After Bone Hiding

The `clavicle_l` / `clavicle_r` bones may show shoulder blobs. Add two more:
- `clavicle_l`
- `clavicle_r`

---

## Weapon Alignment

`FP_WeaponMount` is at (32, 15, -13) relative to `FirstPersonCamera`. This positions the weapon slightly forward, right, and below center. If the weapon floats or clips badly in PIE, adjust:
- Move `FP_WeaponMount` in the Components panel → Details → Transform.
- Target: weapon appears at center of screen, slightly below crosshair, at arm reach distance.

---

## Status

Manual editor steps required. Python headless path confirmed blocked for graph node creation in UE 5.8.4.

**After completing the steps above:**
- Compile + Save
- PIE test: confirm arms visible, no head/torso blocking view
- Report acceptance
