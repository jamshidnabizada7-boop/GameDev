# PHASE 1 NEXT STEPS — Afghanistan FPS
Updated 2026-11-14 (video-analysis session). Fixed this session: the E-interaction
zero-length trace, the floating-cube camera clip (black screen), plus a live debug
HUD. What remains is the user's manual acceptance pass.

1. **[manual, ~5 min] Live input acceptance pass** — open
   Content/AfghanFPS/TestMaps/L_Test_PlayerFoundation, PIE, click the viewport:
   - W walk (~450 cm/s on the Velocity readout), Shift+W sprint (~700), release restores
   - Ctrl crouch (CapsuleHalfHeight 96 -> 40 on the readout), release -> 96
   - Space jump (clear space, not under the tunnel slab)
   - Look at the cube ON THE FLOOR ~2 m ahead (aim slightly down), press E:
     the blue debug ray is drawn each press; cube scales 1.0 -> 1.25 -> 1.0
   - Walk into the damage volume at the far side repeatedly: HP readout 100->75->50->25->0,
     "YOU DIED" prints; movement/interaction stop; press R -> HP back to 100
   - R while alive does nothing by design (restart branch is gated on bIsDead)
   - Health/death readouts come from the new DisplayAll HUD (updates every frame;
     lines persist until editor restart — that is normal DisplayAll behavior)

2. **[manual] Animation visual check** — arms visible and animating in idle/walk/jump
   (ABP_FP_Copy + SKM_Manny_Simple verified wired). Sprint/crouch have no dedicated
   anims (documented placeholders).

3. **[manual] SkyLight warning** — confirm the Lumen cached-lighting warning is gone
   on PIE open (recaptured + saved earlier).

4. **Cleanup after editor close** — confirm _Scratch folder stays empty (leftovers
   were removed from the repo in commit 8798cb7).

5. **Phase 2 planning** — Phase 1 acceptance complete pending the manual pass above.
   Do not begin Phase 2 before it.
