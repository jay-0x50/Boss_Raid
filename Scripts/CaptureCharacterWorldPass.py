import re
from pathlib import Path

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"


def find_actor(label):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return next(
        (actor for actor in subsystem.get_all_level_actors() if actor.get_actor_label() == label),
        None,
    )


def prepare_cast_preview():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    nel = find_actor("Story_NelCompanion_Awakening")
    if not nel:
        raise RuntimeError("Could not find awakening Nel actor")
    nel.set_actor_hidden_in_game(False)
    ghost01 = unreal.load_asset("/Game/Characters/Exception/Materials/M_Nel_Ghost01")
    ghost02 = unreal.load_asset("/Game/Characters/Exception/Materials/M_Nel_Ghost02")
    robe = unreal.load_asset("/Game/Characters/Exception/Materials/M_Nel_Robe")
    nel_mesh = nel.get_component_by_class(unreal.SkeletalMeshComponent)
    nel_mesh.set_material(0, ghost01)
    nel_mesh.set_material(1, ghost02)
    robe_meshes = list(nel.get_components_by_class(unreal.StaticMeshComponent))
    if robe_meshes:
        robe_meshes[0].set_material(0, robe)

    handel = subsystem.spawn_actor_from_class(
        unreal.SkeletalMeshActor.static_class(),
        unreal.Vector(1260.0, 125.0, 96.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-35.0),
    )
    handel.set_actor_label("Preview_Hendel_Concept")
    handel_mesh = handel.get_component_by_class(unreal.SkeletalMeshComponent)
    handel_mesh.set_skeletal_mesh(unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"))
    handel_mesh.set_material(0, unreal.load_asset("/Game/Characters/Exception/Materials/M_Hendel_Armor01"))
    handel_mesh.set_material(1, unreal.load_asset("/Game/Characters/Exception/Materials/M_Hendel_Armor02"))
    handel_mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    handel_mesh.set_animation(unreal.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"))
    handel_mesh.set_position(0.42, False)

    return (
        unreal.Vector(760.0, -590.0, 285.0),
        unreal.Vector(1450.0, 270.0, 170.0),
        60.0,
    )


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"-CharacterPassCapture=([A-Za-z]+)", command_line)
    view = match.group(1) if match else "EncounterA"

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    if view == "EncounterA":
        location = unreal.Vector(2400.0, -520.0, 360.0)
        target = unreal.Vector(2400.0, 560.0, 175.0)
        field_of_view = 72.0
    elif view == "Cast":
        location, target, field_of_view = prepare_cast_preview()
    else:
        raise RuntimeError(f"Unknown character pass capture: {view}")

    rotation = unreal.MathLibrary.find_look_at_rotation(location, target)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera = subsystem.spawn_actor_from_class(unreal.CameraActor.static_class(), location, rotation)
    camera.set_actor_label(f"Preview_CharacterWorld_{view}")
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    camera_component.set_editor_property("field_of_view", field_of_view)

    output = Path(unreal.Paths.project_saved_dir()) / "CharacterWorldCaptures" / f"{view}.png"
    unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(output), camera, False, False)
    unreal.log(f"[CaptureCharacterWorldPass] Requested {output}")


main()
