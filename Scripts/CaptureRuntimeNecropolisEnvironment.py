import re
import time
from pathlib import Path

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
OUTPUT_SIZE = (1920, 1080)

# Stable baseline cameras for the five environment-only shots in the vertical
# slice review. Combat/Groggy/Victory captures must be taken from PIE after the
# corresponding boss state is active; this script intentionally does not fake it.
VIEWS = {
    "SpawnChamber": ((720.0, -180.0, 250.0), (1320.0, 40.0, 125.0), 55.0),
    "FirstCheckpoint": ((1550.0, -250.0, 330.0), (2180.0, 520.0, 150.0), 62.0),
    "Field1MainRoute": ((2050.0, 520.0, 310.0), (2550.0, 2700.0, 135.0), 72.0),
    "PythonEntrance": ((900.0, 4550.0, 430.0), (2100.0, 5200.0, 180.0), 63.0),
    "PythonArenaWide": ((2300.0, 5200.0, 900.0), (4300.0, 5200.0, 150.0), 73.0),
}


# Automation screenshots complete over several editor frames.  Retain both the
# task and its temporary camera until completion instead of letting Python GC
# cancel the request as soon as capture_view returns.
_ACTIVE_CAPTURES = {}


def capture_view(name):
    if name not in VIEWS:
        raise RuntimeError(f"Unknown Runtime Necropolis capture: {name}")

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    location_values, target_values, field_of_view = VIEWS[name]
    location = unreal.Vector(*location_values)
    target = unreal.Vector(*target_values)
    rotation = unreal.MathLibrary.find_look_at_rotation(location, target)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera = actor_subsystem.spawn_actor_from_class(unreal.CameraActor.static_class(), location, rotation)
    if not camera:
        raise RuntimeError("Could not create Runtime Necropolis capture camera")
    camera.set_actor_label(f"Preview_RuntimeNecropolis_{name}")
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    camera_component.set_editor_property("field_of_view", field_of_view)

    output = Path(unreal.Paths.project_saved_dir()) / "CodexCaptures" / f"After_{name}.png"
    output.parent.mkdir(parents=True, exist_ok=True)
    task = unreal.AutomationLibrary.take_high_res_screenshot(
        OUTPUT_SIZE[0], OUTPUT_SIZE[1], str(output), camera, False, False
    )
    if not task:
        actor_subsystem.destroy_actor(camera)
        raise RuntimeError(f"Could not start Runtime Necropolis capture: {name}")

    started_at = time.monotonic()

    def finish_capture(timed_out=False):
        state = _ACTIVE_CAPTURES.pop(name, None)
        if not state:
            return
        try:
            unreal.unregister_slate_post_tick_callback(state["tick_handle"])
        except Exception:
            pass
        if camera and unreal.SystemLibrary.is_valid(camera):
            actor_subsystem.destroy_actor(camera)
        if timed_out:
            unreal.log_error(
                f"[CaptureRuntimeNecropolisEnvironment] Timed out waiting for {output}"
            )
        else:
            unreal.log(
                f"[CaptureRuntimeNecropolisEnvironment] COMPLETE {output} "
                f"exists={output.exists()} bytes={output.stat().st_size if output.exists() else 0}"
            )

    def poll_capture(_delta_seconds):
        try:
            if task.is_task_done():
                finish_capture(False)
            elif time.monotonic() - started_at > 30.0:
                finish_capture(True)
        except Exception as exc:
            unreal.log_error(
                f"[CaptureRuntimeNecropolisEnvironment] Capture task failed for {name}: {exc}"
            )
            finish_capture(True)

    tick_handle = unreal.register_slate_post_tick_callback(poll_capture)
    _ACTIVE_CAPTURES[name] = {
        "task": task,
        "camera": camera,
        "tick_handle": tick_handle,
        "output": output,
    }
    unreal.log(f"[CaptureRuntimeNecropolisEnvironment] Requested {output}")
    return output


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"-NecropolisCapture=([A-Za-z0-9]+)", command_line)
    capture_view(match.group(1) if match else "SpawnChamber")


if __name__ == "__main__" and not globals().get("SKIP_AUTORUN", False):
    main()
