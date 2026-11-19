# Reference review and implementation — 2026-11-19

Source: C:/Users/PC/Downloads/WhatsApp Video 2026-11-19 at 2.28.11 PM.mp4
29.07 seconds; 576 x 1024. Contact sheets retained outside production at D:/GameDev/ThirdPartyReview/Reference_20261119.
This is a compressed edited showcase. Actions inferred from isolated frames require playback-level validation; no exact animation assets or source project were provided.

Observed or labelled:
- 0–4 s: rifle/SMG with holographic-style sight, firing, left-hand gesture, nearby knife-equipped mannequin and contextual commands.
- 4–7 s: close rear interaction with grenade visible, firing/ejection; transition needs closer frame review to identify the entire grab/plant sequence.
- 8–15 s: HUD label 1911, pistol shooting, impacts, falling bodies, slide/weapon manipulation during a reload.
- 16–19 s: HUD labels Cruiser / Karambit / Dual Wield; larger stocked firearm, low camera position. The exact firearm and prone-versus-slide transition are not conclusively identifiable from sampled frames.
- 20–24 s: close-range doorway interaction, firing, weapon changes, door/debris motion.
- 25–29 s: ending watermark obscures gameplay.

Requested beyond confirmed video evidence:
full-body shadow synchronization; four-direction prone; height-dependent hand-supported mantle and AI traversal; tactical sprint; grenade detection and navigable escape; material/posture-aware footsteps; unaware-behind knife takedown and aligned paired animation; iron/red-dot/holographic/magnified optics; model-specific reloads and per-weapon grips; robust interruptions and performance.
These are NOT marked complete by this review.

Current changes:
deferred weapon replacement with old-lower/new-raise sequence; smooth sprint pose alpha; slow crouch omits footstep noise emission. Native equipment pose is scheduled after character neutral pose, with first-person arm solving after equipment. Build logs and runtime results are in Saved/Foundation/visual_handling* and staged_switch_pie.json.

New asset evaluation:
Original author upload https://opengameart.org/content/m4a1-assault-rifle (nisu / 3DModelsCC0), declared CC0.
Downloaded m4a1_0.zip contains FBX and base color/normal/metallic/roughness/height textures.
Imported isolated candidate /Game/AfghanFPS/Weapons/Reviewed/M4A1, including a duplicate weapon Blueprint. The original rifle definition is unchanged on disk. A preview exposed remaining hand placement problems; importing a model alone is not visual acceptance.
M4A1 is a candidate replacement, not an identification of the video's main weapon.

Runtime checks completed:
- staged_switch_pie.json: observed old slot first, new slot at lowered position, raised at completion, and no shot accepted during the transition. Only six samples were captured under this editor's memory pressure; this is a functional check, not a visual smoothness or frame budget pass.
- grenade_evasion_pie.json: player input released a frag; fixture then fixed its position and extended fuse to isolate detection/path choice. NPC entered evasive behavior, found complete navigation paths, and moved 1329 cm. Occluded threats, smoke/flash negatives, moving grenade landing, blocked escape and return-to-combat remain unverified.
- silent_crouch_pie.json: two quiet crouch steps did not increase a nearby stationary guard's hearing count (4 to 4); ordinary walking did (4 to 5). No claim of material-specific footstep audio.

Original morning checkpoint remains commit 386857e. Pre-change source copies are in D:/GameDev/Backups/Visual_Handling_PreChanges_20261119. No new Git commit or push was made for this work.

R23 full Windows cook/package succeeded. Packaged 1080p title, play, pause, death and restart sequence passed and exited with code 0. Independent checkpoint write/read launches both passed and exited with code 0. Runtime evidence copied to VisualHandlingEvidence. Archive: D:/GameDev/AfghanistanFPS_Archives/Foundation_Win64_20261119_R23/Windows/AfghanistanFPS.exe.

R23 did not change the default rifle. It preserved the original test map and did not load the afternoon TacticalCQB assets quarantined during recovery.

R24 sets the textured M4A1 as the default rifle. The weapon Blueprint stores wrist anchors specific to its mesh. The animation proxy uses the rifle's wrist orientation and finger curl, leaves the right index finger less curled, and solves the wrists after equipment updates the weapon. The arms and the camera that carries the rifle attach to the pawn root at runtime, preventing an animated head socket from shifting the camera and its rifle after the grip solve. The arms use their own idle base pose. This pass targets first-person hand placement. It retains the mannequin mesh and procedural poses; it does not include authored finger animation, a moving magazine or reload hand contacts, all alternate weapons, or complete shadow and NPC animation work.

On a fresh editor launch with the saved rifle, the seven-angle alignment sweep passed from -89 to +89 degrees. The dynamic grip test passed across sampled walking, reload, and both settled weapon slots. These tests measure wrist-to-anchor distance; they do not measure finger-level mesh clipping. Hip and iron-sight views were captured separately. Editor tests used D3D11.

R24 Windows packaging succeeded. The 1080p D3D12 title-flow probe reached PASS and captured the default M4A1 in game. A second run using the game binary directly under D3D11 completed the title-flow probe and exited with code 0. User launch path: D:/GameDev/AfghanistanFPS_Archives/Foundation_Win64_20261119_R24/Windows/AfghanistanFPS.exe. Source snapshot: D:/GameDev/Backups/AfghanistanFPS_R24_20261119/Project.
