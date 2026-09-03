import math
import unreal


TRACK = "ExceptionCombat"
PLAYER_BP = "/Game/Blueprints/Core/BP_ExceptionCharacter"
DEST = "/Game/Player/Hendel/Animations"


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing animation asset: {path}")
    return asset


def duplicate(source, destination):
    if not unreal.EditorAssetLibrary.does_asset_exist(destination):
        if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
            raise RuntimeError(f"Could not duplicate {source} -> {destination}")
    return load(destination)


def reset_managed_track(animation):
    track_names = [str(name) for name in unreal.AnimationLibrary.get_animation_notify_track_names(animation)]
    if TRACK in track_names:
        unreal.AnimationLibrary.remove_animation_notify_events_by_track(animation, TRACK)
        unreal.AnimationLibrary.remove_animation_notify_track(animation, TRACK)
    unreal.AnimationLibrary.add_animation_notify_track(
        animation, TRACK, unreal.LinearColor(0.0, 0.72, 1.0, 1.0)
    )


def add_window(animation, start_time, duration, window):
    # Configure the notify object before adding it. AnimationLibrary caches the
    # notify display name when the FAnimNotifyEvent is created; changing the
    # enum afterwards leaves a misleading default name in the asset even though
    # the runtime UObject property is correct.
    notify = unreal.new_object(unreal.BRPlayerAnimNotifyState, outer=animation)
    if not notify:
        raise RuntimeError(
            f"Could not create gameplay window for {animation.get_path_name()}"
        )
    notify.set_editor_property("window", window)
    unreal.AnimationLibrary.add_animation_notify_state_event_object(
        animation,
        start_time,
        duration,
        notify,
        TRACK,
    )


def add_event(animation, time, event):
    notify = unreal.new_object(unreal.BRPlayerAnimNotify, outer=animation)
    if not notify:
        raise RuntimeError(
            f"Could not create animation event for {animation.get_path_name()}"
        )
    notify.set_editor_property("event", event)
    unreal.AnimationLibrary.add_animation_notify_event_object(
        animation,
        time,
        notify,
        TRACK,
    )


def save(animation, status):
    unreal.EditorAssetLibrary.set_metadata_tag(animation, "ExceptionAnimationStatus", status)
    if not unreal.EditorAssetLibrary.save_loaded_asset(animation, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {animation.get_path_name()}")


def build_attack(
    source,
    name,
    window_start,
    window_duration,
    heavy=False,
    combo_start=None,
    combo_duration=0.0,
    lock_start=0.03,
    lock_duration=0.45,
):
    animation = duplicate(source, f"{DEST}/{name}")
    reset_managed_track(animation)
    add_event(
        animation,
        max(0.0, window_start - 0.045),
        unreal.BRPlayerAnimEvent.HEAVY_WEAPON_SWING if heavy else unreal.BRPlayerAnimEvent.LIGHT_WEAPON_SWING,
    )
    add_window(
        animation,
        window_start,
        window_duration,
        unreal.BRPlayerAnimWindow.ATTACK_TRACE,
    )
    if combo_start is not None and combo_duration > 0.0:
        add_window(
            animation,
            combo_start,
            combo_duration,
            unreal.BRPlayerAnimWindow.COMBO_INPUT,
        )
    add_window(
        animation,
        lock_start,
        lock_duration,
        unreal.BRPlayerAnimWindow.ROOT_MOTION_LOCK,
    )
    save(animation, "FunctionalPlaceholder_NotifyAuthored")
    return animation


def copy_vector(value):
    return unreal.Vector(value.x, value.y, value.z)


def copy_quat(value):
    return unreal.Quat(value.x, value.y, value.z, value.w)


def create_or_load_sequence(name, skeleton, preview_mesh):
    path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)

    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skeleton)
    factory.set_editor_property("preview_skeletal_mesh", preview_mesh)
    sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, DEST, unreal.AnimSequence, factory
    )
    if not sequence:
        raise RuntimeError(f"Could not create roll sequence {path}")
    return sequence


def bake_roll(source, name, euler_axis, direction_sign):
    skeleton = source.get_editor_property("skeleton")
    preview_mesh = load("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
    sequence = create_or_load_sequence(name, skeleton, preview_mesh)
    track_names = [str(track_name) for track_name in unreal.AnimationLibrary.get_animation_track_names(source)]
    key_count = unreal.AnimationLibrary.get_num_keys(source)
    frame_count = max(1, unreal.AnimationLibrary.get_num_frames(source))
    if not track_names or key_count < 2:
        raise RuntimeError("MM_Dash has no source animation tracks to bake")

    sampled_frames = []
    for frame in range(key_count):
        sampled_frames.append(
            list(
                unreal.AnimationLibrary.get_bone_poses_for_frame(
                    source, track_names, min(frame, frame_count), False, preview_mesh
                )
            )
        )

    controller = sequence.get_editor_property("controller")
    if not controller:
        raise RuntimeError(f"No animation data controller for {sequence.get_path_name()}")

    source_model = source.get_editor_property("data_model_interface")
    sequence.modify()
    controller.open_bracket(f"Bake {name}", False)
    try:
        controller.remove_all_bone_tracks(False)
        if source_model:
            controller.set_frame_rate(source_model.get_frame_rate(), False)
        controller.set_number_of_frames(unreal.FrameNumber(frame_count), False)

        pelvis_index = track_names.index("pelvis")
        root_index = track_names.index("root")
        pelvis_base = sampled_frames[0][pelvis_index]
        root_base = sampled_frames[0][root_index]

        for bone_index, bone_name in enumerate(track_names):
            positions = []
            rotations = []
            scales = []
            for frame, poses in enumerate(sampled_frames):
                pose = poses[bone_index]
                position = copy_vector(pose.translation)
                rotation = copy_quat(pose.rotation)
                scale = copy_vector(pose.scale3d)
                alpha = frame / float(max(1, key_count - 1))

                if bone_name == "root":
                    # CharacterMovement owns the swept travel. Keep the baked
                    # skeleton centered so root motion never doubles it.
                    position = copy_vector(root_base.translation)
                    rotation = copy_quat(root_base.rotation)
                elif bone_name == "pelvis":
                    roll_alpha = max(0.0, min(1.0, (alpha - 0.04) / 0.88))
                    roll_alpha = roll_alpha * roll_alpha * (3.0 - 2.0 * roll_alpha)
                    curl = math.sin(math.pi * roll_alpha)
                    position = unreal.Vector(
                        position.x + (pelvis_base.translation.x - position.x) * curl * 0.72,
                        position.y + (pelvis_base.translation.y - position.y) * curl * 0.72,
                        position.z
                        + (pelvis_base.translation.z - position.z) * curl * 0.72
                        - 24.0 * curl,
                    )
                    roll_degrees = direction_sign * 360.0 * roll_alpha
                    euler = unreal.Vector(
                        euler_axis.x * roll_degrees,
                        euler_axis.y * roll_degrees,
                        euler_axis.z * roll_degrees,
                    )
                    roll_rotation = unreal.Quat()
                    roll_rotation.set_from_euler(euler)
                    rotation = roll_rotation.multiply(rotation)
                    rotation.normalize()

                positions.append(position)
                rotations.append(rotation)
                scales.append(scale)

            if not controller.add_bone_curve(bone_name, False):
                raise RuntimeError(f"Could not add {bone_name} to {name}")
            if not controller.set_bone_track_keys(
                bone_name, positions, rotations, scales, False
            ):
                raise RuntimeError(f"Could not set {bone_name} keys on {name}")
    finally:
        controller.close_bracket(False)

    unreal.AnimationLibrary.set_root_motion_enabled(sequence, False)
    save(sequence, "FunctionalPlaceholder_BakedDirectionalRoll")
    return sequence


def create_or_load_montage(sequence, name):
    path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        montage = load(path)
    else:
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", sequence)
        factory.set_editor_property("target_skeleton", sequence.get_editor_property("skeleton"))
        montage = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DEST, unreal.AnimMontage, factory
        )
        if not montage:
            raise RuntimeError(f"Could not create roll montage {path}")

    reset_managed_track(montage)
    # All roll montages are normalized to DodgeDuration (0.65 s) at runtime.
    play_rate = montage.get_play_length() / 0.65
    add_window(
        montage,
        0.0,
        0.32 * play_rate,
        unreal.BRPlayerAnimWindow.INVINCIBILITY,
    )
    save(montage, "FunctionalPlaceholder_BakedDirectionalRollMontage")
    return montage


unreal.EditorAssetLibrary.make_directory(DEST)

light_1 = build_attack(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01",
    "A_Hendel_Light_01_Temp",
    0.248,
    0.155,
    combo_start=0.405,
    combo_duration=0.180,
    lock_duration=0.450,
)
light_2 = build_attack(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02",
    "A_Hendel_Light_02_Temp",
    0.277,
    0.142,
    combo_start=0.420,
    combo_duration=0.210,
    lock_duration=0.500,
)
light_3 = build_attack(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03",
    "A_Hendel_Light_03_Temp",
    0.294,
    0.150,
    combo_start=0.480,
    combo_duration=0.190,
    lock_duration=0.590,
)
heavy_1 = build_attack(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03",
    "A_Hendel_Heavy_01_Temp",
    0.402,
    0.161,
    True,
    lock_duration=0.690,
)
heavy_2 = build_attack(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack",
    "A_Hendel_Heavy_02_Temp",
    0.403,
    0.150,
    True,
    lock_duration=0.740,
)

heal = duplicate(
    "/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload",
    f"{DEST}/A_Hendel_Heal_Temp",
)
reset_managed_track(heal)
# Played at 1.6x so 1.152 asset seconds lands at the 0.72 gameplay-second heal moment.
add_event(heal, 1.152, unreal.BRPlayerAnimEvent.HEAL)
save(heal, "FunctionalPlaceholder_CompatibleReload")

dash_source = load("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash")
roll_forward = bake_roll(
    dash_source, "A_Hendel_Roll_Forward_Temp", unreal.Vector(0.0, 1.0, 0.0), -1.0
)
roll_back = bake_roll(
    dash_source, "A_Hendel_Roll_Back_Temp", unreal.Vector(0.0, 1.0, 0.0), 1.0
)
roll_left = bake_roll(
    dash_source, "A_Hendel_Roll_Left_Temp", unreal.Vector(1.0, 0.0, 0.0), -1.0
)
roll_right = bake_roll(
    dash_source, "A_Hendel_Roll_Right_Temp", unreal.Vector(1.0, 0.0, 0.0), 1.0
)
dodge_forward = create_or_load_montage(roll_forward, "AM_Hendel_Roll_Forward_Temp")
dodge_back = create_or_load_montage(roll_back, "AM_Hendel_Roll_Back_Temp")
dodge_left = create_or_load_montage(roll_left, "AM_Hendel_Roll_Left_Temp")
dodge_right = create_or_load_montage(roll_right, "AM_Hendel_Roll_Right_Temp")

parry = duplicate(
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/AM_Player_Parry",
    f"{DEST}/AM_Hendel_Parry_Temp",
)
reset_managed_track(parry)
add_window(parry, 0.0, 0.15, unreal.BRPlayerAnimWindow.PARRY)
save(parry, "FunctionalPlaceholder_ParryMontage")

parry_success = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_04"
)
hit_front = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01"
)
hit_back = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01"
)
hit_left = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_03"
)
hit_right = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02"
)
heavy_knockback = load(
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01"
)
death = load("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01")

player_bp = load(PLAYER_BP)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
blueprint_status = player_bp.get_editor_property("Status")
if blueprint_status not in (
    unreal.BlueprintStatus.BS_UP_TO_DATE,
    unreal.BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS,
):
    raise RuntimeError(
        f"BP_ExceptionCharacter did not compile before assignment: {blueprint_status}"
    )
player_class = player_bp.generated_class()
if not player_class:
    raise RuntimeError("BP_ExceptionCharacter generated class is unavailable")
defaults = unreal.get_default_object(player_class)
defaults.modify()
player_bp.modify()
defaults.set_editor_property("light_combo_anims", [light_1, light_2, light_3])
defaults.set_editor_property("root_light_anim", light_2)
defaults.set_editor_property("root_heavy_anim", heavy_1)
defaults.set_editor_property("heavy_alt_anim", heavy_2)
defaults.set_editor_property("heal_anim", heal)
defaults.set_editor_property("dodge_montage", dodge_forward)
defaults.set_editor_property("dodge_forward_montage", dodge_forward)
defaults.set_editor_property("dodge_back_montage", dodge_back)
defaults.set_editor_property("dodge_left_montage", dodge_left)
defaults.set_editor_property("dodge_right_montage", dodge_right)
defaults.set_editor_property("parry_montage", parry)
defaults.set_editor_property("parry_success_anim", parry_success)
defaults.set_editor_property("hit_front_anim", hit_front)
defaults.set_editor_property("hit_back_anim", hit_back)
defaults.set_editor_property("hit_left_anim", hit_left)
defaults.set_editor_property("hit_right_anim", hit_right)
defaults.set_editor_property("heavy_knockback_anim", heavy_knockback)
defaults.set_editor_property("death_anim", death)
# Explicitly remove the old serialized Parry montage reuse.
defaults.set_editor_property("heal_montage", None)
unreal.EditorAssetLibrary.set_metadata_tag(
    player_bp, "ExceptionPlayerAnimationPass", "NotifyAuthored_TemporarySourceMotions"
)
if not unreal.EditorAssetLibrary.save_loaded_asset(player_bp, only_if_is_dirty=False):
    raise RuntimeError("Could not save BP_ExceptionCharacter")

unreal.log(
    "PLAYER_ANIMATION_PASS_COMPLETE "
    "Light=3 Heavy=2 Heal=1 DirectionalRoll=4 DodgeWindow=4 ParryWindow=1 "
    "ComboWindow=true RootMotionLock=true "
    "HealMontageParryReuse=false"
)
