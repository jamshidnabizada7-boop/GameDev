# PHASE 1 NEXT STEPS — Afghanistan FPS
Ordered by priority. Anything marked [manual] needs a person in the editor.

1. **Finish interaction system** (~10 min, mostly [manual])
   a. [manual] Open `BP_TestInteractable` → Class Settings → Interfaces → Add →
      `BPI_Interactable` → Compile → Save.
   b. Run via editor Cmd console:
      `py "C:/Users/PC/Documents/Default Project/zcode_phase1/zcode_interact_impl.py"`
      (provided in the handover folder; adds the Interact event override +
      scale-toggle/PrintString feedback, compiles, saves, verifies
      does_implement_interface by spawning an instance).
   c. PIE: look at the cube within 2.2 m, press E → cube scales 1↔2 + message.
      Verify out-of-range (>250 cm) and wall-blocked cases fail.

2. **[manual] SkyLight recapture** — select SkyLight in the test map → Recapture Sky.
   Removes the Lumen cached-lighting warning; makes the room visible in PIE.

3. **[manual] Live input pass** — PIE the test map and confirm: WASD walk (~450 cm/s),
   Shift sprint (~700 cm/s), release restores, Ctrl crouch (capsule shrinks), jump,
   R after death restarts. Everything behind these keys is verified working except
   the key delivery itself.

4. **Verify OnHealthChanged/OnDeath evidence** — add temporary PrintStrings bound to
   the dispatchers, or accept the indirect verification (see Known Issues #9).

5. **Git checkpoint (see Handover for exact commands)** — commit all changed/new
   assets + Docs as `Phase1-01: player foundation, input, test map, damage/death/
   restart verified in PIE`, push to `backup` remote.

6. **Cleanup (after checkpoint)**
   - Delete `_Scratch` probe BPs (8) via editor asset tools.
   - Delete the empty Door/JumpPad/Materials/Test folders under Interaction.
   - Optional: tidy orphan pure nodes in BP_PlayerFoundation EventGraph.

7. **Then Phase 1 remainder** (per original plan): manual animation visual check
   (ABP_Unarmed/ABP_FP_Copy), the 30-item checklist manual pass, reports refresh,
   final commit.
