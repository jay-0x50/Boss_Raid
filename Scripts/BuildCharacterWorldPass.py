import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ENCOUNTER_PREFIX = "EncounterBuild_"
NEL_PREFIX = "Story_NelCompanion_"

FORT = "/Game/ThirdParty/BossEnvironment/PolyHaven/SM_PH_ModularFort01"
GATE = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_Gate"
FLOOR = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_FloorBig"
BOULDER = "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD1"
FORT_WALL = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Wall"
FORT_TRIM = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Trim"
BOULDER_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Boulder01"
LEAK_MAT = "/Game/World/Environment/Materials/M_Wall_Boss_CMD"

SPAWNERS = {
    "A": {"label": "Demo_Field_EnemySpawner_A", "anchor": (2400.0, 320.0, 120.0), "side": 1.0, "count": 3, "alive": 2},
    "B": {"label": "Demo_Field_EnemySpawner_B", "anchor": (3600.0, -320.0, 120.0), "side": -1.0, "count": 4, "alive": 2},
    "C": {"label": "Demo_Field_EnemySpawner_C", "anchor": (5350.0, 330.0, 120.0), "side": 1.0, "count": 4, "alive": 2},
    "D": {"label": "Demo_Field_EnemySpawner_D", "anchor": (9000.0, -330.0, 120.0), "side": -1.0, "count": 5, "alive": 3},
}

NEL_BEATS = {
    "Awakening": {"location": (1500.0, 330.0, 95.0), "trigger": None},
    "CaveExit": {"location": (2220.0, 260.0, 95.0), "trigger": "Story_Nel_CaveExit"},
    "FirstRest": {"location": (2680.0, 170.0, 95.0), "trigger": "Story_Nel_FirstRest"},
    "PythonTrace": {"location": (2050.0, 4210.0, 105.0), "trigger": "Story_Nel_PythonTrace"},
    "PerlSigil": {"location": (2050.0, -3260.0, 105.0), "trigger": "Story_Nel_PerlSigil"},
    "RuntimeShard": {"location": (11380.0, 270.0, 115.0), "trigger": "Story_Nel_RuntimeShard"},
}


def log(message):
    unreal.log(f"[BuildCharacterWorldPass] {message}")


def load_asset(path, expected):
    asset = unreal.load_asset(path)
    if not isinstance(asset, expected):
        raise RuntimeError(f"Missing {expected.__name__}: {path}")
    return asset


def actor_subsystem():
    result = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not result:
        raise RuntimeError("EditorActorSubsystem is unavailable")
    return result


def actors():
    return list(actor_subsystem().get_all_level_actors())


def find(label):
    return next((actor for actor in actors() if actor.get_actor_label() == label), None)


def spawn_or_update(label, actor_class, location, rotation=(0.0, 0.0, 0.0), folder="World/Encounters"):
    subsystem = actor_subsystem()
    actor = find(label)
    if actor and actor.get_class() != actor_class:
        subsystem.destroy_actor(actor)
        actor = None
    rot = unreal.Rotator(roll=rotation[2], pitch=rotation[0], yaw=rotation[1])
    if not actor:
        actor = subsystem.spawn_actor_from_class(actor_class, unreal.Vector(*location), rot)
        if not actor:
            raise RuntimeError(f"Could not spawn {label}")
    actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_actor_rotation(rot, False)
    actor.set_folder_path(unreal.Name(folder))
    return actor


def mesh_actor(label, mesh, material, location, scale, rotation=(0.0, 0.0, 0.0), collision=False):
    actor = spawn_or_update(label, unreal.StaticMeshActor.static_class(), location, rotation)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    for index in range(max(1, component.get_num_materials())):
        if material:
            component.set_material(index, material)
    component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION
    )
    component.set_collision_profile_name(unreal.Name("BlockAll" if collision else "NoCollision"))
    actor.set_actor_enable_collision(collision)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def point_light(label, location, color, intensity=1800.0, radius=750.0):
    actor = spawn_or_update(label, unreal.PointLight.static_class(), location, folder="World/Encounters/Lights")
    component = actor.get_component_by_class(unreal.PointLightComponent)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("attenuation_radius", radius)
    component.set_editor_property("light_color", color)
    component.set_editor_property("cast_shadows", True)
    actor.set_actor_enable_collision(False)
    return actor


def build_spawn_buildings():
    fort = load_asset(FORT, unreal.StaticMesh)
    gate = load_asset(GATE, unreal.StaticMesh)
    floor = load_asset(FLOOR, unreal.StaticMesh)
    boulder = load_asset(BOULDER, unreal.StaticMesh)
    fort_wall = load_asset(FORT_WALL, unreal.MaterialInterface)
    fort_trim = load_asset(FORT_TRIM, unreal.MaterialInterface)
    boulder_mat = load_asset(BOULDER_MAT, unreal.MaterialInterface)
    leak_mat = load_asset(LEAK_MAT, unreal.MaterialInterface)

    desired = set()
    for key, data in SPAWNERS.items():
        spawner = find(data["label"])
        if not spawner:
            raise RuntimeError(f"Missing encounter spawner: {data['label']}")

        x, y, z = data["anchor"]
        side = data["side"]
        outward_yaw = -90.0 if side > 0.0 else 90.0
        spawner.set_actor_location(unreal.Vector(x, y + side * 165.0, z), False, False)
        spawner.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=outward_yaw), False)
        spawner.set_folder_path(unreal.Name(f"World/Encounters/{key}/Logic"))
        for prop, value in (
            ("should_spawn_enemies_immediately", True),
            ("initial_spawn_delay", 0.6),
            ("wait_for_player_when_auto_spawning", True),
            ("auto_activation_distance", 1750.0),
            ("spawn_count", data["count"]),
            ("max_alive_enemies", data["alive"]),
            ("spawn_spread_radius", 95.0),
            ("respawn_delay", 1.8),
        ):
            spawner.set_editor_property(prop, value)

        building_y = y + side * 345.0
        specs = [
            ("Foundry", fort, None, (x, building_y, z - 82.0), (0.105, 0.105, 0.23), (0.0, outward_yaw, 0.0), False),
            ("DoorArch", gate, fort_wall, (x, y + side * 78.0, z + 90.0), (0.92, 0.92, 0.92), (0.0, outward_yaw, 0.0), False),
            ("Threshold", floor, fort_trim, (x, y - side * 65.0, z - 82.0), (1.25, 1.10, 0.42), (0.0, outward_yaw, 0.0), False),
            ("RubbleL", boulder, boulder_mat, (x - 275.0, y + side * 65.0, z - 35.0), (2.5, 2.2, 2.0), (7.0, outward_yaw + 24.0, -5.0), True),
            ("RubbleR", boulder, boulder_mat, (x + 275.0, y + side * 65.0, z - 35.0), (2.2, 2.6, 1.8), (-5.0, outward_yaw - 31.0, 6.0), True),
            ("LeakCore", gate, leak_mat, (x, y + side * 245.0, z + 82.0), (0.54, 0.54, 0.72), (0.0, outward_yaw, 0.0), False),
        ]
        for suffix, mesh, mat, loc, scale, rot, collision in specs:
            label = f"{ENCOUNTER_PREFIX}{key}_{suffix}"
            desired.add(label)
            actor = mesh_actor(label, mesh, mat, loc, scale, rot, collision)
            actor.set_folder_path(unreal.Name(f"World/Encounters/{key}/Building"))

        light_label = f"{ENCOUNTER_PREFIX}{key}_SpawnLight"
        desired.add(light_label)
        color = unreal.Color(255, 38, 82, 255) if key in ("B", "D") else unreal.Color(35, 168, 255, 255)
        point_light(light_label, (x, y + side * 165.0, z + 170.0), color)

    for actor in actors():
        if actor.get_actor_label().startswith(ENCOUNTER_PREFIX) and actor.get_actor_label() not in desired:
            actor_subsystem().destroy_actor(actor)
    return len(desired)


def build_nel_appearances():
    nel_class = unreal.load_class(None, "/Script/Exception.BRNelCompanion")
    if not nel_class:
        raise RuntimeError("BRNelCompanion class was not found. Build ExceptionEditor first.")

    desired = set()
    companions = {}
    for key, data in NEL_BEATS.items():
        label = f"{NEL_PREFIX}{key}"
        desired.add(label)
        companion = spawn_or_update(
            label,
            nel_class,
            data["location"],
            folder="Story/World/Nel/Appearances",
        )
        companion.set_actor_scale3d(unreal.Vector(0.92, 0.92, 0.92))
        companions[key] = companion
        trigger_label = data["trigger"]
        if trigger_label:
            trigger = find(trigger_label)
            if not trigger:
                raise RuntimeError(f"Missing Nel trigger: {trigger_label}")
            trigger.set_editor_property("nel_companion", companion)
            trigger.set_editor_property("show_nel_companion", True)
            trigger.set_editor_property("nel_visible_time", 6.5)

    director = find("Story_IntroDirector")
    if not director:
        raise RuntimeError("Story_IntroDirector was not found")
    director.set_editor_property("opening_nel_companion", companions["Awakening"])

    for actor in actors():
        if actor.get_actor_label().startswith(NEL_PREFIX) and actor.get_actor_label() not in desired:
            actor_subsystem().destroy_actor(actor)
    return len(desired)


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")
    building_count = build_spawn_buildings()
    nel_count = build_nel_appearances()
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.save_current_level():
        raise RuntimeError(f"Could not save {MAP_PATH}")
    log(f"Saved {MAP_PATH}: {building_count} encounter building actors and {nel_count} Nel appearances.")


main()
