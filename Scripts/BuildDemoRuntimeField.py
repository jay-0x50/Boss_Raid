import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
FLOOR_FIELD_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Floor_Field_Default"
FLOOR_PYTHON_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Floor_Boss_Python"
FLOOR_CAMEL_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Floor_Boss_Camel"
FLOOR_CMD_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Floor_Boss_CMD"
WALL_CORRIDOR_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Wall_Corridor_Default"
WALL_CMD_MATERIAL_PATH = "/Game/World/Environment/Materials/M_Wall_Boss_CMD"
SPAWNER_BP_PATH = "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemySpawner"
ENEMY_BP_PATH = "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemy"

PREFIX = "Demo_Field_"
BOSS_BP_DIR = "/Game/Blueprints/Bosses"
ROOM_SCALE = 2.5


def log(message):
    unreal.log(f"[BuildDemoRuntimeField] {message}")


def load_map():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")
    log(f"Loaded map: {MAP_PATH}")


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def get_component(actor, component_class):
    try:
        return actor.get_component_by_class(component_class)
    except Exception:
        pass

    try:
        components = actor.get_components_by_class(component_class)
        if components:
            return components[0]
    except Exception:
        pass

    return None


def set_prop(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
        return True
    except Exception as exc:
        log(f"Skipped {prop_name} on {obj}: {exc}")
        return False


def spawn_or_move(label, actor_class, location, rotation=unreal.Rotator(0.0, 0.0, 0.0)):
    actor = find_actor(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
        actor.set_actor_label(label)
        log(f"Spawned {label}")
    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(rotation, False)
    return actor


def cleanup_legacy_demo_actors():
    legacy_labels = {
        "BossPlate_2_VritraArena",
        "BossPlate_3_CMDArena",
        "Demo_CMD_PlayerStart",
        "Demo_CMD_ArenaFloor",
        "Demo_CMD_ApproachCorridor",
        "Demo_CMD_EntryGate",
        "Demo_CMD_ExitGate",
        "Demo_CMD_ClearReward",
        "Demo_CMD_ArenaSign",
        "Demo_CMD_Wall_North",
        "Demo_CMD_Wall_South",
        "Demo_CMD_Wall_East",
        "Demo_CMD_Wall_West_Left",
        "Demo_CMD_Wall_West_Right",
        "Demo_CMD_CorridorWall_North",
        "Demo_CMD_CorridorWall_South",
    }

    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        if actor.get_actor_label() in legacy_labels:
            unreal.EditorLevelLibrary.destroy_actor(actor)
            log(f"Removed legacy actor: {actor.get_actor_label()}")


def mesh_block(label, location, scale, collision=True, material_path=None):
    actor = spawn_or_move(label, unreal.StaticMeshActor.static_class(), location)
    actor.set_actor_scale3d(scale)
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
    material = unreal.EditorAssetLibrary.load_asset(material_path) if material_path else None
    component = get_component(actor, unreal.StaticMeshComponent)
    if component:
        component.set_static_mesh(mesh)
        if material and isinstance(material, unreal.MaterialInterface):
            component.set_material(0, material)
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_enable_collision(collision)
    return actor


def text_label(label, text, location, size=82.0):
    actor = spawn_or_move(label, unreal.TextRenderActor.static_class(), location)
    component = get_component(actor, unreal.TextRenderComponent)
    if component:
        component.set_text(text)
        component.set_editor_property("world_size", size)
    return actor


def load_bp_class(path):
    return unreal.EditorAssetLibrary.load_blueprint_class(path)


def find_first_asset(root, class_names, keywords):
    if not unreal.EditorAssetLibrary.does_directory_exist(root):
        return None

    candidates = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset and asset.get_class().get_name() in class_names:
            candidates.append((asset, asset_path))

    for keyword in keywords:
        lowered_keyword = keyword.lower()
        for asset, asset_path in candidates:
            if lowered_keyword in asset_path.lower():
                log(f"Found preferred asset for {root}: {asset_path}")
                return asset

    return candidates[0][0] if candidates else None


def create_or_update_boss_bp(bp_name, parent_class_path, asset_root, keywords, scale):
    bp_path = f"{BOSS_BP_DIR}/{bp_name}"
    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        log(f"Skipped {bp_name}: missing parent class {parent_class_path}")
        return None

    blueprint = unreal.EditorAssetLibrary.load_asset(bp_path) if unreal.EditorAssetLibrary.does_asset_exist(bp_path) else None
    if not blueprint:
        unreal.EditorAssetLibrary.make_directory(BOSS_BP_DIR)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("ParentClass", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(bp_name, BOSS_BP_DIR, unreal.Blueprint, factory)
        log(f"Created boss blueprint: {bp_path}")

    generated_class = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
    if not generated_class:
        return None

    cdo = unreal.get_default_object(generated_class)
    skeletal_mesh = find_first_asset(asset_root, {"SkeletalMesh"}, keywords)
    static_mesh = find_first_asset(asset_root, {"StaticMesh"}, keywords)
    material = find_first_asset(asset_root, {"Material", "MaterialInstanceConstant", "MaterialInstance"}, ["Material", "Perl", "SQL", "Aurathos"])

    if skeletal_mesh:
        set_prop(cdo, "VisualMeshType", unreal.BRBossVisualMeshType.SKELETAL_MESH)
        component = get_component(cdo, unreal.SkeletalMeshComponent)
        if component:
            component.set_skeletal_mesh(skeletal_mesh)
            component.set_editor_property("hidden_in_game", False)
            component.set_editor_property("visible", True)
            if material and isinstance(material, unreal.MaterialInterface):
                component.set_material(0, material)
    elif static_mesh:
        set_prop(cdo, "VisualMeshType", unreal.BRBossVisualMeshType.STATIC_MESH)
        component = get_component(cdo, unreal.StaticMeshComponent)
        if component:
            component.set_static_mesh(static_mesh)
            component.set_editor_property("hidden_in_game", False)
            component.set_editor_property("visible", True)
            if material and isinstance(material, unreal.MaterialInterface):
                component.set_material(0, material)

    set_prop(cdo, "MeshRelativeLocation", unreal.Vector(0.0, 0.0, -90.0))
    set_prop(cdo, "MeshRelativeScale", scale)
    set_prop(cdo, "bCombatAIEnabled", False)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return generated_class


def configure_arena(label, location, boss_class, spawn_offset, gate=None, reward=None):
    arena_class = unreal.load_class(None, "/Script/Exception.BRBossArenaTrigger")
    if not arena_class or not boss_class:
        return None

    arena = spawn_or_move(label, arena_class, location)
    set_prop(arena, "BossClassToSpawn", boss_class)
    set_prop(arena, "BossSpawnOffset", spawn_offset)
    set_prop(arena, "bSpawnBossOnArenaStart", True)
    set_prop(arena, "bResetBossOnEnter", True)
    set_prop(arena, "bAutoIncludeNearbyBosses", False)
    set_prop(arena, "bAutoIncludeTeamMembers", False)
    set_prop(arena, "bStartOnPlayerOverlap", True)
    set_prop(arena, "bDeactivateUnmanagedBossesOnStart", True)
    set_prop(arena, "bPlayBossIntroBeforeAI", True)
    set_prop(arena, "BossIntroDelay", 1.0)
    if gate:
        set_prop(arena, "GateActorToHideOnDefeat", gate)
    if reward:
        set_prop(arena, "RewardActorToShowOnDefeat", reward)

    trigger_box = get_component(arena, unreal.BoxComponent)
    if trigger_box:
        trigger_box.set_box_extent(unreal.Vector(520.0, 900.0, 260.0))
    return arena


def build_room(name, center, title, boss_class, spawn_offset):
    if "CMDArena" in name:
        floor_material = FLOOR_CMD_MATERIAL_PATH
        wall_material = WALL_CMD_MATERIAL_PATH
    elif "VritraArena" in name:
        floor_material = FLOOR_CAMEL_MATERIAL_PATH
        wall_material = WALL_CORRIDOR_MATERIAL_PATH
    else:
        floor_material = FLOOR_PYTHON_MATERIAL_PATH
        wall_material = WALL_CORRIDOR_MATERIAL_PATH

    mesh_block(f"{PREFIX}{name}_Floor", center, unreal.Vector(18.0 * ROOM_SCALE, 14.0 * ROOM_SCALE, 0.22), material_path=floor_material)
    mesh_block(f"{PREFIX}{name}_Wall_N", center + unreal.Vector(0.0, 750.0 * ROOM_SCALE, 300.0), unreal.Vector(18.5 * ROOM_SCALE, 0.3, 6.0), material_path=wall_material)
    mesh_block(f"{PREFIX}{name}_Wall_S", center + unreal.Vector(0.0, -750.0 * ROOM_SCALE, 300.0), unreal.Vector(18.5 * ROOM_SCALE, 0.3, 6.0), material_path=wall_material)
    mesh_block(f"{PREFIX}{name}_Wall_E", center + unreal.Vector(925.0 * ROOM_SCALE, 0.0, 300.0), unreal.Vector(0.3, 14.5 * ROOM_SCALE, 6.0), material_path=wall_material)
    mesh_block(f"{PREFIX}{name}_Wall_W_L", center + unreal.Vector(-925.0 * ROOM_SCALE, 1180.0, 300.0), unreal.Vector(0.3, 8.8, 6.0), material_path=wall_material)
    mesh_block(f"{PREFIX}{name}_Wall_W_R", center + unreal.Vector(-925.0 * ROOM_SCALE, -1180.0, 300.0), unreal.Vector(0.3, 8.8, 6.0), material_path=wall_material)
    gate = mesh_block(f"{PREFIX}{name}_ExitGate", center + unreal.Vector(1000.0 * ROOM_SCALE, 0.0, 270.0), unreal.Vector(0.38, 8.5, 5.4))
    reward = mesh_block(f"{PREFIX}{name}_ClearReward", center + unreal.Vector(1130.0 * ROOM_SCALE, 0.0, 140.0), unreal.Vector(0.85, 0.85, 0.85), collision=False)
    reward.set_actor_hidden_in_game(True)
    text_label(f"{PREFIX}{name}_Sign", title, center + unreal.Vector(-1800.0, 0.0, 300.0), 110.0)
    return configure_arena(f"BossPlate_{name}", center + unreal.Vector(-2480.0, 0.0, 85.0), boss_class, spawn_offset, gate, reward)


def place_spawner(label, location, count):
    spawner_class = load_bp_class(SPAWNER_BP_PATH)
    enemy_class = load_bp_class(ENEMY_BP_PATH)
    if not spawner_class or not enemy_class:
        log("Skipped enemy spawner placement: missing BP_CombatEnemySpawner or BP_CombatEnemy")
        return None

    spawner = spawn_or_move(label, spawner_class, location)
    set_prop(spawner, "EnemyClass", enemy_class)
    set_prop(spawner, "bShouldSpawnEnemiesImmediately", True)
    set_prop(spawner, "InitialSpawnDelay", 1.0)
    set_prop(spawner, "SpawnCount", count)
    set_prop(spawner, "RespawnDelay", 1.5)
    return spawner


def build_field():
    mesh_block(f"{PREFIX}MainPath_A", unreal.Vector(2300.0, 0.0, 20.0), unreal.Vector(22.0, 6.0, 0.2), material_path=FLOOR_FIELD_MATERIAL_PATH)
    mesh_block(f"{PREFIX}MainPath_B", unreal.Vector(5000.0, 0.0, 20.0), unreal.Vector(32.0, 6.0, 0.2), material_path=FLOOR_FIELD_MATERIAL_PATH)
    mesh_block(f"{PREFIX}MainPath_C", unreal.Vector(8600.0, 0.0, 20.0), unreal.Vector(36.0, 6.0, 0.2), material_path=FLOOR_FIELD_MATERIAL_PATH)
    mesh_block(f"{PREFIX}MainPath_D", unreal.Vector(12400.0, 0.0, 20.0), unreal.Vector(38.0, 6.0, 0.2), material_path=FLOOR_FIELD_MATERIAL_PATH)
    for index, x in enumerate([2100.0, 2800.0, 3600.0, 5200.0, 7400.0, 9800.0, 11800.0]):
        mesh_block(f"{PREFIX}Cover_{index}", unreal.Vector(x, -360.0 if index % 2 else 360.0, 95.0), unreal.Vector(1.8, 1.0, 1.6))

    place_spawner(f"{PREFIX}EnemySpawner_A", unreal.Vector(2400.0, 320.0, 120.0), 2)
    place_spawner(f"{PREFIX}EnemySpawner_B", unreal.Vector(3600.0, -320.0, 120.0), 3)
    place_spawner(f"{PREFIX}EnemySpawner_C", unreal.Vector(5350.0, 330.0, 120.0), 3)
    place_spawner(f"{PREFIX}EnemySpawner_D", unreal.Vector(9000.0, -330.0, 120.0), 3)
    # Keep the layer name as small diegetic wayfinding instead of a prototype
    # banner that blocks the opening cinematic.
    text_label(f"{PREFIX}FieldSign", "RUNTIME // FIELD 0", unreal.Vector(1980.0, -500.0, 160.0), 24.0)


def main():
    load_map()
    cleanup_legacy_demo_actors()

    cmd_class = load_bp_class("/Game/Blueprints/Bosses/BP_CMDBoss")
    vritra_class = load_bp_class("/Game/Blueprints/Bosses/BP_VritraBoss")
    python_class = create_or_update_boss_bp(
        "BP_PythonBoss",
        "/Script/Exception.BRPythonBoss",
        "/Game/Bosses/Python/Vethara",
        ["Walking_withSkin", "Vethara", "Meshy"],
        unreal.Vector(1.0, 1.0, 1.0),
    )
    selvara_class = create_or_update_boss_bp(
        "BP_SelvaraBoss",
        "/Script/Exception.BRSelvaraBoss",
        "/Game/Bosses/SQL",
        ["Neon_Circuit", "SQL", "Dragon"],
        unreal.Vector(1.0, 1.0, 1.0),
    )

    build_field()
    build_room("1_VritraArena", unreal.Vector(4300.0, -5200.0, 80.0), "PERL Vritra", vritra_class, unreal.Vector(1300.0, 0.0, 0.0))
    build_room("2_PythonArena", unreal.Vector(4300.0, 5200.0, 80.0), "PYTHON Twin", python_class, unreal.Vector(1300.0, 0.0, 0.0))
    build_room("3_SelvaraArena", unreal.Vector(9500.0, 5200.0, 80.0), "SQL Selvara", selvara_class, unreal.Vector(1300.0, 0.0, 0.0))
    build_room("4_CMDArena", unreal.Vector(14800.0, 0.0, 80.0), "CMD Final Boss", cmd_class, unreal.Vector(1600.0, 0.0, 0.0))

    player_start = spawn_or_move(f"{PREFIX}PlayerStart", unreal.PlayerStart.static_class(), unreal.Vector(1200.0, 0.0, 150.0))
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(actor, unreal.PlayerStart):
            actor.set_actor_location(unreal.Vector(1200.0, 0.0, 150.0), False, False)
            actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)

    unreal.EditorLevelLibrary.set_selected_level_actors([player_start])
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved runtime field demo layout.")


main()
