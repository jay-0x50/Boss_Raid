import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
PREFIX = "Story_"
FOLDER = "Story/Prologue"

ROCK_MESH = "/Game/LevelPrototyping/Meshes/SM_ChamferCube"
RAMP_MESH = "/Game/LevelPrototyping/Meshes/SM_Ramp"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"
FIELD_MAT = "/Game/World/Environment/Materials/M_Floor_Field_Default"
WALL_MAT = "/Game/World/Environment/Materials/M_Wall_Corridor_Default"
CODE_MAT = "/Game/World/Environment/Materials/M_Floor_Boss_Python"
CMD_MAT = "/Game/World/Environment/Materials/M_Wall_Boss_CMD"


def log(message):
    unreal.log(f"[BuildStoryPrologue] {message}")


def load_map():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")


def all_actors():
    return unreal.EditorLevelLibrary.get_all_level_actors()


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
    if actor and not actor.is_a(actor_class):
        unreal.EditorLevelLibrary.destroy_actor(actor)
        actor = None
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            actor_class,
            location,
            rotation or unreal.Rotator(0.0, 0.0, 0.0),
        )
        if not actor:
            raise RuntimeError(f"Failed to spawn {label}")
        actor.set_actor_label(label)
    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(rotation or unreal.Rotator(0.0, 0.0, 0.0), False)
    actor.set_folder_path(unreal.Name(folder))
    return actor


def make_mesh(label, location, scale, rotation=(0.0, 0.0, 0.0), mesh_path=ROCK_MESH,
              material_path=WALL_MAT, collision=True, folder="Story/Prologue/Cave"):
    actor = spawn_or_update(
        label,
        unreal.StaticMeshActor.static_class(),
        unreal.Vector(*location),
        unreal.Rotator(*rotation),
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
    actor.set_actor_enable_collision(collision)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def build_spawn_cave():
    specs = [
        ("BackWall", (360.0, 0.0, 365.0), (2.2, 14.0, 7.0), (0.0, 3.0, 0.0)),
        ("WallLeft", (1190.0, -720.0, 365.0), (16.5, 2.5, 6.8), (0.0, 1.0, -2.0)),
        ("WallRight", (1170.0, 735.0, 350.0), (16.0, 2.7, 6.5), (0.0, -2.0, 2.5)),
        ("Roof", (1190.0, 0.0, 735.0), (16.5, 14.5, 1.7), (0.0, 0.0, -1.5)),
        ("ExitPillarLeft", (2050.0, -570.0, 330.0), (3.2, 3.0, 6.0), (0.0, 18.0, -3.0)),
        ("ExitPillarRight", (2050.0, 575.0, 340.0), (3.0, 3.4, 6.2), (0.0, -14.0, 3.0)),
        ("ExitLintel", (2050.0, 0.0, 680.0), (3.0, 12.0, 1.8), (0.0, 2.0, 0.0)),
        ("RockA", (640.0, -510.0, 220.0), (4.5, 3.5, 3.4), (8.0, 21.0, -10.0)),
        ("RockB", (700.0, 530.0, 235.0), (4.0, 3.8, 3.8), (-6.0, 68.0, 9.0)),
        ("RockC", (1460.0, -560.0, 205.0), (5.5, 2.8, 3.2), (5.0, 14.0, -8.0)),
        ("RockD", (1530.0, 580.0, 235.0), (4.8, 3.0, 3.8), (-8.0, 52.0, 7.0)),
    ]
    for name, location, scale, rotation in specs:
        make_mesh(f"{PREFIX}Cave_{name}", location, scale, rotation)

    # A dark stone bed masks the original flat slab inside the awakening room.
    make_mesh(
        f"{PREFIX}Cave_Floor",
        (1180.0, 0.0, 25.0),
        (17.0, 13.5, 0.18),
        material_path=FIELD_MAT,
        folder="Story/Prologue/Cave",
    )

    # Thin emissive strips form the boot seal around Hendel without blocking movement.
    line_specs = [
        ((1050.0, -230.0, 48.0), (5.0, 0.06, 0.03), (0.0, 0.0, 0.0)),
        ((1050.0, 230.0, 48.0), (5.0, 0.06, 0.03), (0.0, 0.0, 0.0)),
        ((820.0, 0.0, 48.0), (0.06, 4.6, 0.03), (0.0, 0.0, 0.0)),
        ((1280.0, 0.0, 48.0), (0.06, 4.6, 0.03), (0.0, 0.0, 0.0)),
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
    return unreal.Rotator(pitch, yaw, 0.0)


def build_cameras_and_intro():
    shots = [
        ((620.0, -500.0, 285.0), (1200.0, 0.0, 130.0), 55.0),
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


def build_lights():
    light_specs = [
        ("Wake", (1000.0, 0.0, 360.0), unreal.Color(25, 150, 255, 255), 4200.0, 780.0),
        ("Exit", (1940.0, 0.0, 420.0), unreal.Color(255, 190, 115, 255), 5600.0, 900.0),
    ]
    for name, location, color, intensity, radius in light_specs:
        light = spawn_or_update(
            f"{PREFIX}CaveLight_{name}",
            unreal.PointLight.static_class(),
            unreal.Vector(*location),
            folder="Story/Prologue/Lights",
        )
        component = get_component(light, unreal.PointLightComponent)
        if component:
            set_prop(component, "intensity", intensity)
            set_prop(component, "attenuation_radius", radius)
            set_prop(component, "light_color", color)
            set_prop(component, "cast_shadows", True)


def make_lore(label, location, title, text, show_time=4.2):
    actor_class = unreal.load_class(None, "/Script/Exception.BRLoreLogTrigger")
    if not actor_class:
        raise RuntimeError("BRLoreLogTrigger class was not found. Build ExceptionEditor first.")
    actor = spawn_or_update(label, actor_class, unreal.Vector(*location), folder="Story/World/Lore")
    set_prop(actor, "log_title", title, required=True)
    set_prop(actor, "log_text", text, required=True)
    set_prop(actor, "show_time", show_time, required=True)
    set_prop(actor, "bTriggerOnce", True, required=True)
    return actor


def make_nel(label, location, line, request_id="None", completes=False, hidden_hint=False):
    actor_class = unreal.load_class(None, "/Script/Exception.BRNelRequestTrigger")
    if not actor_class:
        raise RuntimeError("BRNelRequestTrigger class was not found. Build ExceptionEditor first.")
    actor = spawn_or_update(label, actor_class, unreal.Vector(*location), folder="Story/World/Nel")
    set_prop(actor, "display_line", line, required=True)
    set_prop(actor, "request_id", unreal.Name(request_id), required=True)
    set_prop(actor, "bCompletesRequest", completes, required=True)
    set_prop(actor, "bIsHiddenRequestHint", hidden_hint, required=True)
    set_prop(actor, "bTriggerOnce", True, required=True)
    set_prop(actor, "bDestroyOnComplete", False)
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
        (2500.0, -120.0, 150.0),
        "이 구간은 아직 안정적이네. 저 불빛을 기억해 둬. 다시 일어날 자리가 될 거야.",
    )
    make_lore(
        f"{PREFIX}Lore_MemoryLeak",
        (3380.0, -300.0, 150.0),
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


def build_hidden_fragments():
    fragment_class = unreal.load_class(None, "/Script/Exception.BRHiddenFragmentPickup")
    if not fragment_class:
        raise RuntimeError("BRHiddenFragmentPickup class was not found.")
    locations = [
        (4700.0, 900.0, 165.0),
        (2650.0, -4050.0, 175.0),
        (10800.0, -880.0, 185.0),
    ]
    for index, location in enumerate(locations):
        fragment = spawn_or_update(
            f"{PREFIX}HiddenFragment_{index + 1}",
            fragment_class,
            unreal.Vector(*location),
            folder="Story/World/HiddenRoute",
        )
        set_prop(fragment, "fragment_amount", 1)
        set_prop(fragment, "bDestroyOnPickup", True)

    altar = find_actor("Demo_Field_HiddenWeaponAltar")
    if altar:
        altar.set_actor_location(unreal.Vector(11950.0, -720.0, 155.0), False, False)
        altar.set_actor_rotation(unreal.Rotator(0.0, 18.0, 0.0), False)
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
    player_start.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)


def main():
    load_map()
    set_player_start()
    build_spawn_cave()
    build_rolling_terrain()
    build_lights()
    build_cameras_and_intro()
    build_story_beats()
    build_hidden_fragments()
    set_boss_story()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    story_count = len([actor for actor in all_actors() if actor.get_actor_label().startswith(PREFIX)])
    log(f"Saved {MAP_PATH} with {story_count} story actors.")


main()
