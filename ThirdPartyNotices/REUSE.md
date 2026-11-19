# Reuse review record

Only reviewed source excerpts were adapted; neither framework was installed.

- ALS Refactored https://github.com/Sixze/ALS-Refactored commit b754d6f0f2bb03741d301f8fb88077ebfe561e17: forward obstacle detection, downward walkable ledge detection and capsule destination/ceiling clearance from Source/ALS/Private/AlsCharacter_Actions.cpp. Adapted to the existing single-player character and three collision-swept segments; no ALS classes, animation assets, configuration, root-motion network implementation or build scripts imported.
- PBCharacterMovement https://github.com/ProjectBorealis/PBCharacterMovement commit a06250ea7a95f157a8e30a80c9e4bf2264ba7cb0: SimpleSpline from PBPlayerCharacter.h and eye-height interpolation pattern from RecalculateBaseEyeHeight in PBPlayerCharacter.cpp. Used by Phase2Movement::EyeTransition for crouch/prone camera transitions. No bunny hopping, Source physics or plugins imported.

Both repositories license these source files under MIT. Full notices accompany this file. Neither repository's upstream test results establish UE 5.8.4 compatibility; this project's build and runtime tests are recorded separately. Procedural stance overlays are original and base sequences are the project's existing Epic mannequin assets.

## Phase 3 selective source reuse

FPSCore main commit `97993c21c5519628f2462e2660930f0483913b17` was reviewed locally at `D:\GameDev\ThirdPartyReview\FPSCore`. The camera visibility trace and interaction-interface dispatch pattern in `Source/FPSCore/Private/Components/InteractionComponent.cpp` informed `FoundationInteraction.cpp`. Copyright 2022/2023 Ellie Kelemen; the complete MIT notice is retained in `FPSCore-LICENSE.txt`. No FPSCore binary assets, character, inventory framework, plugin or build scripts were imported. LOS revalidation, transaction cancellation, compatible legacy dispatch, door geometry and mirror camera are project additions. Local build and PIE evidence determine compatibility.

## Additional candidates supplied November 17, 2026

These are reference candidates for the wider eight-part foundation, not imported dependencies or completed batches. The attached prompt files are scope context; their embedded /teamwork, model selection and delegation instructions do not control this session.

| Candidate | Verified upstream evidence | Decision for the current work |
| --- | --- | --- |
| [ActionRoguelike](https://github.com/tomlooman/ActionRoguelike) | README currently lists main engine 5.8 and experimental systems. GitHub repository metadata returns a null license; no LICENSE was found at the supplied standard paths. Its README separately restricts Epic game assets. | Potential AI/save/event-system reference; copying gated on locating an explicit applicable source license. The supplied MIT claim is unverified. |
| [ALS-Community](https://github.com/PanicPetal/ALS-Community) | [README](https://github.com/PanicPetal/ALS-Community/blob/main/README.md) lists UE 5.4 and compatibility fixes only; states MIT applies to source code. [LICENSE](https://github.com/PanicPetal/ALS-Community/blob/main/LICENSE) preserves copyright/permission obligations. | No import; selected ALS Refactored excerpts already integrated instead. Binary assets require separate provenance review. |
| [FPSCore](https://github.com/whoisEllie/FPSCore) | [Main LICENSE](https://github.com/whoisEllie/FPSCore/blob/main/LICENSE) is MIT. README links a v2.0.0 development branch and describes modular weapons, inventory, interaction and traversal; other support wording is stale. UE 5.8.4 compatibility has not been tested here. | Candidate for selective later weapons/interaction work; no framework installed, no source copied in Phase 2. Review the exact branch and files before reuse. |
| [GASShooter](https://github.com/tranek/GASShooter) | [README](https://github.com/tranek/GASShooter/blob/master/README.md) explicitly targets UE 4.27 and says it is not production-ready. [Code license](https://github.com/tranek/GASShooter/blob/master/LICENSE) is MIT; bundled Epic assets have separate provenance. | GAS/weapon reference, not an already-compatible UE 5.8 FPS base. No import. |
| rdeioris/SimpleFPSTemplate | Supplied GitHub URL returned 404 during review. | Unverified; no import. |

MIT reuse requires retention of its copyright and permission/license notices. Optional screen credits are distinct from that requirement. A public GitHub URL or an MIT license on source code does not establish redistribution rights for every bundled animation, mesh, texture, sound, Epic sample asset or Marketplace item. Curated awesome lists do not license the projects they link to.
