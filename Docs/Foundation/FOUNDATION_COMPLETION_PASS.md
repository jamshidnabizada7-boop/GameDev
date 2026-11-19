# Foundation completion pass

This pass implemented three camera responses, an original ambient loop, and a reusable training terminal. The current status is in the first columns of [the action matrix](PHASE8_ACTION_MATRIX.csv); the source register's historical columns are preserved for traceability.

| Actions | Implementation and observed result | Evidence |
| --- | --- | --- |
| K-3, K-4, K-5 | Landing dip, sprint FOV and cosmetic shot kick. Possessed PIE measured about 3.55 cm dip, 5.72° additional FOV and 0.38° camera kick in the sample run. The same actions produced no measurable cosmetic feedback with Camera Motion off. | `Saved/Foundation/camera_motion_pie.json` |
| L-8 | One sandbox-owned wind source routed through Effects. Unreal reported active playback after the six-second loop elapsed. Physical listening and volume acceptance remain open. | `Saved/Foundation/ambient_pie.json` |
| M-8, O-4 | The player focused and used the training terminal, received a caption, could not complete it a second time, and restored its pending state from an earlier checkpoint. | `Saved/Foundation/terminal_pie.json` |
| O-2 | Existing remap implementation rejected conflicts and a reserved key; changed the live movement input and recovered the default in PIE. Physical menu navigation and relaunch persistence still need acceptance. | `Saved/Foundation/rebind_pie.json` |

PIE verified the integrated session's 22 save/settings assertions again. Tests temporarily suppress the editor's 3 FPS background throttle for transient camera measurements, then restore the previous value. The new wind waveform is generated locally by the project script and requires no new third-party notices. The terminal uses the existing interaction class and save state; no story content was introduced.

The R3 build reached the sandbox and completed one ordinary window close with exit code 0. A timed normal exit returned Unreal's crash-reporting-thread code 777003, including under NullRHI. An immediate quit with no profiling capture returned 0 in both the engine Entry map and the sandbox. These comparisons narrow the failure but do not identify a fix or establish consistent normal shutdown. The logs are in the sibling folders `Foundation_Profile_20261118_R3_Exit`, `Foundation_Profile_20261118_R3_NoRendererExit`, `Foundation_Profile_20261118_R3_ImmediateSandbox`, and `Foundation_Profile_20261118_R3_WindowClose` under `D:/GameDev/AfghanistanFPS_Archives`. Test with `test_sandbox_exit.ps1` both with and without `-CloseWindow`.

The latest test build is `D:/GameDev/AfghanistanFPS_Archives/Foundation_Win64_20261118_R4/Windows/AfghanistanFPS.exe`. It contains the terminal in its final sandbox position, clear of the movement lane. Unreal built, cooked, and archived it successfully (`Saved/Foundation/completion_package_r4.log`). It launched in D3D12; a regular close request did not terminate within 35 seconds (`Foundation_Profile_20261118_R4_WindowClose/result.json`). The package is for testing and the shutdown item stays open.

Next acceptance work: establish repeatable exit without profiling or forced termination; verify packaged AI navigation, physical input and HUD/audio/accessibility; use the matrix to schedule the remaining animation, effect and session-state work; run a full ten-minute mixed gameplay soak on the completed build. Prior PIE measurements are listed as prior coverage where available and do not establish packaged behavior.
