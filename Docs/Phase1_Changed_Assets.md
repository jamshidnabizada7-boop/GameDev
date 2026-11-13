# PHASE 1 CHANGED ASSETS — Afghanistan FPS
Date: 2026-11-13/14 · Agent: ZCode continuation · Branch: main

## Modified (previously untracked, now completed)
| Asset | Change | Status |
|---|---|---|
| `Content/Input/IMC_Default.uasset` | Added 4 mappings: LeftShift→IA_Sprint, LeftCtrl→IA_Crouch, E→IA_Interact, R→IA_Restart (16 total) | Saved, verified |
| `Content/AfghanFPS/Characters/Player/Blueprints/BP_PlayerFoundation.uasset` | EventGraph rebuilt from empty: template input (IA_Move/Look/MouseLook/Jump) + sprint + crouch + restart + AnyDamage→BPC_Health + BeginPlay InitializeHealth/OnDeath bind + HandleDeath. BPC_Health component attached (verified pre-existing) | Saved, compiled BS_UP_TO_DATE, 0 errors |
| `Content/AfghanFPS/Characters/Player/Blueprints/BP_GM_FoundationTest.uasset` | No changes this session (verified correct: pawn=BP_PlayerFoundation_C, PC=template controller) | Unchanged |

## New
| Asset | Purpose | Status |
|---|---|---|
| `Content/AfghanFPS/Interaction/Interfaces/BPI_Interactable.uasset` | Interface with `Interact(Interactor: Actor)` | Created, compiled, saved |
| `Content/AfghanFPS/Interaction/BP_TestInteractable.uasset` | StaticMeshActor BP, Cube mesh, bBig var; **interface assignment + Interact override pending (manual GUI step)** | Created, compiled, saved |
| `Content/AfghanFPS/Interaction/BP_TestDamageVolume.uasset` | TriggerBox BP: ActorBeginOverlap → ApplyDamage(OtherActor, 25) | Created, compiled, saved, PIE-verified |
| `Content/AfghanFPS/TestMaps/L_Test_PlayerFoundation.umap` | Test room: floor 20×20 m, 4 walls, crouch tunnel (100 cm clearance), interact cube at 2.2 m, damage volume at 6 m, PlayerStart, lighting. WorldSettings GameMode override = BP_GM_FoundationTest | Created, saved, PIE-tested |
| `Docs/*.md` | Phase 1 report set + handover | New |

## Unchanged (verified)
- `Content/AfghanFPS/_Scratch/*` (8 probe BPs) — untouched, pending cleanup review
- `Content/DemoTemplate/_Core/Lvl_IntroRoom` — untouched
- ABP_Unarmed, ABP_FP_Copy, template FirstPerson content — untouched
- `Config/*` — untouched (Python was force-enabled at runtime by UE 5.8 for
  `-ExecutePythonScript`; no plugin edits needed)

## Pre-existing input actions (verified correct, unchanged)
IA_Sprint/IA_Crouch/IA_Interact/IA_Restart — Boolean, correct keys (created by
previous agent, phase1_build_01_input evidence).
