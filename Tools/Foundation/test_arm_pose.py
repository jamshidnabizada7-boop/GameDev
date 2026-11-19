"""Sample camera-relative arm presentation in a possessed PIE sandbox."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal as ue

world, = ue.EditorLevelLibrary.get_pie_worlds(False)
p = ue.GameplayStatics.get_player_pawn(world, 0)
pc = ue.GameplayStatics.get_player_controller(world, 0)
mount, = [c for c in p.get_components_by_class(ue.SceneComponent) if "FP_WeaponMount" in c.get_name()]
prefs = p.foundation_session.preferences
initial = prefs.camera_motion
performance = ue.get_default_object(ue.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
throttle = performance.get_editor_property("bThrottleCPUWhenNotForeground")
performance.set_editor_property("bThrottleCPUWhenNotForeground", False)
out = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_saved_dir())) / "Foundation/arm_pose_pie.json"
frames = {}
def key(name, down):
    ue.Phase2Tools.test_key(pc, name, down)
steps = [
    ("idle", 4.2, lambda: setattr(prefs, "camera_motion", True)),
    ("reduced", .8, lambda: setattr(prefs, "camera_motion", False)),
    ("crouch", .9, lambda: key("LeftControl", True)),
    ("stand", .8, lambda: key("LeftControl", False)),
    ("prepare", .2, lambda: setattr(prefs, "camera_motion", True)),
    ("jump", 1.25, lambda: key("SpaceBar", True)),
]
i = -1
end = 0
deadline = time.monotonic()+45
def tick(dt):
    global i, end
    try:
        assert time.monotonic() < deadline
        now = ue.GameplayStatics.get_time_seconds(world)
        if i >= 0:
            name = steps[i][0]
            frames[name].append((mount.relative_location.z, mount.relative_rotation.pitch, p.p2_pose_crouch))
        if now < end:
            return
        i += 1
        if i < len(steps):
            name, duration, action = steps[i]
            frames[name] = []
            action()
            end = now+duration
            return
        def span(key, column):
            values = [r[column] for r in frames[key][2:]]
            return max(values)-min(values)
        report = dict(idle_cm=span("idle",0), reduced_cm=span("reduced",0),
                      jump_pitch_degrees=span("jump",1),
                      crouch_drop_cm=frames["stand"][-1][0]-frames["crouch"][-1][0])
        report["passed"] = (report["idle_cm"] > .35 and report["reduced_cm"] < .08
                            and report["jump_pitch_degrees"] > 2 and report["crouch_drop_cm"] > .75)
    except Exception:
        report = {"error": traceback.format_exc()}
    key("SpaceBar", False)
    key("LeftControl", False)
    prefs.camera_motion = initial
    performance.set_editor_property("bThrottleCPUWhenNotForeground", throttle)
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    ue.unregister_slate_post_tick_callback(handle)
handle = ue.register_slate_post_tick_callback(tick)
