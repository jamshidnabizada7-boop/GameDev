"""Create a Phase 8 overlay for the source register without rewriting legacy results."""
import argparse
import csv
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("register", type=Path)
parser.add_argument("--packaged-exe", type=Path)
parser.add_argument("--package-log", type=Path, default=root / "Saved/Foundation/phase8_package.log")
parser.add_argument("--profile-captures", type=Path)
parser.add_argument("--exit-result", type=Path)
parser.add_argument("--navigation-result", type=Path)
parser.add_argument("--relaunch-profile", type=Path)
parser.add_argument("--title-results", type=Path, nargs="+")
parser.add_argument("--soak-result", type=Path)
parser.add_argument("--frame-budget", type=Path)
parser.add_argument("--output", type=Path, default=root / "Docs/Foundation/PHASE8_ACTION_MATRIX.csv")
args = parser.parse_args()
budget = json.loads(args.frame_budget.read_text(encoding="utf-8-sig")) if args.frame_budget else {}
def passed_probe(path, field, marker):
    result = json.loads(path.read_text(encoding="utf-8-sig"))
    return result.get("Exited") is True and result.get("ExitCode") == 0 and marker in (result.get(field) or "")

soak_ok = args.soak_result and passed_probe(args.soak_result, "MixedSoakLog", "FoundationMixedSoak PASS")
title_ok = args.title_results and all(passed_probe(path, "TitleFlowLog", "FoundationTitleFlow PASS") for path in args.title_results)
restart_ok = title_ok and all(passed_probe(path, "TitleFlowLog", "PASS step=11") for path in args.title_results)
response_ok = title_ok and all("FoundationSightAndResponse PASS" in Path(json.loads(
    path.read_text(encoding="utf-8-sig"))["Log"]).read_text(encoding="utf-8", errors="replace") for path in args.title_results)
relaunch_ok = args.relaunch_profile and all(
    f"FoundationRelaunch {mode.lower()} PASS" in (args.relaunch_profile / f"{mode}.log").read_text(encoding="utf-8", errors="replace")
    for mode in ("Write", "Read")
)
with args.register.open(newline="", encoding="utf-8-sig") as handle:
    rows = list(csv.DictReader(handle))
expected = {f"{letter}-{i}" for letter, count in zip("ABCDEFGHIJKLMNOP",
    (12, 10, 10, 9, 7, 8, 12, 7, 8, 12, 8, 11, 8, 5, 7, 6))
    for i in range(1, count + 1)}
assert len(rows) == len(expected) == len({r["Action_ID"] for r in rows}) == 140
assert {r["Action_ID"] for r in rows} == expected

def load_report(relative):
    path = root / relative
    return json.loads(path.read_text(encoding="utf-8-sig")) if path.is_file() else {}

checks = load_report("Saved/Foundation/phase7_pie.json")
alignment_path = "Saved/Foundation/aim_alignment_pie.json"
alignment = load_report(alignment_path)
feet_path = "Saved/Foundation/foot_placement_pie.json"
feet = load_report(feet_path)
arms_path = "Saved/Foundation/arm_pose_pie.json"
arms = load_report(arms_path)
axes_path = "Saved/Foundation/controller_axes_pie.json"
axes = load_report(axes_path)
camera_path = "Saved/Foundation/camera_motion_pie.json"
camera = load_report(camera_path).get("checks", {})
ambient_path = "Saved/Foundation/ambient_pie.json"
ambient = load_report(ambient_path)
terminal_path = "Saved/Foundation/terminal_pie.json"
terminal = load_report(terminal_path)
rebind_path = "Saved/Foundation/rebind_pie.json"
rebind = load_report(rebind_path)
death_path = "Saved/Phase4/restart_results.json"
death = load_report(death_path)
death_reduced_path = "Saved/Phase4/restart_reduced_results.json"
death_reduced = load_report(death_reduced_path)
direction_path = "Saved/Foundation/damage_feedback_pie.json"
direction = load_report(direction_path)
numbers_path = "Saved/Foundation/damage_numbers_pie.json"
numbers = load_report(numbers_path)
contrast_path = "Saved/Foundation/high_contrast_labels_pie.json"
contrast = load_report(contrast_path)
interaction_path = "Docs/Foundation/Phase3Evidence/checks.json"
interaction = {item["check"]: item["passed"] for item in load_report(interaction_path).get("checks", [])}
locomotion_path = "Docs/Foundation/Phase2Evidence/derived_checks.json"
equipment_path = "Docs/Foundation/Phase4Evidence/checks.json"
equipment = {item["check"]: item["passed"] for item in load_report(equipment_path).get("checks", [])}
recorded_checks = {}
def register_samples(path, report, mapping):
    for action, names in mapping.items():
        if all(report.get(name) is True for name in names):
            recorded_checks[action] = f"{path}: " + ", ".join(names)

register_samples(locomotion_path, load_report(locomotion_path), {
    "A-1": ["walk_450"], "A-2": ["sprint_700", "sprint_release_450"],
    "A-3": ["crouch_200"], "A-5": ["prone_crosses_tunnel", "prone_sprint_prevented"],
    "A-6": ["left_and_right", "lean_centers", "wall_lean_limited"],
    "A-8": ["crouch_ceiling_blocks", "prone_stand_blocked"],
    "A-11": ["slow_walk_200", "slow_sprint_conflict_limited"],
    "A-12": ["vault_crosses", "climb_reaches_top", "blocked_landing_rejected"],
})
register_samples(interaction_path, interaction, {
    "J-1": ["legacy cube still toggles through E", "range and LOS cancel hold without reward"],
    "J-2": ["key unlocks and opens to 90", "swept door stops against obstruction"],
    "J-5": ["actual light toggles off", "linked external lamp toggles off"],
    "J-6": ["early release cancels loot", "loot grants designed reserve once"],
    "J-7": ["existing weapon damages and breaks native prop"],
    "J-8": ["two wheel notches hold ten degrees"],
    "J-9": ["peek enters and restores FOV", "fifteen-degree peek shifts camera without moving capsule"],
    "J-10": ["mirror survives opening E release", "mirror exits to pawn and walking"],
    "J-12": ["locked door denies entry", "unaligned pick fails, aligned pick unlocks"],
})
register_samples(equipment_path, equipment, {
    "C-3": ["reload conserves magazine and reserve", "full reload rejected", "limited reserve transfers minimum"],
    "C-4": ["dry click consumes no ammunition and fires no shot"], "C-5": ["holster blocks firing"],
    "C-6": ["switch preserves base ammunition", "switch preserves alternate ammunition"],
    "C-7": ["pickup creates second unique slot", "dropping both firearms leaves an empty loadout"],
    "C-8": ["initial existing firearm and ammunition", "switch preserves base ammunition"],
    "C-9": ["drop removes one inventory copy", "reclaim preserves dropped magazine and reserve"],
    "C-10": ["inspection blocks firing"], "D-1": ["semi hold fires once", "second trigger fires once"],
    "D-2": ["semi hold fires once", "automatic hold fires repeatedly within rate", "burst completes exactly three shots after short press"],
    "D-3": ["rapid triggers respect cooldown"], "D-5": ["projectile collision applies actual damage"],
    "D-6": ["distance falloff reduces damage", "limb multiplier applies thirteen damage"],
    "D-7": ["tagged thin cover passes reduced damage", "thick cover rejects penetration", "untagged cover rejects penetration"],
    "D-9": ["reload blocks firing", "holster blocks firing", "inspection blocks firing"],
    "E-1": ["ADS narrows FOV and spread"], "E-4": ["actual look input scales to sixty percent in ADS"],
    "E-5": ["ADS hides existing crosshair", "ADS release restores FOV and crosshair"],
    "F-7": ["head multiplier applies forty damage", "limb multiplier applies thirteen damage"],
    "I-2": ["stone preparation spends a finite item and draws trajectory", "released trajectory disappears"],
    "I-3": ["frag harms target after real fuse"], "I-4": ["wall blocks radial damage", "removing same wall permits radial damage"],
    "I-5": ["cooking frag in hand kills owner and spends last item"],
    "I-6": ["smoke activates after fuse", "held flash actually affects view and fades"],
    "I-7": ["knife embeds and leaves finite inventory", "knife retrieval removes world copy and restores one item"],
    "I-8": ["stone actually simulates physics and emits bounded impact events", "stone can be retrieved once"],
    "J-3": ["normal cover blocks hitscan"],
})
ai_path = "Saved/Phase5/ai_results.json"
ai = load_report(ai_path)
surrender_path = "Saved/Phase5/surrender.json"
surrender = load_report(surrender_path)
tactical_path = "Saved/Phase6/tactical_results.json"
tactical = load_report(tactical_path)
def snapshot(label, key):
    return tactical.get(label, {}).get(key)

tactical_checks = {
    "G-12": ("FOLLOW" in str(snapshot("follow", "order")) and "CEASE_FIRE" in str(snapshot("cease", "order")),
             "Follow and cease-fire order changes"),
    "H-5": (snapshot("hostage", "hostage") is True and snapshot("release", "hostage") is False,
            "Context starts and releases"),
    "H-6": (bool(snapshot("drag", "tactical")) and snapshot("drag_released", "tactical") == "" and
            snapshot("drag", "pelvis") != snapshot("drag_moved", "pelvis"), "Drag, pelvis displacement and release"),
    "H-7": ("DOWNED" in str(snapshot("friendly_down", "friendly")) and snapshot("revived", "friendly_hp") == 50,
            "Downed ally recovered 50 HP"),
    "L-11": (snapshot("whistle", "noise") == "Whistle" and (snapshot("whistle", "heard") or 0) > 0,
             "Simulated hearing event received"),
}
def metric(sample, key, default=None):
    return ai.get(sample, {}).get(key, default)

ai_checks = {
    "G-3": (metric("outside_cone", "sight") is False and metric("combat", "sight") is True and
            metric("occluded", "sight") is False, "Sight acquisition, outside-cone and occlusion samples"),
    "G-4": (metric("patrol_after", "arrivals", 0) > metric("patrol_before", "arrivals", 0),
            "Patrol waypoints advanced"),
    "G-5": (metric("combat", "shots", 0) > 0 and metric("combat", "player_health", 100) < 100,
            "Combat shot and simulated player damage"),
    "G-6": (metric("after_stagger", "cover", 0) > 0, "Cover state selection sampled"),
    "G-7": ("STAGGER" in metric("stagger", "state", "") and "STAGGER" not in metric("after_stagger", "state", ""),
            "Stagger and recovery samples"),
    "G-8": (metric("dead", "ragdoll") is True and metric("respawn", "ragdoll") is False and
            metric("respawn", "health", 0) > 0, "Ragdoll and drill reset samples"),
    "G-9": (len(ai.get("nav_path", [])) > 1 and metric("patrol_after", "arrivals", 0) > 1,
            "PIE navigation path and patrol progress"),
}
phase7 = {
    "K-6": (["damage_feedback"], "Damage event reached HUD state; review flash and direction on screen."),
    "M-8": (["objective_changed", "objective_restored"], "Objective flag round trip; visible objective update requires playtest."),
    "N-2": (["checkpoint_saved", "checkpoint_loaded", "invalid_checkpoint_rejected_without_reset",
             "position_restored", "health_restored", "exact_ammo_and_weapon_restored", "inventory_restored"],
            "Checkpoint restores position, health, inventory and ammo; verify props, menu/hotkeys, and a full relaunch."),
    "N-4": (["pause", "resume"], "Pause and resume verified through component; verify controls and menu focus."),
    "N-5": (["preferences_saved", "preferences_disk_round_trip"], "Sensitivity saved to disk; relaunch and verify every option."),
    "O-3": (["preferences_disk_round_trip"], "Saved sensitivity verified; verify the actual look response and inversion."),
}
package_log = args.package_log
package_ok = (args.packaged_exe and args.packaged_exe.is_file() and package_log.is_file()
              and "BUILD SUCCESSFUL" in package_log.read_text(encoding="utf-8", errors="replace"))
profile_runs = json.loads(args.profile_captures.read_text(encoding="utf-8-sig")) if args.profile_captures else []
profile_ok = {(run["Width"], run["Height"]) for run in profile_runs} == {(1280, 720), (1920, 1080)} and all(
    Path(run["FrameCapture"]).is_file() and Path(run["Log"]).is_file() for run in profile_runs
)
exit_result = json.loads(args.exit_result.read_text(encoding="utf-8-sig")) if args.exit_result else {}
nav_result = json.loads(args.navigation_result.read_text(encoding="utf-8-sig")) if args.navigation_result else exit_result
for row in rows:
    key, old = row["Action_ID"], row["Evidence_Status"]
    status = "Needs integration test"
    evidence = ""
    remaining = row["Acceptance_Test"]
    if old.startswith("WORKS IN PIE") or old == "TESTED":
        status = "Prior evidence; integration retest pending"
        evidence = "Original register evidence and acceptance notes"
    elif old.startswith("PARTIAL") or old == "PROTOTYPED":
        status = "Prior partial coverage"
        match = re.search(r"Docs/[^ ;]+", old)
        evidence = match.group(0) if match else "Original register evidence and acceptance notes"
    if key in recorded_checks:
        status = "Prior PIE measurements available; packaged action retest pending"
        evidence = recorded_checks[key]
    if key in phase7:
        names, detail = phase7[key]
        if all(checks.get(n) is True for n in names):
            status = "Partial PIE coverage"
            evidence = "Saved/Foundation/phase7_pie.json: " + ", ".join(names)
            remaining = detail + " " + remaining
    camera_test = {"K-3": "landing_dip", "K-4": "sprint_fov", "K-5": "shot_kick"}.get(key)
    if camera_test and camera.get(camera_test) and camera.get("reduced_motion"):
        status = "Implemented; measured in PIE with reduced-motion comparison"
        evidence = f"{camera_path}: {camera_test}, reduced_motion"
    if key == "L-8" and all(ambient.get(k) for k in ("playing_at_start", "looping", "playing_after_one_loop")):
        status = "Implemented; loop played in PIE; listening review pending"
        evidence = f"{ambient_path}: single sandbox source, looping through SC_Effects"
    if key == "M-8" and terminal and all(terminal.values()) and terminal.get("terminal_activated") is True:
        status = "Training objective implemented; interaction and checkpoint verified in PIE"
        evidence = terminal_path
    if key == "O-4" and terminal.get("caption_emitted"):
        status = "Implemented; objective caption emitted in PIE; display and cue coverage pending"
        evidence = terminal_path
    if key == "O-2" and all(rebind.get(k) for k in (
        "duplicate_key_rejected", "reserved_key_rejected", "alternate_key_saved", "old_key_disabled",
        "new_key_moves", "default_key_moves"
    )):
        status = "Implemented; live rebind and default recovery verified in PIE; menu and relaunch pending"
        evidence = rebind_path
    if key == "B-7" and alignment.get("muzzle_aligned") and alignment.get("both_grips_aligned"):
        status = "Stationary equipped grip and muzzle alignment verified in PIE; moving poses and swaps pending"
        evidence = alignment_path
    if key in ai_checks and ai_checks[key][0]:
        status = "Implemented; prior PIE behavior sampled; integration retest pending"
        evidence = f"{ai_path}: {ai_checks[key][1]}"
    if key == "G-11" and "SURRENDER" in surrender.get("state", "") and surrender.get("armed") is False and surrender.get("drops") == 1:
        status = "Surrender pose, disarmed state and one dropped item sampled in PIE"
        evidence = surrender_path
    if key in tactical_checks and tactical_checks[key][0]:
        status = "Partial contextual gameplay verified in PIE; full acceptance pending"
        evidence = f"{tactical_path}: {tactical_checks[key][1]}"
    if key in ("G-4", "G-9") and nav_result.get("NavigationVerified") is True:
        status = "Packaged patrol and full path verified; other routes and chase retest pending"
        evidence = str(args.navigation_result or args.exit_result)
    if key == "K-7" and death.get("passed") is True and death_reduced.get("passed") is True:
        status = "Implemented; camera and restart verified in PIE; visual fade review pending"
        evidence = f"{death_path}; {death_reduced_path}"
    if key == "F-5" and all(direction.get(k) is True for k in ("ahead", "left", "right", "behind", "no_false_direction_for_self_damage")):
        status = "Implemented; four directional damage cues verified in PIE; layout review pending"
        evidence = direction_path
    if key == "F-6" and all(numbers.get(k) is True for k in ("amount_matches_health_loss", "world_number_spawned", "world_number_expired", "disabled_hides_number")):
        status = "Implemented for direct hitscan; exact value and toggle verified in PIE"
        evidence = numbers_path
        remaining = "Verify visual readability; extending the cue to secondary damage sources remains open. " + remaining
    if key == "O-5" and contrast.get("passed") is True and all(direction.get(k) is True for k in ("ahead", "left", "right", "behind")):
        status = "Implemented; text-based cues and NPC labels verified in PIE; visual simulation pending"
        evidence = f"{direction_path}; {contrast_path}"
    if key in ("F-8", "J-4") and all(interaction.get(k) for k in (
        "ammo adds reserve only", "full health rejects without consuming", "health heals and updates existing HUD"
    )):
        status = "Implemented; prior PIE verified; integration retest pending"
        evidence = interaction_path
    if key == "J-11" and interaction.get("listen enters and releases"):
        status = "Implemented; prior PIE verified; listening review pending"
        evidence = interaction_path
    if key in ("P-1", "P-4") and checks and all(checks.values()):
        status = "Session smoke ran in PIE; full action coverage pending"
        evidence = "Tools/Foundation/test_phase7.py and Saved/Foundation/phase7_pie.json"
    if key == "P-6" and package_ok:
        launched = profile_ok or exit_result.get("RequestedExit") is True
        status = "Windows package built and started; gameplay acceptance pending" if launched else "Windows package built; gameplay acceptance pending"
        evidence = f"{args.packaged_exe}; {package_log}"
        if exit_result:
            evidence += f"; {args.exit_result}"
            if exit_result.get("ExitCode") != 0:
                status += "; shutdown failed"
            elif (exit_result.get("ExitSeconds") or 0) > 10:
                status += "; shutdown slow"
    if key in ("P-3", "P-5") and profile_ok:
        status = "Short packaged baseline; gameplay soak and budget validation pending"
        evidence = str(args.profile_captures)
    if key == "A-10" and feet.get("step_passed") and feet.get("slope_passed"):
        status = "30-degree side slope and 40 cm step verified in PIE"
        evidence = feet_path
        remaining = "Inspect foot roll and contact during a moving turn."
    if key in ("B-2", "B-4", "B-5") and arms.get("passed") is True:
        status = "First-person mount motion measured in PIE"
        evidence = arms_path
        remaining = "Inspect arm silhouettes and grip alignment across supported weapons."
    if key == "H-3" and terminal.get("hand_reaches_on_instant_use") is True:
        status = "Immediate interaction and reach blend verified in PIE"
        evidence = terminal_path
        remaining = "Check hand contact on the other interactable shapes."
    if key in ("G-3", "G-5") and response_ok:
        status = "Packaged NPC sight acquisition and response verified"
        evidence = ", ".join(map(str, args.title_results))
        remaining = "Verify other NPC routes, occluders and alert propagation."
    if key == "N-2" and relaunch_ok:
        status = "Checkpoint inventory restored by a fresh packaged process"
        evidence = str(args.relaunch_profile)
        remaining = "Exercise save and load through both menu and hotkeys after changing placed and dropped items."
    if key == "L-3" and numbers.get("character_mark_attached") and numbers.get("character_mark_red") and numbers.get("character_mark_expired"):
        status = "Damage-colored mark attached to target and expired in PIE"
        evidence = numbers_path
        remaining = "Check visibility at range and distinguish it from the environment impact cue."
    if key in ("P-3", "P-5") and budget.get("passed") is True:
        status = "Packaged steady-state frame budget passed at both resolutions"
        evidence = str(args.frame_budget)
        remaining = "Exercise peak NPC and effects load and measure system memory and VRAM headroom."
    if key == "N-3" and title_ok:
        status = "Packaged title, play, pause and resume verified" if not restart_ok else "Packaged title, play, pause, death and restart verified"
        evidence = ", ".join(map(str, args.title_results))
        remaining = "Repeat the flow using a physical controller." if restart_ok else "Exercise death and restart in the same packaged session."
    if key in ("M-8", "P-4") and soak_ok:
        status = "Ten-minute packaged terminal, movement and checkpoint loop passed"
        evidence = str(args.soak_result)
        remaining = "Exercise remaining action families and failure cases in integrated gameplay."
    if key == "O-7" and axes.get("passed") is True:
        status = "Simulated controller move and look verified in PIE; physical device test pending"
        evidence = axes_path
        if soak_ok and passed_probe(args.soak_result, "MixedSoakLog", "PASS phase=7"):
            status = "Simulated controller move, look, interact and fire verified; physical device test pending"
            evidence += f"; {args.soak_result}"
        remaining = "Repeat movement, aim, interaction, fire and menu navigation with a connected physical controller."
    if key == "B-7" and alignment.get("both_grips_aligned") and load_report("Saved/Foundation/dynamic_grips_pie.json").get("passed"):
        status = "PIE grips verified during walking, reload and after two slot switches; stationary pitch sweep passed"
        evidence = alignment_path + "; Saved/Foundation/dynamic_grips_pie.json"
        remaining = "Check transitions, other poses and visual clipping."
    if key == "P-5" and load_report("Saved/Foundation/npc_capacity_pie.json").get("passed"):
        status += "; ten-NPC cap and slot reuse verified in PIE"
        evidence += "; Saved/Foundation/npc_capacity_pie.json"
    if key == "G-10" and load_report("Saved/Foundation/group_hearing_pie.json").get("passed"):
        status = "One player sound reached both nearby NPCs and excluded a distant NPC in PIE"
        evidence = "Saved/Foundation/group_hearing_pie.json"
        remaining = "Check subsequent investigation and combat state transitions."
    if key == "N-2" and load_report("Saved/Foundation/corrupt_checkpoint_pie.json").get("passed"):
        status += "; truncated and checksum-damaged checkpoint rejected, prior format loaded in PIE"
        evidence += "; Saved/Foundation/corrupt_checkpoint_pie.json"
    if key in ("P-3","P-5") and load_report("Saved/Foundation/r22_population_budget.json").get("passed"):
        status += "; ten possessed NPCs profiled in packaged R22"
        evidence += "; Saved/Foundation/r22_population_budget.json"
        remaining = "Exercise simultaneous maximum effects and physics load during gameplay."
    row["Integration_Status"] = status
    row["Integration_Evidence"] = evidence
    row["Next_Validation"] = remaining

with args.output.open("w", newline="", encoding="utf-8-sig") as handle:
    leading = ["Action_ID", "Action_Name", "Integration_Status", "Integration_Evidence", "Next_Validation"]
    writer = csv.DictWriter(handle, fieldnames=leading + [key for key in rows[0] if key not in leading])
    writer.writeheader()
    writer.writerows(rows)
print(f"Wrote {len(rows)} unique action rows to {args.output}")
