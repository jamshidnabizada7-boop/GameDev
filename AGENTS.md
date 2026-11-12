# Afghanistan Historical FPS — AI Agent Working Rules

## Mission
Build a single-player, history-informed Afghanistan Soviet-war FPS in Unreal Engine 5. Start with a tiny, playable first-person prototype; increase realism only after the prototype works. Do not assume a game engine version or installed plugins without checking them.

## Environment constraints
Windows 11; Ryzen 5 7500F; RTX 3060 12GB; 16GB system RAM; 256GB NVMe and 1TB HDD. Keep editor scalability modest and avoid large simultaneous local models. Confirm storage locations and remaining free space before any large install, download, or build.

## Working conventions
- Keep the Unreal `.uproject` as the source of truth. Use the same repository from Antigravity, Codex and OpenCode.
- Only one coding agent edits project files at a time; other agents review.
- Prefer Blueprint for the first playable prototype. Add C++ when there is a measured need or clear reusable system.
- Work in small increments: plan, change, build/load, test, report. Each increment must leave the project launchable.
- Never claim that a Blueprint, scene, material, import, animation or build works without testing it in Unreal or examining actual tool output.
- Never overwrite, delete, migrate an Unreal version, install a plugin, or run a third-party script without a specific review and user authorization.
- Use verified source references when modeling real historical locations; label artistic reconstruction and uncertainty clearly.
- Check licensing before incorporating third-party assets, source code, audio, fonts, or AI-generated materials.
- Use sensible names (`BP_`, `SM_`, `M_`, `MI_`, `T_`, `SK_`, `ABP_`) and separate maps, Blueprints, 3D assets and references.
- Back up or commit before bulk asset generation or plugin experiments. Never commit Unreal generated caches (Saved, Intermediate, DerivedDataCache, Binaries).

## Milestone 1 acceptance test
A minimal First Person template opens and runs: player can move, look around, jump, and interact with one target. Explain controls and show how to play-test. No character packs, cinematic level or historical campaign until milestone 1 succeeds.
