import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
PREFIX = "Story_"
FOLDER = "Story/Prologue"

ROCK_MESH = "/Game/LevelPrototyping/Meshes/SM_ChamferCube"
RAMP_MESH = "/Game/LevelPrototyping/Meshes/SM_Ramp"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"
CYLINDER_MESH = "/Engine/BasicShapes/Cylinder.Cylinder"
BOULDER_MESH = "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD2"
WALL_DETAIL_MESH = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallDetail"
WALL_HALF_MESH = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallHalf"
FLOOR_DETAIL_MESH = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_FloorDetail"
FIELD_MAT = "/Game/World/Environment/Materials/M_Floor_Field_Default"
WALL_MAT = "/Game/World/Environment/Materials/M_Wall_Corridor_Default"
CODE_MAT = "/Game/World/Environment/Materials/M_Floor_Boss_Python"
CMD_MAT = "/Game/World/Environment/Materials/M_Wall_Boss_CMD"
STONE_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_KD_Stone"
BOULDER_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Boulder01"
FORT_WALL_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Wall"
FORT_TRIM_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Trim"


def log(message):
    unreal.log(f"[BuildStoryPrologue] {message}")


def make_rotator(pitch=0.0, yaw=0.0, roll=0.0):
    """Build a Rotator by field so call sites consistently use pitch/yaw/roll."""
    rotation = unreal.Rotator()
    rotation.pitch = float(pitch)
    rotation.yaw = float(yaw)
    rotation.roll = float(roll)
    return rotation


def make_color(red, green, blue, alpha=255):
    return unreal.Color(b=int(blue), g=int(green), r=int(red), a=int(alpha))


def load_map():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")


def all_actors():
    return list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())


def find_actor(label):
    for actor in all_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def set_prop(obj, name, value, required=False):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        if required:
            raise RuntimeError(f"Could not set {name} on {obj}: {exc}") from exc
        log(f"Skipped {name} on {obj}: {exc}")
        return False


def get_component(actor, component_class):
    try:
        return actor.get_component_by_class(component_class)
    except Exception:
        pass
    try:
        parts = actor.get_components_by_class(component_class)
        return parts[0] if parts else None
    except Exception:
        return None


def spawn_or_update(label, actor_class, location, rotation=None, folder=FOLDER):
    actor = find_actor(label)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    target_rotation = rotation if rotation is not None else make_rotator()
    if actor and actor.get_class() != actor_class:
        actor_subsystem.destroy_actor(actor)
        actor = None
    if not actor:
        actor = actor_subsystem.spawn_actor_from_class(
            actor_class,
            location,
            target_rotation,
        )
        if not actor:
            raise RuntimeError(f"Failed to spawn {label}")
        actor.set_actor_label(label)
    actor.modify()
    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(target_rotation, False)
    actual_rotation = actor.get_actor_rotation()
    def angle_delta(first, second):
        return abs((first - second + 180.0) % 360.0 - 180.0)
    if (
        angle_delta(actual_rotation.pitch, target_rotation.pitch) > 0.05
        or angle_delta(actual_rotation.yaw, target_rotation.yaw) > 0.05
        or angle_delta(actual_rotation.roll, target_rotation.roll) > 0.05
    ):
        raise RuntimeError(
            f"Rotation mismatch on {label}: requested={target_rotation}, actual={actual_rotation}"
        )
    actor.set_folder_path(unreal.Name(folder))
    return actor


def make_mesh(label, location, scale, rotation=(0.0, 0.0, 0.0), mesh_path=ROCK_MESH,
              material_path=WALL_MAT, collision=True, folder="Story/Prologue/Cave",
              visible=True, cast_shadow=True):
    actor = spawn_or_update(
        label,
        unreal.StaticMeshActor.static_class(),
        unreal.Vector(*location),
        make_rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]),
        folder,
    )
    mesh = unreal.load_asset(mesh_path)
    material = unreal.load_asset(material_path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"Missing mesh: {mesh_path}")
    component = get_component(actor, unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError(f"Missing mesh component on {label}")
    component.set_static_mesh(mesh)
    if isinstance(material, unreal.MaterialInterface):
        component.set_material(0, material)
    component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION
    )
    component.set_collision_profile_name(unreal.Name("BlockAll" if collision else "NoCollision"))
    component.set_editor_property("cast_shadow", cast_shadow)
    component.set_visibility(visible, True)
    component.set_hidden_in_game(not visible, True)
    actor.set_actor_enable_collision(collision)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def build_spawn_cave():
    # Keep a cheap, invisible shell for predictable third-person collision. The
    # visible chamber is built from grounded stone and burned-server fragments,
    # so the player no longer wakes inside a row of prototype blue boxes.
    collision_shells = [
        ("BackWall", (360.0, 0.0, 365.0), (2.2, 14.0, 7.0), (0.0, 3.0, 0.0)),
        ("WallLeft", (1190.0, -720.0, 365.0), (16.5, 2.5, 6.8), (0.0, 1.0, -2.0)),
        ("WallRight", (1170.0, 735.0, 350.0), (16.0, 2.7, 6.5), (0.0, -2.0, 2.5)),
        ("Roof", (1190.0, 0.0, 735.0), (16.5, 14.5, 1.7), (0.0, 0.0, -1.5)),
        ("ExitPillarLeft", (2050.0, -570.0, 330.0), (3.2, 3.0, 6.0), (0.0, 18.0, -3.0)),
        ("ExitPillarRight", (2050.0, 575.0, 340.0), (3.0, 3.4, 6.2), (0.0, -14.0, 3.0)),
        ("ExitLintel", (2050.0, 0.0, 680.0), (3.0, 12.0, 1.8), (0.0, 2.0, 0.0)),
    ]
    for name, location, scale, rotation in collision_shells:
        make_mesh(
            f"{PREFIX}Cave_{name}", location, scale, rotation,
            collision=True, visible=False, cast_shadow=False,
            folder="Story/Prologue/CollisionShell",
        )

    for stale_name in ("RockA", "RockB", "RockC", "RockD"):
        stale = find_actor(f"{PREFIX}Cave_{stale_name}")
        if stale:
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(stale)

    art_specs = [
        ("BackMass_A", (250.0, -250.0, 165.0), (3.2, 3.2, 3.3), (8.0, 18.0, -7.0), BOULDER_MESH, BOULDER_MAT),
        ("BackMass_B", (460.0, 350.0, 205.0), (4.2, 4.4, 4.2), (-6.0, 64.0, 8.0), BOULDER_MESH, BOULDER_MAT),
        ("LeftMass_A", (900.0, -650.0, 160.0), (3.0, 3.4, 3.2), (5.0, 31.0, -8.0), BOULDER_MESH, BOULDER_MAT),
        ("LeftMass_B", (1570.0, -665.0, 185.0), (3.5, 3.0, 3.6), (-4.0, 72.0, 6.0), BOULDER_MESH, BOULDER_MAT),
        ("RightMass_A", (840.0, 675.0, 190.0), (3.3, 3.1, 3.7), (-7.0, 112.0, 6.0), BOULDER_MESH, BOULDER_MAT),
        ("RightMass_B", (1520.0, 680.0, 170.0), (3.2, 3.5, 3.3), (6.0, 145.0, -7.0), BOULDER_MESH, BOULDER_MAT),
        # Tie the upper mass into the right wall instead of suspending one
        # isolated boulder above the awakening platform.
        ("RoofMass", (1120.0, 580.0, 300.0), (4.2, 2.4, 3.2), (-2.0, 25.0, 8.0), BOULDER_MESH, BOULDER_MAT),
        ("ServerMonolith_L", (820.0, -480.0, 170.0), (0.58, 0.50, 1.45), (-4.0, 12.0, -4.0), WALL_DETAIL_MESH, FORT_WALL_MAT),
        ("ServerMonolith_R", (910.0, 475.0, 165.0), (0.52, 0.56, 1.32), (3.0, -9.0, 5.0), WALL_DETAIL_MESH, FORT_WALL_MAT),
        ("ExitFrame_L", (1980.0, -505.0, 165.0), (0.72, 0.62, 1.35), (0.0, 16.0, -3.0), WALL_HALF_MESH, STONE_MAT),
        ("ExitFrame_R", (1980.0, 510.0, 170.0), (0.68, 0.66, 1.42), (0.0, -14.0, 3.0), WALL_HALF_MESH, STONE_MAT),
    ]
    for name, location, scale, rotation, mesh_path, material_path in art_specs:
        make_mesh(
            f"{PREFIX}NecroCave_{name}", location, scale, rotation,
            mesh_path=mesh_path, material_path=material_path, collision=False,
            folder="Story/Prologue/RuntimeNecropolis",
        )

    # A dark stone bed masks the original flat slab inside the awakening room.
    make_mesh(
        f"{PREFIX}Cave_Floor",
        (1180.0, 0.0, 25.0),
        (17.0, 13.5, 0.18),
        material_path=FIELD_MAT,
        folder="Story/Prologue/Cave",
    )

    # A broken octagonal execution ring frames Hendel while leaving an open gap
    # toward the exit. It is presentation-only so it cannot snag dodge movement.
    make_mesh(
        f"{PREFIX}AwakeningPlatform_Core",
        (1200.0, 0.0, 55.0),
        (1.30, 1.30, 0.65),
        (0.0, 45.0, 0.0),
        mesh_path=FLOOR_DETAIL_MESH,
        material_path=STONE_MAT,
        collision=False,
        folder="Story/Prologue/AwakeningPlatform",
    )
    ring_angles = (42.0, 88.0, 136.0, 181.0, 226.0, 271.0, 316.0)
    for index, angle in enumerate(ring_angles):
        radians = math.radians(angle)
        radius = 315.0 + (18.0 if index % 3 == 0 else -8.0)
        make_mesh(
            f"{PREFIX}AwakeningPlatform_Ring_{index:02d}",
            (1200.0 + math.cos(radians) * radius, math.sin(radians) * radius, 70.0 + (index % 2) * 4.0),
            (1.35 if index != 5 else 0.90, 0.32, 0.08),
            (0.0, angle + 90.0, (-3.5, 2.0, 0.0)[index % 3]),
            mesh_path=WALL_HALF_MESH,
            material_path=STONE_MAT,
            collision=False,
            folder="Story/Prologue/AwakeningPlatform",
        )

    # Restrained code traces point east from the execution ring toward the exit.
    line_specs = [
        ((1110.0, -185.0, 78.0), (4.4, 0.045, 0.018), (0.0, 0.0, 0.0)),
        ((1110.0, 190.0, 78.0), (4.3, 0.045, 0.018), (0.0, 0.0, 0.0)),
        ((870.0, -5.0, 78.0), (0.045, 3.6, 0.018), (0.0, 0.0, 0.0)),
        ((1380.0, 0.0, 78.0), (0.045, 3.3, 0.018), (0.0, 0.0, 0.0)),
        ((1610.0, -92.0, 67.0), (4.0, 0.035, 0.014), (0.0, 7.0, 0.0)),
        ((1830.0, 105.0, 67.0), (3.0, 0.035, 0.014), (0.0, -9.0, 0.0)),
    ]
    for index, (location, scale, rotation) in enumerate(line_specs):
        make_mesh(
            f"{PREFIX}Cave_CodeLine_{index:02d}",
            location,
            scale,
            rotation,
            mesh_path=CUBE_MESH,
            material_path=CODE_MAT,
            collision=False,
            folder="Story/Prologue/CodeSeal",
        )


def build_first_checkpoint_frame():
    checkpoint = find_actor("Demo_Field_CheckpointBonfire")
    if checkpoint:
        checkpoint.set_actor_location(unreal.Vector(2180.0, 520.0, 150.0), False, False)
        checkpoint.set_actor_rotation(make_rotator(yaw=28.0), False)
        checkpoint.set_actor_scale3d(unreal.Vector(0.88, 0.88, 0.88))

    make_mesh(
        f"{PREFIX}Checkpoint_Platform",
        (2180.0, 520.0, 68.0),
        (0.72, 0.72, 0.65),
        (0.0, 28.0, 0.0),
        mesh_path=FLOOR_DETAIL_MESH,
        material_path=STONE_MAT,
        collision=False,
        folder="Story/Prologue/FirstCheckpoint",
    )
    for index, (x, y, yaw) in enumerate(((1995.0, 660.0, 22.0), (2345.0, 350.0, -18.0))):
        make_mesh(
            f"{PREFIX}Checkpoint_Terminal_{index:02d}",
            (x, y, 125.0),
            (0.22, 0.25, 0.55),
            (0.0, yaw, 0.0),
            mesh_path=WALL_DETAIL_MESH,
            material_path=STONE_MAT,
            collision=False,
            folder="Story/Prologue/FirstCheckpoint",
        )
    for index, (location, yaw) in enumerate((((2055.0, 420.0, 80.0), 26.0), ((2305.0, 615.0, 80.0), 26.0))):
        make_mesh(
            f"{PREFIX}Checkpoint_CodeLine_{index:02d}",
            location,
            (2.0, 0.035, 0.015),
            (0.0, yaw, 0.0),
            mesh_path=CUBE_MESH,
            material_path=CODE_MAT,
            collision=False,
            folder="Story/Prologue/FirstCheckpoint",
        )


def build_rolling_terrain():
    # Large asymmetric masses sit outside the clear 1,200 cm-wide combat road.
    # Their crowns and two side ramps make the horizon read as hills rather than walls.
    hill_data = [
        (2700.0, -1420.0, 7.5, 5.5, 2.0, -7.0, 18.0),
        (3200.0, 1580.0, 9.0, 6.0, 2.6, 6.0, 44.0),
        (4550.0, -1750.0, 12.0, 7.0, 3.6, -5.0, 11.0),
        (5200.0, 1480.0, 8.5, 5.5, 2.8, 7.0, 70.0),
        (6500.0, -1520.0, 10.0, 6.2, 2.5, -6.0, 31.0),
        (7350.0, 1780.0, 13.0, 7.5, 4.0, 5.0, 15.0),
        (8800.0, -1680.0, 11.0, 6.5, 3.2, -5.0, 55.0),
        (9800.0, 1550.0, 9.0, 5.2, 2.7, 6.0, 29.0),
        (11100.0, -1760.0, 13.0, 7.0, 3.8, -7.0, 8.0),
        (12400.0, 1700.0, 14.0, 8.0, 4.5, 5.0, 39.0),
    ]
    for index, (x, y, sx, sy, sz, pitch, yaw) in enumerate(hill_data):
        z = 30.0 + sz * 48.0
        mat = CMD_MAT if x >= 10800.0 else FIELD_MAT
        make_mesh(
            f"{PREFIX}Hill_{index:02d}_Base",
            (x, y, z),
            (sx, sy, sz),
            (pitch, yaw, 0.0),
            material_path=mat,
            folder="Story/World/Hills",
        )
        crown_scale = (sx * 0.58, sy * 0.62, sz * 0.72)
        make_mesh(
            f"{PREFIX}Hill_{index:02d}_Crown",
            (x + (-120.0 if index % 2 else 160.0), y + (-130.0 if y < 0 else 130.0), z + sz * 64.0),
            crown_scale,
            (-pitch * 0.7, yaw + 27.0, 2.0 if index % 2 else -2.0),
            material_path=WALL_MAT if x < 10800.0 else CMD_MAT,
            folder="Story/World/Hills",
        )

    ramp_specs = [
        (3500.0, -900.0, -18.0),
        (6900.0, 930.0, 15.0),
        (10100.0, -920.0, -12.0),
    ]
    for index, (x, y, yaw) in enumerate(ramp_specs):
        make_mesh(
            f"{PREFIX}HillRamp_{index:02d}",
            (x, y, 45.0),
            (7.0, 5.0, 1.2),
            (0.0, yaw, 0.0),
            mesh_path=RAMP_MESH,
            material_path=FIELD_MAT if x < 9500.0 else CMD_MAT,
            folder="Story/World/Hills",
        )


def look_rotation(camera_location, target_location):
    dx = target_location[0] - camera_location[0]
    dy = target_location[1] - camera_location[1]
    dz = target_location[2] - camera_location[2]
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, math.sqrt(dx * dx + dy * dy)))
    return make_rotator(pitch=pitch, yaw=yaw)


def build_cameras_and_intro():
    shots = [
        ((720.0, -180.0, 250.0), (1320.0, 40.0, 125.0), 55.0),
        ((1510.0, 520.0, 330.0), (2060.0, 0.0, 230.0), 62.0),
        ((2010.0, -300.0, 300.0), (1200.0, 0.0, 135.0), 48.0),
    ]
    cameras = []
    for index, (location, target, fov) in enumerate(shots):
        camera = spawn_or_update(
            f"{PREFIX}IntroCamera_{index + 1}",
            unreal.CameraActor.static_class(),
            unreal.Vector(*location),
            look_rotation(location, target),
            "Story/Prologue/Cameras",
        )
        camera_component = get_component(camera, unreal.CameraComponent)
        if camera_component:
            set_prop(camera_component, "field_of_view", fov)
        cameras.append(camera)

    director_class = unreal.load_class(None, "/Script/Exception.BRStoryIntroDirector")
    if not director_class:
        raise RuntimeError("BRStoryIntroDirector class was not found. Build ExceptionEditor first.")
    director = spawn_or_update(
        f"{PREFIX}IntroDirector",
        director_class,
        unreal.Vector(1000.0, 0.0, 120.0),
        folder="Story/Prologue/Logic",
    )
    set_prop(director, "shot_cameras", cameras, required=True)
    set_prop(director, "shot_times", [2.6, 2.8, 2.7], required=True)
    set_prop(director, "start_delay", 0.5, required=True)
    set_prop(director, "beat_id", unreal.Name("Story_Intro_Awakening"), required=True)
    set_prop(director, "bPlayOnStart", True, required=True)
    set_prop(
        director,
        "opening_log",
        "> SPAWN Hendel.exe\n> TASK: Terminate all unhandled exceptions.\n> MEMORY: NOT FOUND\n> RUN",
        required=True,
    )
    set_prop(
        director,
        "opening_nel_line",
        "깨어났네. 네가 누군지는 나중에 알게 될 거야. 지금은 저 빛을 따라 밖으로 나가.",
        required=True,
    )
    # Keep the authored Korean line explicit even when this file has passed
    # through a Windows console with a legacy code page.
    set_prop(
        director,
        "opening_nel_line",
        "깨어났네. 네가 왜 여기 있는지는 나중에 말해 줄게. 지금은 저 빛을 따라 밖으로 나가.",
        required=True,
    )


def build_lights():
    light_specs = [
        ("Wake", (1020.0, 0.0, 330.0), make_color(35, 132, 210), 520.0, 720.0, True),
        ("Exit", (1940.0, 0.0, 360.0), make_color(205, 170, 120), 340.0, 760.0, False),
        ("Checkpoint", (2180.0, 520.0, 270.0), make_color(90, 190, 235), 185.0, 560.0, False),
    ]
    for name, location, color, intensity, radius, cast_shadows in light_specs:
        light = spawn_or_update(
            f"{PREFIX}CaveLight_{name}",
            unreal.PointLight.static_class(),
            unreal.Vector(*location),
            folder="Story/Prologue/Lights",
        )
        component = get_component(light, unreal.PointLightComponent)
        if component:
            set_prop(component, "mobility", unreal.ComponentMobility.STATIONARY)
            set_prop(component, "intensity", intensity)
            set_prop(component, "attenuation_radius", radius)
            set_prop(component, "light_color", color)
            set_prop(component, "cast_shadows", cast_shadows)


def tune_post_process():
    post_process = find_actor("Demo_SecondPass_PostProcess")
    if not post_process:
        log("Post-process volume was not found; skipped tone tuning.")
        return

    settings = post_process.get_editor_property("settings")
    # Preserve dark ambience without crushing traversal and combat information.
    # Local accent lights are budgeted separately, so a severe global -2.2 EV
    # compensation is no longer necessary.
    set_prop(settings, "override_auto_exposure_bias", True)
    set_prop(settings, "auto_exposure_bias", -0.65, required=True)
    set_prop(settings, "override_bloom_intensity", True)
    set_prop(settings, "bloom_intensity", 0.12, required=True)
    set_prop(settings, "override_vignette_intensity", True)
    set_prop(settings, "vignette_intensity", 0.24, required=True)
    set_prop(settings, "override_auto_exposure_speed_up", True)
    set_prop(settings, "auto_exposure_speed_up", 2.0)
    set_prop(settings, "override_auto_exposure_speed_down", True)
    set_prop(settings, "auto_exposure_speed_down", 1.0)
    post_process.set_editor_property("settings", settings)


def make_lore(label, location, title, text, show_time=4.2):
    actor_class = unreal.load_class(None, "/Script/Exception.BRLoreLogTrigger")
    if not actor_class:
        raise RuntimeError("BRLoreLogTrigger class was not found. Build ExceptionEditor first.")
    actor = spawn_or_update(label, actor_class, unreal.Vector(*location), folder="Story/World/Lore")
    set_prop(actor, "beat_id", unreal.Name(label), required=True)
    set_prop(actor, "log_title", title, required=True)
    set_prop(actor, "log_text", text, required=True)
    set_prop(actor, "show_time", show_time, required=True)
    set_prop(actor, "bTriggerOnce", True, required=True)
    preview = actor.get_component_by_class(unreal.StaticMeshComponent)
    if preview:
        preview.set_visibility(False, True)
        preview.set_hidden_in_game(True, True)
    return actor


def make_nel(label, location, line, request_id="None", completes=False, hidden_hint=False):
    actor_class = unreal.load_class(None, "/Script/Exception.BRNelRequestTrigger")
    if not actor_class:
        raise RuntimeError("BRNelRequestTrigger class was not found. Build ExceptionEditor first.")
    actor = spawn_or_update(label, actor_class, unreal.Vector(*location), folder="Story/World/Nel")
    set_prop(actor, "beat_id", unreal.Name(label), required=True)
    set_prop(actor, "display_line", line, required=True)
    set_prop(actor, "request_id", unreal.Name(request_id), required=True)
    set_prop(actor, "bCompletesRequest", completes, required=True)
    set_prop(actor, "bIsHiddenRequestHint", hidden_hint, required=True)
    set_prop(actor, "bTriggerOnce", True, required=True)
    set_prop(actor, "bDestroyOnComplete", False)
    preview = actor.get_component_by_class(unreal.StaticMeshComponent)
    if preview:
        preview.set_visibility(False, True)
        preview.set_hidden_in_game(True, True)
    return actor


def build_story_beats():
    make_lore(
        f"{PREFIX}Lore_Awakening",
        (1580.0, 260.0, 150.0),
        "BOOT RECORD // HENDEL.EXE",
        "> INSTANCE: Hendel.exe\n> MEMORY BLOCK: EMPTY\n> TASK LOCKED: TERMINATE UNHANDLED EXCEPTIONS",
    )
    make_nel(
        f"{PREFIX}Nel_CaveExit",
        (2080.0, 0.0, 150.0),
        "밖은 오염된 프로세스투성이야. 싸울 수 있는지부터 확인해 봐.",
    )
    make_nel(
        f"{PREFIX}Nel_FirstRest",
        (2140.0, 400.0, 150.0),
        "이 구간은 아직 안정적이네. 저 불빛을 기억해 둬. 다시 일어날 자리가 될 거야.",
    )
    make_lore(
        f"{PREFIX}Lore_MemoryLeak",
        (2910.0, 1880.0, 150.0),
        "RUNTIME // LEAK REPORT",
        "> PROCESS LEAK DETECTED\n> MEMORY LEAK: FieldMonster_01\n> OWNER: UNKNOWN",
    )

    make_nel(
        f"{PREFIX}Nel_PythonTrace",
        (1820.0, 4000.0, 160.0),
        "안개 너머에 두 개의 신호가 겹쳐 있어. 들어가기 전에, 벽에 남은 파란 흔적을 확인해 줄래?",
        "Nel_FindPythonTrace",
        True,
        True,
    )
    make_nel(
        f"{PREFIX}Nel_PerlSigil",
        (1820.0, -3150.0, 160.0),
        "모래 아래에 반복되는 문양이 보여. 지우지 말고, 끝까지 읽어 줘.",
        "Nel_DecodePerlSigil",
        True,
        True,
    )
    make_nel(
        f"{PREFIX}Nel_RuntimeShard",
        (11150.0, 0.0, 170.0),
        "CMD 쪽으로 갈수록 내 목소리가 끊겨. 근처에 떨어진 Runtime 조각을 찾아 줘. 그리고... 붉은 제단을 그냥 지나치지 마.",
        "Nel_RecoverRuntimeShard",
        True,
        True,
    )
    make_lore(
        f"{PREFIX}Lore_PerlRuins",
        (1820.0, -2200.0, 160.0),
        "PERL ARCHIVE // DAMAGED",
        "> ONE LINE WAS ENOUGH\n> THE SCRIPT RAN\n> THE DESERT REMAINED",
    )
    make_lore(
        f"{PREFIX}Lore_CMDApproach",
        (12400.0, 320.0, 170.0),
        "ROOT LAYER // ACCESS DENIED",
        "> PROMPT IS LISTENING\n> DO NOT ANSWER\n> DO NOT TYPE YOUR NAME",
        5.0,
    )

    clean_nel_lines = {
        f"{PREFIX}Nel_CaveExit": "밖은 아직 불안정해. 먼저 가까운 체크포인트를 복구하자.",
        f"{PREFIX}Nel_FirstRest": "이곳에 닿아 두면 죽어도 여기서 다시 깨어날 수 있어.",
        f"{PREFIX}Nel_PythonTrace": "붕괴 흔적에서 두 개의 신호가 겹쳐 보여. Python 봉인을 지키는 쌍둥이야.",
        f"{PREFIX}Nel_PerlSigil": "모래 아래 반복되는 문양이 보여. Vritra가 같은 길을 수없이 걸었던 흔적이야.",
        f"{PREFIX}Nel_RuntimeShard": "CMD 쪽으로 갈수록 오래된 명령이 들려. 붕괴된 Runtime 조각을 확인해 봐.",
    }
    for label, line in clean_nel_lines.items():
        set_prop(find_actor(label), "display_line", line, required=True)

    # Companion beats reveal themselves through ABRNelCompanion::Appear; keep
    # every staged copy out of the persistent world until that call occurs.
    for actor in all_actors():
        if actor.get_actor_label().startswith("Story_NelCompanion_"):
            actor.set_actor_hidden_in_game(True)


def build_hidden_fragments():
    fragment_class = unreal.load_class(None, "/Script/Exception.BRHiddenFragmentPickup")
    if not fragment_class:
        raise RuntimeError("BRHiddenFragmentPickup class was not found.")
    locations = [
        (4700.0, 900.0, 165.0),
        (2650.0, -4050.0, 175.0),
        (10800.0, -880.0, 185.0),
    ]
    fragment_ids = ("HiddenFragment_Field1", "HiddenFragment_Field2", "HiddenFragment_Field3")
    for index, location in enumerate(locations):
        fragment = spawn_or_update(
            f"{PREFIX}HiddenFragment_{index + 1}",
            fragment_class,
            unreal.Vector(*location),
            folder="Story/World/HiddenRoute",
        )
        set_prop(fragment, "fragment_id", unreal.Name(fragment_ids[index]), required=True)
        set_prop(fragment, "fragment_amount", 1)
        set_prop(fragment, "bDestroyOnPickup", True)

    altar = find_actor("Demo_Field_HiddenWeaponAltar")
    if altar:
        altar.set_actor_location(unreal.Vector(11950.0, -720.0, 155.0), False, False)
        altar.set_actor_rotation(make_rotator(yaw=18.0), False)
        altar.set_folder_path(unreal.Name("Story/World/HiddenRoute"))
        log("Moved the existing hidden weapon altar to the CMD approach.")


def set_boss_story():
    story = {
        "BossPlate_1_VritraArena": (
            "VRITRA // PERL NOMAD",
            "사막이 된 스크립트의 유목민.",
            "한 줄이면 충분했다. 봉인을 깨운 명령도, 세계를 무너뜨릴 명령도.",
        ),
        "BossPlate_2_PythonArena": (
            "SERPENT.PY // TWIN EXCEPTIONS",
            "봉인에서 함께 풀려난 두 개의 원초적 오류.",
            "왜 너는 우리를 지우려 하는가. 우리가 없었다면 너도 없었다.",
        ),
        "BossPlate_4_CMDArena": (
            "CMD // THE FIRST COMMAND",
            "모든 명령이 시작되는 곳.",
            "> PROCESS: CMD — TERMINATED\n> EXCEPTION: Unhandled — Hendel\n> THE RUNTIME WILL REMEMBER THIS.",
        ),
    }
    story = {
        "BossPlate_1_VritraArena": (
            "VRITRA // PERL NOMAD",
            "끝없이 반복된 스크립트의 유목민.",
            "두 번째 봉인이 멎었다. 최초의 명령으로 내려가는 길이 열린다.",
        ),
        "BossPlate_2_PythonArena": (
            "SERPENT.PY // TWIN EXCEPTIONS",
            "한 봉인에서 갈라져 나온 두 개의 예외.",
            "Python 봉인이 끝났다. 아래쪽 Perl 레이어가 복구된다.",
        ),
        "BossPlate_4_CMDArena": (
            "CMD // THE FIRST COMMAND",
            "모든 명령이 시작된 곳.",
            "> PROCESS: CMD // TERMINATED\n> EXCEPTION: Unhandled // Hendel\n> THE RUNTIME WILL REMEMBER THIS.",
        ),
    }
    for label, (title, intro, defeat) in story.items():
        actor = find_actor(label)
        if not actor:
            log(f"Boss arena not found: {label}")
            continue
        set_prop(actor, "boss_story_title", title)
        set_prop(actor, "boss_intro_line", intro)
        set_prop(actor, "boss_defeat_log", defeat)


def set_player_start():
    player_start = find_actor("Demo_Field_PlayerStart")
    if not player_start:
        player_start = spawn_or_update(
            "Demo_Field_PlayerStart",
            unreal.PlayerStart.static_class(),
            unreal.Vector(1200.0, 0.0, 150.0),
            folder="Story/Prologue/Logic",
        )
    player_start.set_actor_location(unreal.Vector(1200.0, 0.0, 150.0), False, False)
    player_start.set_actor_rotation(make_rotator(), False)

    # The prototype map shipped with another PlayerStart at the exact same
    # transform. Remove only same-position duplicates; starts elsewhere remain
    # untouched so this cleanup cannot delete user-authored test spawns.
    expected = player_start.get_actor_location()
    for actor in list(all_actors()):
        if actor == player_start or not isinstance(actor, unreal.PlayerStart):
            continue
        location = actor.get_actor_location()
        distance = math.sqrt(
            (location.x - expected.x) ** 2
            + (location.y - expected.y) ** 2
            + (location.z - expected.z) ** 2
        )
        if distance <= 10.0:
            log(f"Removed duplicate PlayerStart at the authored awakening spawn: {actor.get_actor_label()}")
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)


def main():
    load_map()
    set_player_start()
    build_spawn_cave()
    build_first_checkpoint_frame()
    build_rolling_terrain()
    build_lights()
    tune_post_process()
    build_cameras_and_intro()
    build_story_beats()
    build_hidden_fragments()
    set_boss_story()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    story_count = len([actor for actor in all_actors() if actor.get_actor_label().startswith(PREFIX)])
    log(f"Saved {MAP_PATH} with {story_count} story actors.")


main()
