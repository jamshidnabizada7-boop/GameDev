# Foundation integration and Windows test guide

The release gate is open. This is the reusable sandbox, with no story maps or final art added. The [action matrix](PHASE8_ACTION_MATRIX.csv) reconciles 140 unique IDs against the authoritative register. It retains each existing status in a separate column and records only the integration evidence collected here. The authoritative CSV is preserved.

- Editor checks: 18 Blueprints compiled, and 22 PIE assertions passed for pause/resume, sensitivity, damage feedback, invalid saves, and restoring position, health, exact ammunition, weapon, inventory and objective state. See `Saved/Foundation/blueprint_compilation.json` and `Saved/Foundation/phase7_pie.json`. The death/restart regression passed with 100 HP, one HUD and 450 cm/s movement after allowing 0.4 s of simulated movement time (`Saved/Phase4/restart_results.json`).
- Windows build: `D:/GameDev/AfghanistanFPS_Archives/Foundation_Win64_20261118_R2/Windows/AfghanistanFPS.exe` (development). `Saved/Foundation/phase8_package.log` records successful build, cook and archive; `phase8_package_r2.log` records staging the same game binary and cook with the crash reporting client. The package uses the sandbox map and its dependencies, explicitly includes native runtime lookup directories, and excludes the F2Test and shooter probe directories. Attribution is beside the archive. See `Saved/Foundation/phase8_git_status.txt` for the working tree snapshot.
- Evidence scripts: `Tools/Foundation/test_phase7.py` (run using `run_remote.py` during an active PIE game), `package_sandbox.ps1`, `profile_sandbox.ps1`, and `audit_action_register.py`.
- Scope still open: repeat the original acceptance tests against this integrated build, check physical gamepad navigation and input across the entire sandbox, validate the visual and audible presentation at both resolutions, and complete the end-to-end restart and soak checks. Music volume has a reserved sound class, but no music track is assigned. World checkpoint state is implemented for stable interactables; NPC alert state is not serialized.

Both packaged D3D12 runs reached the sandbox on the RTX 3060. The captures are short and unattended; they establish a baseline and do not exercise representative gameplay. Values omit the first 120 of 720 recorded frames. The working set is a sampled process value, not VRAM or a system-wide memory figure.

| Output window | Median frame | 95th percentile | Peak sampled working set |
| --- | ---: | ---: | ---: |
| 1280×720 | 7.22 ms | 8.00 ms | 1,362 MiB |
| 1920×1080 | 8.40 ms | 9.25 ms | 1,536 MiB |

Raw logs and captures: `D:/GameDev/AfghanistanFPS_Archives/Foundation_Profile_20261118_Automated`. The profiling runner ends the process when the CSV is complete. Ordinary shutdown is still blocked: closing the window left the process alive; the timed normal-exit comparison returned Unreal's crash-reporter-thread error code 777003. Staging the crash reporting client did not fix it: the R2 shutdown timed out after 35 seconds, and an R2 run with audio disabled still returned 777003. Evidence: `Foundation_Profile_20261118_NormalExit`, `Foundation_Profile_20261118_R2_Exit` and `Foundation_Profile_20261118_R2_NoAudio` beside the archive. Run `test_sandbox_exit.ps1` to reproduce. The packaged startup logs also report a Recast tile serialization mismatch and mesh recreation; confirm NPC navigation in packaged play before marking it verified.

To check the packaged sandbox:

1. Open the Windows sandbox. Confirm you spawn with health and ammo visible. Walk and look around, fire once into the test target, and confirm that ammo and the existing HUD respond.
2. Stand on clear ground and press **F5**. Walk to a new spot and fire once. Press **F9**. Verify position, ammunition, and any saved door or switch state. Save again, close the game, relaunch, and load. Use an isolated test user directory if you have a save you want to keep.
3. Open pause with **Esc**, **F10**, or gamepad **Menu**. Set sensitivity, invert Y, and reduced motion. Rebind **E** to a free key, test interaction, then relaunch and confirm your settings persisted. Use **Restore defaults** to recover the original controls.
4. Test the captions and high-contrast setting, then change effects volume to zero and restore it. Check the HUD at **1280×720** and **1920×1080**, using both keyboard/mouse and gamepad. Log any clipping, inaccessible buttons, or overlapping prompts.
5. Run the sandbox for at least ten minutes with repeated pause, death/restart, and save/load cycles. Report any duplicate HUD, persistent audio loop, or crash. Use the matrix for per-action acceptance criteria rather than counting startup as gameplay coverage.
