# PHASE 1 NEXT STEPS — Afghanistan FPS
Updated 2026-11-14 (input-chain repair session). Both root causes are fixed:
(1) IMC mappings persisted, (2) the input chains rewired after the debug prints
severed them. All gameplay chains are machine-verified end-to-end with runtime
measurements (zcode_final_run2.json). What remains is a short **manual
acceptance pass** — key delivery to PIE is not automatable in this environment.

1. **[manual, ~5 min] Live input acceptance pass** — PIE `L_Test_PlayerFoundation` and:
   - click the viewport once (game captures input; "Shift+F1" hint appears)
   - WASD walk (~450 cm/s), Shift sprint (~700 cm/s), release → walk speed restores
   - Ctrl crouch (capsule shrinks), Ctrl release → stands; low tunnel blocks standing
     (don't test UnCrouch while wedged against the interact cube — UE blocks it
     correctly there; step away first)
   - Space jump
   - look at the blue-tinted cube (2.2 m ahead of spawn), press **E** →
     cube scales 1.0 → 1.25 → 1.0 on repeated presses
   - walk into the damage volume repeatedly → HP 100→75→50→25→0 → "YOU DIED" →
     press **R** → HP back to 100
   The debug prints are wired IN SERIES (event → print → gameplay), so every
   keypress prints its "INPUT: ..." message AND performs the action. If any
   action fails while its print appears, report which.

2. **[manual] Animation visual check** — PIE and confirm: idle pose, walk cycle,
   jump/fall/land poses (ABP_FP_Copy + SKM_Manny_Simple verified wired). No
   dedicated sprint/crouch anims (documented placeholders).

3. **Verify SkyLight** — recapture was scripted and the map saved (zcode_sky3.json);
   open PIE and confirm the Lumen cached-lighting warning is gone.

4. **After editor close**: confirm the `_Scratch` files flushed from disk
   (deletion was accepted by the editor; on-disk unlink is deferred while loaded).
   BPC_ProbeHealth.uasset and BP_ProbeHealthHost.uasset were still on disk at the
   last check — delete them in the Content Browser if they remain.

5. **Phase 2 planning** — Phase 1 acceptance is complete pending the manual pass
   above. No other known blockers.
