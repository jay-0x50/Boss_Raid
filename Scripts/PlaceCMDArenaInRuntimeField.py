import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ARENA_LABEL = "BossPlate_3_CMDArena"
BP_CMD_PATH = "/Game/Blueprints/Bosses/BP_CMDBoss"
PREFIX = "Demo_CMD_"

PLAYER_START_LABEL = f"{PREFIX}PlayerStart"
FLOOR_LABEL = f"{PREFIX}ArenaFloor"
CORRIDOR_LABEL = f"{PREFIX}ApproachCorridor"
ENTRY_GATE_LABEL = f"{PREFIX}EntryGate"
EXIT_GATE_LABEL = f"{PREFIX}ExitGate"
REWARD_LABEL = f"{PREFIX}ClearReward"
SIGN_LABEL = f"{PREFIX}ArenaSign"

CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
ARENA_CENTER = unreal.Vector(6200.0, 0.0, 80.0)
TRIGGER_LOCATION = unreal.Vector(4750.0, 0.0, 140.0)
BOSS_OFFSET = unreal.Vector(1550.0, 0.0, 0.0)
PLAYER_START_LOCATION = unreal.Vector(2250.0, 0.0, 150.0)


def log(message):
    unreal.log(f"[PlaceCMDArena] {message}")


def load_map():
    result = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if not result:
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")
    log(f"Loaded map: {MAP_PATH}")


def find_actor_by_label(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def set_property_if_exists(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
        log(f"Set {prop_name} = {value}")
        return True
    except Exception as exc:
        log(f"Skipped {prop_name}: {exc}")
        return False


def get_component(actor, component_class):
    if not actor:
        return None

    try:
        return actor.get_component_by_class(component_class)
    except Exception:
        return None


def create_or_reuse_actor(label, actor_class, location, rotation=unreal.Rotator(0.0, 0.0, 0.0)):
    actor = find_actor_by_label(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
        actor.set_actor_label(label)
        log(f"Spawned {label}")
    else:
        log(f"Reusing existing {label}")

    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(rotation, False)
    return actor


def create_or_reuse_static_mesh(label, location, scale, material=None):
    actor = create_or_reuse_actor(label, unreal.StaticMeshActor.static_class(), location)
    actor.set_actor_scale3d(scale)

    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
    component = get_component(actor, unreal.StaticMeshComponent)
    if component and mesh:
        component.set_static_mesh(mesh)
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        if material:
            component.set_material(0, material)

    return actor


def set_actor_collision(actor, enabled):
    try:
        actor.set_actor_enable_collision(enabled)
    except Exception:
        pass

    component = get_component(actor, unreal.StaticMeshComponent)
    if component:
        component.set_collision_enabled(
            unreal.CollisionEnabled.QUERY_AND_PHYSICS if enabled else unreal.CollisionEnabled.NO_COLLISION
        )


def make_material(name, color):
    material_path = f"/Game/Materials/Demo/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        return unreal.EditorAssetLibrary.load_asset(material_path)

    unreal.EditorAssetLibrary.make_directory("/Game/Materials/Demo")
    factory = unreal.MaterialFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(name, "/Game/Materials/Demo", unreal.Material, factory)
    if material:
        try:
            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "BaseColor", color)
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def build_demo_room():
    floor_mat = None
    wall_mat = None
    gate_mat = None
    reward_mat = None

    # Materials are optional; the room still works if material editing APIs are unavailable.
    try:
        floor_mat = make_material("M_Demo_CMD_Floor", unreal.LinearColor(0.04, 0.045, 0.05, 1.0))
        wall_mat = make_material("M_Demo_CMD_Wall", unreal.LinearColor(0.10, 0.11, 0.13, 1.0))
        gate_mat = make_material("M_Demo_CMD_Gate", unreal.LinearColor(0.65, 0.08, 0.12, 1.0))
        reward_mat = make_material("M_Demo_CMD_Reward", unreal.LinearColor(0.0, 0.7, 1.0, 1.0))
    except Exception as exc:
        log(f"Material setup skipped: {exc}")

    create_or_reuse_static_mesh(FLOOR_LABEL, ARENA_CENTER, unreal.Vector(22.0, 16.0, 0.25), floor_mat)
    create_or_reuse_static_mesh(CORRIDOR_LABEL, unreal.Vector(3500.0, 0.0, 20.0), unreal.Vector(24.0, 4.0, 0.22), floor_mat)

    create_or_reuse_static_mesh(f"{PREFIX}Wall_North", ARENA_CENTER + unreal.Vector(0.0, 850.0, 250.0), unreal.Vector(22.5, 0.35, 5.0), wall_mat)
    create_or_reuse_static_mesh(f"{PREFIX}Wall_South", ARENA_CENTER + unreal.Vector(0.0, -850.0, 250.0), unreal.Vector(22.5, 0.35, 5.0), wall_mat)
    create_or_reuse_static_mesh(f"{PREFIX}Wall_East", ARENA_CENTER + unreal.Vector(1125.0, 0.0, 250.0), unreal.Vector(0.35, 16.5, 5.0), wall_mat)
    create_or_reuse_static_mesh(f"{PREFIX}Wall_West_Left", ARENA_CENTER + unreal.Vector(-1125.0, 520.0, 250.0), unreal.Vector(0.35, 6.2, 5.0), wall_mat)
    create_or_reuse_static_mesh(f"{PREFIX}Wall_West_Right", ARENA_CENTER + unreal.Vector(-1125.0, -520.0, 250.0), unreal.Vector(0.35, 6.2, 5.0), wall_mat)

    create_or_reuse_static_mesh(f"{PREFIX}CorridorWall_North", unreal.Vector(3500.0, 250.0, 180.0), unreal.Vector(24.0, 0.25, 3.6), wall_mat)
    create_or_reuse_static_mesh(f"{PREFIX}CorridorWall_South", unreal.Vector(3500.0, -250.0, 180.0), unreal.Vector(24.0, 0.25, 3.6), wall_mat)

    entry_gate = create_or_reuse_static_mesh(ENTRY_GATE_LABEL, unreal.Vector(5060.0, 0.0, 360.0), unreal.Vector(0.25, 4.8, 0.45), gate_mat)
    set_actor_collision(entry_gate, False)

    exit_gate = create_or_reuse_static_mesh(EXIT_GATE_LABEL, unreal.Vector(7350.0, 0.0, 220.0), unreal.Vector(0.35, 5.8, 4.4), gate_mat)
    reward = create_or_reuse_static_mesh(REWARD_LABEL, unreal.Vector(7550.0, 0.0, 130.0), unreal.Vector(1.2, 1.2, 1.2), reward_mat)

    try:
        reward.set_actor_hidden_in_game(True)
        reward.set_actor_enable_collision(False)
    except Exception:
        pass

    player_start_class = unreal.PlayerStart.static_class()
    player_start = create_or_reuse_actor(
        PLAYER_START_LABEL,
        player_start_class,
        PLAYER_START_LOCATION,
        unreal.Rotator(0.0, 0.0, 0.0),
    )

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(actor, unreal.PlayerStart):
            actor.set_actor_location(PLAYER_START_LOCATION, False, False)
            actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)

    try:
        text_class = unreal.TextRenderActor.static_class()
        sign = create_or_reuse_actor(
            SIGN_LABEL,
            text_class,
            unreal.Vector(4350.0, 0.0, 220.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        text_component = get_component(sign, unreal.TextRenderComponent)
        if text_component:
            text_component.set_text("CMD FINAL BOSS")
            if hasattr(unreal, "HorizontalTextAligment"):
                text_component.set_editor_property("horizontal_alignment", unreal.HorizontalTextAligment.EHTA_CENTER)
            text_component.set_editor_property("world_size", 96.0)
    except Exception as exc:
        log(f"Sign setup skipped: {exc}")

    return entry_gate, exit_gate, reward, player_start


def main():
    load_map()

    arena_class = unreal.load_class(None, "/Script/Exception.BRBossArenaTrigger")
    if not arena_class:
        raise RuntimeError("BRBossArenaTrigger class not found.")

    cmd_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_CMD_PATH)
    if not cmd_class:
        raise RuntimeError(f"CMD blueprint class not found: {BP_CMD_PATH}. Run CreateCMDBossBlueprint.py first.")

    entry_gate, exit_gate, reward, player_start = build_demo_room()

    arena = create_or_reuse_actor(
        ARENA_LABEL,
        arena_class,
        TRIGGER_LOCATION,
        unreal.Rotator(0.0, 0.0, 0.0),
    )

    set_property_if_exists(arena, "BossClassToSpawn", cmd_class)
    set_property_if_exists(arena, "BossSpawnOffset", BOSS_OFFSET)
    set_property_if_exists(arena, "bSpawnBossOnArenaStart", True)
    set_property_if_exists(arena, "bResetBossOnEnter", True)
    set_property_if_exists(arena, "bAutoIncludeNearbyBosses", False)
    set_property_if_exists(arena, "bAutoIncludeTeamMembers", False)
    set_property_if_exists(arena, "bStartOnPlayerOverlap", True)
    set_property_if_exists(arena, "bDeactivateUnmanagedBossesOnStart", True)
    set_property_if_exists(arena, "bPlayBossIntroBeforeAI", True)
    set_property_if_exists(arena, "BossIntroDelay", 1.4)
    set_property_if_exists(arena, "GateActorToHideOnDefeat", exit_gate)
    set_property_if_exists(arena, "RewardActorToShowOnDefeat", reward)

    trigger_box = get_component(arena, unreal.BoxComponent)
    if trigger_box:
        trigger_box.set_box_extent(unreal.Vector(180.0, 360.0, 180.0))

    unreal.EditorLevelLibrary.set_selected_level_actors([player_start, arena])
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved map/packages.")


main()
