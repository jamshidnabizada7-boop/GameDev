# Phase 3: reusable world interaction

The current player Blueprint now uses one native interaction component and a Blueprint-extensible interface. Its original health, ammunition HUD, weapon and interaction cube remain in place. The latest user request authorizes continuing phases 3–8 in sequence; the attached documents supply scope, and their older per-batch stop instructions do not create a new approval gate. Owner approval remains separate from technical checks.

## Implemented

Nine Blueprint actor classes cover hinged doors, drawers, light switches, loot, breakable crates, ammunition, health, world weapons and keys/tools. The same graybox map has a labeled eastern annex. No historical mission or final artwork was added. Pickups use one-shot transactions and become hidden and non-colliding after collection; their stable identities remain available for the later save system. A world weapon currently grants an ownership token; equipping and dropping it belong to phase 4.

Camera focus uses simple visibility collision at 250 cm, validates distance and line of sight during holds, and rejects use during death or traversal. Immediate, timed hold and continuous interaction share cancellation and prompt state. The compatibility adapter passes the original `Interactor` argument to `BPI_Interactable`, rather than replacing that interface or its cube.

Doors support tap-to-toggle, continuous five-degree wheel adjustments, key unlocking, a timed lockpick alignment prototype and collision checks at intermediate hinge angles. Holding E at a shut door changes two actual spatial audio components: exterior volume falls from .15 to .09, while interior low-pass filtering is removed and volume rises from .2 to .6. Original synthesized placeholder cues accompany successful uses. Switches can also control a list of external light actors. Crates use a damage-driven mesh swap with non-colliding debris.

Crouch plus E switches to a bounded under-door camera showing the real hidden room. This lower-cost fallback renders one active view, without a SceneCapture render target. E returns to the player, and invalid range, stance or death also exits. Mouse pan is limited to 35 degrees horizontally and -15/+20 vertically. Leaning at a partly open door interpolates FOV to 65 degrees and restores the original FOV on release.

## Evidence and limits

The reproducible checks are in `Tools/Foundation/test_phase3.py`, `test_phase3_edges.py` and `audit_phase3.py`. The final counts, raw observations, screenshots and build logs are under `Phase3Evidence`; consult `checks.json` for actual outcomes. Tests use possessed PIE, real collision frames and generated key events through Enhanced Input. Teleports and fresh fixture states arrange scenarios; they are not substituted for gameplay outcomes.

The tests cover denied locks, key use, wheel angle retention, peek FOV, listening state, health/reserve transactions, cancellation, loot, switches, drawers, breakage, the original cube, mirror entry/exit, lockpicking and a blocked door sweep. Editor and Game target builds are separate from packaging; no packaged executable acceptance is claimed here.

The mirror frame samples are whole-frame measurements from this test session, not isolated GPU overhead or a hardware performance guarantee. Room audio is an original looping placeholder, not a walking NPC simulation; that source is completed with later AI integration. Subjective audibility, physical owner input, camera comfort and animation polish still require review. Pickup reaching uses procedural hand IK rather than final authored interaction animations. No owner approval is inferred.

## Controls and owner review

- E: use; hold E: pickups, loot or listening; tap E: open/close a door.
- Hold E and wheel: adjust a door in five-degree steps; align a locked door's pick at 35 degrees when carrying the tool.
- Q/F at a door crack: lean and peek. Release to restore the camera.
- Hold Ctrl, then tap E at a shut door: tactical mirror; mouse pans; E exits.

In the eastern annex, collect the armory key and open its door. Compare listening on/off with sound enabled, adjust the other door to a small crack and release lean, then crouch and inspect the dummy through the mirror. Try releasing a pickup early and moving out of range. Check that health/ammo remain legible and the original cube still changes scale. Report comfort or audio issues; the automated evidence does not approve these subjective checks.

## Reuse and recovery

Only FPSCore's reviewed camera-trace/interface-dispatch pattern was adapted, from commit `97993c21c5519628f2462e2660930f0483913b17`. Its complete MIT notice is in `ThirdPartyNotices/FPSCore-LICENSE.txt`; exact files and boundaries are recorded in `ThirdPartyNotices/REUSE.md`. No donor framework or binary assets were installed. Retaining the copyright and license notices is required.

The pre-phase-3–8 recovery copy is `D:\GameDev\Backups\Foundation_Phases3_8_20261117_231242`. Existing uncommitted work is preserved. No commit, push, engine upgrade, global rendering change or cache cleanup was performed.
