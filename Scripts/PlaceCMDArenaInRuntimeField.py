import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ARENA_LABEL = "BossPlate_3_CMDArena"
BP_CMD_PATH = "/Game/Blueprints/Bosses/BP_CMDBoss"


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


def main():
    load_map()

    arena_class = unreal.load_class(None, "/Script/Exception.BRBossArenaTrigger")
    if not arena_class:
        raise RuntimeError("BRBossArenaTrigger class not found.")

    cmd_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_CMD_PATH)
    if not cmd_class:
        raise RuntimeError(f"CMD blueprint class not found: {BP_CMD_PATH}. Run CreateCMDBossBlueprint.py first.")

    arena = find_actor_by_label(ARENA_LABEL)
    if not arena:
        arena = unreal.EditorLevelLibrary.spawn_actor_from_class(
            arena_class,
            unreal.Vector(5200.0, 0.0, 120.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        arena.set_actor_label(ARENA_LABEL)
        log(f"Spawned {ARENA_LABEL}")
    else:
        log(f"Reusing existing {ARENA_LABEL}")

    arena.set_actor_location(unreal.Vector(5200.0, 0.0, 120.0), False, False)
    arena.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)

    set_property_if_exists(arena, "BossClassToSpawn", cmd_class)
    set_property_if_exists(arena, "BossSpawnOffset", unreal.Vector(900.0, 0.0, 0.0))
    set_property_if_exists(arena, "bSpawnBossOnArenaStart", True)
    set_property_if_exists(arena, "bResetBossOnEnter", True)
    set_property_if_exists(arena, "bAutoIncludeNearbyBosses", False)
    set_property_if_exists(arena, "bAutoIncludeTeamMembers", False)
    set_property_if_exists(arena, "bStartOnPlayerOverlap", True)
    set_property_if_exists(arena, "bDeactivateUnmanagedBossesOnStart", True)
    set_property_if_exists(arena, "bPlayBossIntroBeforeAI", True)
    set_property_if_exists(arena, "BossIntroDelay", 1.4)

    unreal.EditorLevelLibrary.set_selected_level_actors([arena])
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved map/packages.")


main()
