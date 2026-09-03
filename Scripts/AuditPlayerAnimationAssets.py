import unreal


DEST = "/Game/Player/Hendel/Animations"
TRACK = "ExceptionCombat"
GENERATED_ASSETS = [
    f"{DEST}/A_Hendel_Light_01_Temp",
    f"{DEST}/A_Hendel_Light_02_Temp",
    f"{DEST}/A_Hendel_Light_03_Temp",
    f"{DEST}/A_Hendel_Heavy_01_Temp",
    f"{DEST}/A_Hendel_Heavy_02_Temp",
    f"{DEST}/A_Hendel_Heal_Temp",
    f"{DEST}/A_Hendel_Roll_Forward_Temp",
    f"{DEST}/A_Hendel_Roll_Back_Temp",
    f"{DEST}/A_Hendel_Roll_Left_Temp",
    f"{DEST}/A_Hendel_Roll_Right_Temp",
    f"{DEST}/AM_Hendel_Roll_Forward_Temp",
    f"{DEST}/AM_Hendel_Roll_Back_Temp",
    f"{DEST}/AM_Hendel_Roll_Left_Temp",
    f"{DEST}/AM_Hendel_Roll_Right_Temp",
    f"{DEST}/AM_Hendel_Parry_Temp",
]

BASELINE_ASSETS = [
    "/Game/Blueprints/Core/BP_ExceptionCharacter",
    "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
    "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed",
    "/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/AM_Player_LightAttack",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/AM_Player_HeavyAttack",
    "/Game/Characters/Mannequins/Anims/Unarmed/Attack/AM_Player_Parry",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/AM_Player_Dodge",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash",
    "/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_03",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_04",
    "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01",
    "/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01",
]

NOTIFY_TIME_TOLERANCE = 0.003
DODGE_RUNTIME_SECONDS = 0.65
DODGE_INVINCIBILITY_SECONDS = 0.32
EXPECTED_NOTIFY_ENTRY_COUNT = 24


def event_spec(event, start):
    return ("event", event, start, 0.0)


def window_spec(window, start, duration):
    return ("window", window, start, duration)


# Keep these values synchronized with BuildPlayerAnimationPass.py. Entries are
# deliberately chronological so the audit also detects timeline reordering.
STATIC_EXPECTED_NOTIFY_SPECS = {
    f"{DEST}/A_Hendel_Light_01_Temp": (
        window_spec("RootMotionLock", 0.030, 0.450),
        event_spec("LightWeaponSwing", 0.203),
        window_spec("AttackTrace", 0.248, 0.155),
        window_spec("ComboInput", 0.405, 0.180),
    ),
    f"{DEST}/A_Hendel_Light_02_Temp": (
        window_spec("RootMotionLock", 0.030, 0.500),
        event_spec("LightWeaponSwing", 0.232),
        window_spec("AttackTrace", 0.277, 0.142),
        window_spec("ComboInput", 0.420, 0.210),
    ),
    f"{DEST}/A_Hendel_Light_03_Temp": (
        window_spec("RootMotionLock", 0.030, 0.590),
        event_spec("LightWeaponSwing", 0.249),
        window_spec("AttackTrace", 0.294, 0.150),
        window_spec("ComboInput", 0.480, 0.190),
    ),
    f"{DEST}/A_Hendel_Heavy_01_Temp": (
        window_spec("RootMotionLock", 0.030, 0.690),
        event_spec("HeavyWeaponSwing", 0.357),
        window_spec("AttackTrace", 0.402, 0.161),
    ),
    f"{DEST}/A_Hendel_Heavy_02_Temp": (
        window_spec("RootMotionLock", 0.030, 0.740),
        event_spec("HeavyWeaponSwing", 0.358),
        window_spec("AttackTrace", 0.403, 0.150),
    ),
    f"{DEST}/A_Hendel_Heal_Temp": (event_spec("Heal", 1.152),),
    f"{DEST}/AM_Hendel_Parry_Temp": (
        window_spec("Parry", 0.000, 0.150),
    ),
}

ROLL_MONTAGE_PATHS = {
    f"{DEST}/AM_Hendel_Roll_Forward_Temp",
    f"{DEST}/AM_Hendel_Roll_Back_Temp",
    f"{DEST}/AM_Hendel_Roll_Left_Temp",
    f"{DEST}/AM_Hendel_Roll_Right_Temp",
}

EXPECTED_DEFAULT_PATHS = {
    "root_light_anim": f"{DEST}/A_Hendel_Light_02_Temp",
    "root_heavy_anim": f"{DEST}/A_Hendel_Heavy_01_Temp",
    "heavy_alt_anim": f"{DEST}/A_Hendel_Heavy_02_Temp",
    "heal_anim": f"{DEST}/A_Hendel_Heal_Temp",
    "parry_montage": f"{DEST}/AM_Hendel_Parry_Temp",
    "dodge_montage": f"{DEST}/AM_Hendel_Roll_Forward_Temp",
    "dodge_forward_montage": f"{DEST}/AM_Hendel_Roll_Forward_Temp",
    "dodge_back_montage": f"{DEST}/AM_Hendel_Roll_Back_Temp",
    "dodge_left_montage": f"{DEST}/AM_Hendel_Roll_Left_Temp",
    "dodge_right_montage": f"{DEST}/AM_Hendel_Roll_Right_Temp",
    "parry_success_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_04",
    "hit_front_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01",
    "hit_back_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01",
    "hit_left_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_03",
    "hit_right_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02",
    "heavy_knockback_anim": "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01",
    "death_anim": "/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01",
}

failures = []


def fail(message):
    failures.append(message)
    unreal.log_error(f"PLAYER_ANIM_AUDIT_FAIL {message}")


def safe_property(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as error:
        fail(f"{obj.get_path_name()} property {name} unavailable: {error}")
        return None


def asset_path(value):
    if not value:
        return ""
    try:
        return value.get_path_name().split(".", 1)[0]
    except Exception:
        return str(value).split(".", 1)[0]


def load_required(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        fail(f"missing asset {path}")
    return asset


def enum_token(value):
    if value is None:
        return ""
    text = str(value)
    if "." in text:
        text = text.rsplit(".", 1)[-1]
    if ":" in text:
        text = text.split(":", 1)[0]
    return "".join(character for character in text if character.isalnum()).lower()


def expected_notify_specs(path, asset):
    static_specs = STATIC_EXPECTED_NOTIFY_SPECS.get(path)
    if static_specs is not None:
        return static_specs
    if path in ROLL_MONTAGE_PATHS:
        asset_duration = float(asset.get_play_length())
        authored_duration = (
            DODGE_INVINCIBILITY_SECONDS
            * asset_duration
            / DODGE_RUNTIME_SECONDS
        )
        return (window_spec("Invincibility", 0.0, authored_duration),)
    return ()


def notify_entries(asset):
    try:
        names = [
            str(name)
            for name in unreal.AnimationLibrary.get_animation_notify_event_names(asset)
        ]
        events = unreal.AnimationLibrary.get_animation_notify_events_for_track(
            asset, TRACK
        )
        entries = []
        for entry in events:
            notify = entry.notify
            notify_state = entry.notify_state_class
            if notify and notify.get_class().get_name() == "BRPlayerAnimNotify":
                kind = "event"
                semantic = str(notify.get_editor_property("event"))
            elif (
                notify_state
                and notify_state.get_class().get_name() == "BRPlayerAnimNotifyState"
            ):
                kind = "window"
                semantic = str(notify_state.get_editor_property("window"))
            else:
                kind = "unknown"
                semantic = ""
            start = float(
                unreal.AnimationLibrary.get_anim_notify_event_trigger_time(entry)
            )
            duration = float(
                unreal.AnimationLibrary.get_anim_notify_event_duration(entry)
            )
            entries.append((kind, semantic, start, duration))
        return names, entries
    except Exception as error:
        fail(f"{asset_path(asset)} notify query failed: {error}")
        return [], []


def validate_notify_schedule(path, asset, details):
    expected = expected_notify_specs(path, asset)
    if not expected:
        return

    names, actual = notify_entries(asset)
    tracks = [
        str(name)
        for name in unreal.AnimationLibrary.get_animation_notify_track_names(asset)
    ]
    details.append(f"notify_names={names}")
    details.append(f"notify_entries={actual}")
    details.append(f"notify_tracks={tracks}")
    if TRACK not in tracks:
        fail(f"{path} missing {TRACK} notify track")

    if len(actual) != len(expected):
        fail(
            f"{path} expected {len(expected)} notify entries, got {len(actual)}"
        )

    for index in range(1, len(actual)):
        if actual[index][2] + NOTIFY_TIME_TOLERANCE < actual[index - 1][2]:
            fail(
                f"{path} notify entries are not chronological at indices "
                f"{index - 1}/{index}"
            )

    for index, (actual_entry, expected_entry) in enumerate(zip(actual, expected)):
        actual_kind, actual_semantic, actual_start, actual_duration = actual_entry
        expected_kind, expected_semantic, expected_start, expected_duration = (
            expected_entry
        )
        if actual_kind != expected_kind:
            fail(
                f"{path} notify[{index}] expected {expected_kind}, got {actual_kind}"
            )
        if enum_token(actual_semantic) != enum_token(expected_semantic):
            fail(
                f"{path} notify[{index}] expected {expected_semantic}, "
                f"got {actual_semantic}"
            )
        if abs(actual_start - expected_start) > NOTIFY_TIME_TOLERANCE:
            fail(
                f"{path} notify[{index}] {expected_semantic} start expected "
                f"{expected_start:.3f}, got {actual_start:.3f}"
            )
        if abs(actual_duration - expected_duration) > NOTIFY_TIME_TOLERANCE:
            fail(
                f"{path} notify[{index}] {expected_semantic} duration expected "
                f"{expected_duration:.3f}, got {actual_duration:.3f}"
            )


def validate_audit_definition():
    notify_asset_paths = set(STATIC_EXPECTED_NOTIFY_SPECS) | ROLL_MONTAGE_PATHS
    generated_paths = set(GENERATED_ASSETS)
    unknown_paths = notify_asset_paths - generated_paths
    if unknown_paths:
        fail(f"notify schedule references unknown assets {sorted(unknown_paths)}")

    expected_total = len(ROLL_MONTAGE_PATHS) + sum(
        len(specs) for specs in STATIC_EXPECTED_NOTIFY_SPECS.values()
    )
    if expected_total != EXPECTED_NOTIFY_ENTRY_COUNT:
        fail(
            f"audit definition expected {EXPECTED_NOTIFY_ENTRY_COUNT} notify "
            f"entries, got {expected_total}"
        )

    for path, specs in STATIC_EXPECTED_NOTIFY_SPECS.items():
        starts = [spec[2] for spec in specs]
        if starts != sorted(starts):
            fail(f"audit definition for {path} is not chronological")
        for kind, semantic, start, duration in specs:
            if kind not in ("event", "window") or not semantic or start < 0.0:
                fail(
                    f"audit definition has invalid entry {path}: "
                    f"{(kind, semantic, start, duration)}"
                )
            if kind == "event" and duration != 0.0:
                fail(f"audit event has nonzero duration {path}: {semantic}")
            if kind == "window" and duration <= 0.0:
                fail(f"audit window has invalid duration {path}: {semantic}")


def validate_generated_asset(path):
    asset = load_required(path)
    if not asset:
        return

    class_name = asset.get_class().get_name()
    status_value = unreal.EditorAssetLibrary.get_metadata_tag(
        asset, "ExceptionAnimationStatus"
    )
    status = str(status_value) if status_value else ""
    if not status_value:
        fail(f"{path} missing ExceptionAnimationStatus metadata")

    details = [f"class={class_name}", f"status={status}"]
    try:
        details.append(f"length={asset.get_play_length():.3f}")
    except Exception:
        pass

    validate_notify_schedule(path, asset, details)

    if "/A_Hendel_Roll_" in path and class_name == "AnimSequence":
        track_count = len(unreal.AnimationLibrary.get_animation_track_names(asset))
        details.append(f"track_count={track_count}")
        if track_count < 50:
            fail(f"{path} contains only {track_count} baked bone tracks")
        if bool(safe_property(asset, "enable_root_motion")):
            fail(f"{path} unexpectedly enables root motion")

    unreal.log(f"PLAYER_ANIM_AUDIT {path} " + " ".join(details))


def validate_baseline_assets():
    for path in BASELINE_ASSETS:
        asset = load_required(path)
        if not asset:
            continue
        details = [f"class={asset.get_class().get_name()}"]
        try:
            details.append(f"length={asset.get_play_length():.3f}")
        except Exception:
            pass
        if path.endswith("BS_Idle_Walk_Run"):
            samples = safe_property(asset, "sample_data")
            sample_count = len(samples) if samples is not None else 0
            details.append(f"sample_count={sample_count}")
            if sample_count < 17:
                fail(f"{path} has only {sample_count} locomotion samples")
        unreal.log(f"PLAYER_ANIM_BASELINE {path} " + " ".join(details))


def validate_player_defaults():
    bp = load_required("/Game/Blueprints/Core/BP_ExceptionCharacter")
    if not bp:
        return
    generated_class = bp.generated_class()
    if not generated_class:
        fail("BP_ExceptionCharacter generated class unavailable")
        return
    defaults = unreal.get_default_object(generated_class)

    for property_name, expected_path in EXPECTED_DEFAULT_PATHS.items():
        actual = safe_property(defaults, property_name)
        actual_path = asset_path(actual)
        unreal.log(f"PLAYER_ANIM_DEFAULT {property_name}={actual_path}")
        if actual_path != expected_path:
            fail(
                f"BP default {property_name} expected {expected_path}, got {actual_path}"
            )

    heal_montage = safe_property(defaults, "heal_montage")
    parry_montage = safe_property(defaults, "parry_montage")
    heal_anim = safe_property(defaults, "heal_anim")
    unreal.log(f"PLAYER_ANIM_DEFAULT heal_montage={asset_path(heal_montage)}")
    if heal_montage:
        fail(f"heal_montage must be None, got {asset_path(heal_montage)}")
    if not heal_anim:
        fail("heal_anim is unassigned")
    if asset_path(heal_anim) == asset_path(parry_montage):
        fail("heal animation still reuses the parry montage")

    combo_assets = safe_property(defaults, "light_combo_anims")
    combo_paths = [asset_path(asset) for asset in combo_assets] if combo_assets else []
    expected_combo_paths = [
        f"{DEST}/A_Hendel_Light_01_Temp",
        f"{DEST}/A_Hendel_Light_02_Temp",
        f"{DEST}/A_Hendel_Light_03_Temp",
    ]
    unreal.log(f"PLAYER_ANIM_DEFAULT light_combo_anims={combo_paths}")
    if combo_paths != expected_combo_paths:
        fail(f"light_combo_anims expected {expected_combo_paths}, got {combo_paths}")

    for property_name in (
        "execution_montage",
        "hit_montage",
    ):
        unreal.log(
            f"PLAYER_ANIM_DEFAULT {property_name}="
            f"{asset_path(safe_property(defaults, property_name))}"
        )


if len(GENERATED_ASSETS) != 15:
    fail(f"audit definition expected 15 generated assets, got {len(GENERATED_ASSETS)}")

validate_audit_definition()
validate_baseline_assets()
for generated_path in GENERATED_ASSETS:
    validate_generated_asset(generated_path)
validate_player_defaults()

if failures:
    raise RuntimeError(
        f"PLAYER_ANIM_AUDIT_FAILED count={len(failures)}: " + " | ".join(failures)
    )

unreal.log(
    "PLAYER_ANIM_AUDIT_COMPLETE "
    f"GeneratedAssets={len(GENERATED_ASSETS)} "
    f"NotifyEntries={EXPECTED_NOTIFY_ENTRY_COUNT} "
    f"NotifyTimingToleranceMs={NOTIFY_TIME_TOLERANCE * 1000.0:.0f} "
    "DirectionalDodgeAssignments=4 HealSeparatedFromParry=true "
    "AttackTrace=true ComboInput=true RootMotionLock=true "
    "DodgeInvincibility=true ParryWindow=true HealEvent=true NotifyOrder=true"
)
