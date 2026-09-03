import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ARENA_LABEL = "BossPlate_2_PythonArena"
CAMERA_FOLDER = "Story/BossIntro/Python"


def log(message):
    unreal.log(f"[BuildPythonIntroPresentation] {message}")


def make_rotator(pitch=0.0, yaw=0.0, roll=0.0):
    rotation = unreal.Rotator()
    rotation.pitch = float(pitch)
    rotation.yaw = float(yaw)
    rotation.roll = float(roll)
    return rotation


def look_rotation(camera_location, target_location):
    dx = target_location[0] - camera_location[0]
    dy = target_location[1] - camera_location[1]
    dz = target_location[2] - camera_location[2]
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, math.sqrt(dx * dx + dy * dy)))
    return make_rotator(pitch=pitch, yaw=yaw)


def actors():
    return list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())


def find(label):
    for actor in actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def spawn_camera(label, location, target, fov):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera = find(label)
    if camera and not isinstance(camera, unreal.CameraActor):
        actor_subsystem.destroy_actor(camera)
        camera = None

    rotation = look_rotation(location, target)
    if not camera:
        camera = actor_subsystem.spawn_actor_from_class(
            unreal.CameraActor.static_class(), unreal.Vector(*location), rotation
        )
        if not camera:
            raise RuntimeError(f"Could not spawn {label}")
        camera.set_actor_label(label)

    camera.modify()
    camera.set_actor_location(unreal.Vector(*location), False, False)
    camera.set_actor_rotation(rotation, False)
    camera.set_folder_path(unreal.Name(CAMERA_FOLDER))
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    if not camera_component:
        raise RuntimeError(f"{label} has no CameraComponent")
    camera_component.set_editor_property("field_of_view", float(fov))
    return camera


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")

    arena = find(ARENA_LABEL)
    if not arena:
        raise RuntimeError(f"Missing arena actor: {ARENA_LABEL}")

    # Wide reveal first, followed by short readable portraits of each Python identity.
    shot_specs = [
        ("Story_PythonIntroCamera_Wide", (2300.0, 5200.0, 900.0), (4300.0, 5200.0, 180.0), 72.0),
        ("Story_PythonIntroCamera_Vethara", (3470.0, 5000.0, 345.0), (4300.0, 4650.0, 235.0), 58.0),
        ("Story_PythonIntroCamera_Aurathos", (3470.0, 5400.0, 345.0), (4300.0, 5750.0, 235.0), 58.0),
    ]
    cameras = [spawn_camera(*spec) for spec in shot_specs]

    required_properties = {
        "intro_cameras": cameras,
        "intro_camera_times": [1.45, 1.05, 1.05],
        "intro_camera_blend_time": 0.35,
        "intro_return_blend_time": 0.55,
        "skip_full_intro_on_retry": True,
        "boss_intro_delay": 3.55,
        "hide_boss_status_until_intro_finished": True,
    }
    for name, value in required_properties.items():
        try:
            arena.set_editor_property(name, value)
        except Exception as exc:
            raise RuntimeError(
                f"Could not set '{name}' on {ARENA_LABEL}. Build and restart the editor first: {exc}"
            ) from exc

    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
        raise RuntimeError(f"Could not save {MAP_PATH}")

    log("PASS: 3-shot Python intro is assigned; full reveal is skipped after a death retry.")


if __name__ == "__main__":
    main()
