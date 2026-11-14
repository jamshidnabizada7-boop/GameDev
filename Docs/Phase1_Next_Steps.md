# PHASE 1 NEXT STEPS — Afghanistan FPS
Updated 2026-11-14 (final autonomous session). Everything automatable is done and
verified. Remaining items are a short **manual acceptance pass** in the editor.

1. **[manual, ~5 min] Live input acceptance pass** — PIE `L_Test_PlayerFoundation` and:
   - click the viewport once (game captures input; "Shift+F1" hint appears)
   - WASD walk (~450 cm/s), Shift sprint (~700 cm/s), release → walk speed restores
   - Ctrl crouch (capsule shrinks), Ctrl release → stands; low tunnel blocks standing
   - Space jump
   - look at the blue-tinted cube (2.2 m ahead of spawn), press **E** →
     cube scales 1.0 → 1.25 → 1.0 on repeated presses
   - walk into the damage volume repeatedly → HP 100→75→50→25→0 → "YOU DIED" →
     press **R** → HP back to 100
   All systems behind these keys are individually machine-verified (see
   Phase1_Test_Results.md); this pass confirms the physical keyboard layer.

2. **[manual] Animation visual check** — PIE and confirm: idle pose, walk cycle
   (ABP_Unarmed drives the mesh), jump/fall/land poses. 97 mannequin anim sequences
   exist in the registry; no dedicated sprint/crouch anims (documented placeholders —
   no dedicated animations were required for Phase 1).

3. **Verify SkyLight** — recapture was scripted and the map saved (zcode_sky3.json);
   open PIE and confirm the Lumen cached-lighting warning is gone.

4. **After editor close**: confirm the 6 remaining `_Scratch` files flushed from disk
   (deletion was accepted by the editor; on-disk unlink is deferred while loaded).
   If any remain, delete them in the Content Browser.

5. **Phase 2 planning** — Phase 1 acceptance is complete pending the manual pass
   above. No other known blockers.
