# Phase 4: equipment and throwables

The existing `BP_Weapon_Base` remains the firing implementation. A native equipment component supplies state gating, finite reloads, two independent slots, weapon definitions, modes, ADS and handling around that Blueprint. The alternate is a child of the original weapon, with a compact graybox shape, separate material and its own capacity/reserve. No sample inventory framework or external animation pack was imported.

## Behavior

Semi-auto requires a new trigger press, auto repeats within the configured cooldown, and burst finishes three rounds from a short press. Reload transfers the minimum of missing magazine capacity and available reserve. Empty/full/no-reserve cases, inspection, switching, holstering, death and traversal have explicit gates. Recoil, state-dependent spread, ADS FOV, actual mouse/controller-action sensitivity scaling, crosshair visibility, muzzle obstruction, muzzle light, impact sparks and bounded physical shell ejections are integrated.

Definition assets configure actor class, capacity, reserve, damage, interval and initial mode. The component exposes an available-definition list and slot limit for later story content. Actor classes should inherit the existing weapon Blueprint and retain its ammo fields and `TryFire` contract. Pickups and loot transact through phase 3. Dropped weapons retain their magazine/reserve, and collecting the first firearm into an empty loadout draws it again. World drops have unique persistent identities for phase 7.

Hitscan damage has range falloff and head/limb multipliers. The simple target collider does not return bone names, so a rig-based height fallback handles those targets. Optional projectile mode uses an actual swept projectile. Optional penetration is limited to one explicitly tagged cover no thicker than 25 cm; thick and untagged covers block it. These are bounded prototypes, not a claim of a complete real-world ballistics model.

Stones, knives, frag, smoke and flash share a throwable actor. Holding/releasing uses a visible physical trajectory preview and spends finite inventory. Rigid-body throws exclude their owner's capsule through a free constraint; move-ignore lists alone do not suppress physics contacts. Meaningful collision impulses produce hearing events, while resting contacts stay quiet. Knives embed and can be retrieved once. Grenades retain a running fuse while held and after release. Frag damage respects blocking geometry and can kill the owner when cooked in hand. Smoke is a bounded translucent placeholder sphere; flash is a line-of-sight/direction-aware view overlay that fades. Neither effect is final artwork.

## Controls

- LMB: fire; RMB: ADS; B: cycle semi/auto/burst.
- R: reload while alive; retain the existing R restart behavior when dead.
- 1/2 or wheel: switch; X: drop; H: holster/draw; I: inspect.
- E: shared world pickups and knife/stone retrieval.
- T: hold to prepare/charge, release to throw. Holding a grenade cooks its fuse; the HUD shows the remaining time.
- V: cycle throwable; N: select knife; M: select stone. G remains traversal.

## Evidence and review

`Tools/Foundation/test_phase4_equipment.py`, `test_phase4_ballistics.py`, `test_phase4_throwables.py` and `audit_phase4.py` produce the raw observations and assertions. Final results, screenshots and build logs are under `Phase4Evidence`; `checks.json` is the count source. Fixture teleports, finite ammo arrangements and reversible definition flags set up independent scenarios; actual input, collision, damage and inventory transitions provide outcomes.

The tests exercise both weapons, modes/cooldown, finite transfers, failed states, ADS/look scaling, swap/drop/reclaim, empty-loadout recovery, projectile impact, penetration bounds, damage zones/falloff, trajectory visibility, rigid-body impacts, retrieval, grenade pickups, radial damage with/without the same wall, smoke, flash and in-hand fuse risk. Editor/Game builds are distinct from packaged acceptance, which belongs to phase 8.

Owner approval is not inferred. Hand/equipment motion is procedural prototype animation and needs visual comfort review. Sound cues are original synthesized placeholders; the hardware audio device changed during testing, so mixer state is not audible owner acceptance. Stone hearing must still be tested against phase-5 AI, and later tactical restrictions/save/load remain integration work. Inspect `checks.json` rather than treating this report as a blanket approval of all 140 actions.

Recovery points are the pre-phases backup and `D:\GameDev\Backups\Foundation_After_Phase3_20261118`. Existing art and uncommitted work were retained. MIT notices for the previously adapted interaction/movement excerpts remain in `ThirdPartyNotices`; this equipment/throwable implementation is original project code using the installed engine API.
