import re
from pathlib import Path

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
VIEWS = {
    "Field1": ((2050.0, 250.0, 430.0), (2820.0, 2100.0, 105.0), 68.0),
    "Field2": ((7200.0, 2550.0, 470.0), (6500.0, 900.0, 180.0), 67.0),
    "Field3": ((7000.0, -4870.0, 450.0), (9300.0, -2850.0, 185.0), 67.0),
    "PythonGate": ((5200.0, 5200.0, 1500.0), (6460.0, 5200.0, 245.0), 61.0),
    "VritraGate": ((5200.0, -5200.0, 1500.0), (6460.0, -5200.0, 245.0), 61.0),
}


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"-ExplorationCapture=([A-Za-z0-9]+)", command_line)
    name = match.group(1) if match else "Field1"
    if name not in VIEWS:
        raise RuntimeError(f"Unknown exploration capture: {name}")

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    location_values, target_values, field_of_view = VIEWS[name]
    location = unreal.Vector(*location_values)
    target = unreal.Vector(*target_values)
    rotation = unreal.MathLibrary.find_look_at_rotation(location, target)

    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera = subsystem.spawn_actor_from_class(unreal.CameraActor.static_class(), location, rotation)
    if not camera:
        raise RuntimeError("Could not create exploration capture camera")
    camera.set_actor_label(f"Preview_Exploration_{name}")
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    camera_component.set_editor_property("field_of_view", field_of_view)

    output = Path(unreal.Paths.project_saved_dir()) / "ExplorationCaptures" / f"{name}.png"
    unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(output), camera, False, False)
    unreal.log(f"[CaptureExplorationWorld] Requested {output}")


main()
