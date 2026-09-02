import re
from pathlib import Path

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
VIEWS = {
    "Vritra": ((0.0, -6600.0, 1400.0), (4300.0, -5200.0, 260.0)),
    "Python": ((0.0, 6600.0, 1400.0), (4300.0, 5200.0, 260.0)),
    "Selvara": ((5200.0, 6600.0, 1400.0), (9500.0, 5200.0, 260.0)),
    "CMD": ((10000.0, -1200.0, 1500.0), (14800.0, 0.0, 300.0)),
}


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"-BossCapture=([A-Za-z]+)", command_line)
    name = match.group(1) if match else "CMD"
    if name not in VIEWS:
        raise RuntimeError(f"Unknown boss capture: {name}")

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    location_values, target_values = VIEWS[name]
    location = unreal.Vector(*location_values)
    target = unreal.Vector(*target_values)
    rotation = unreal.MathLibrary.find_look_at_rotation(location, target)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera = actor_subsystem.spawn_actor_from_class(unreal.CameraActor.static_class(), location, rotation)
    if not camera:
        raise RuntimeError("Could not create capture camera.")
    camera.set_actor_label(f"Preview_{name}_BossEnvironment")
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    camera_component.set_editor_property("field_of_view", 74.0)

    output = Path(unreal.Paths.project_saved_dir()) / "BossEnvironmentCaptures" / f"{name}.png"
    unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(output), camera, False, False)
    unreal.log(f"[CaptureBossEnvironmentView] Requested {output}")


main()
