import unreal


BP_DIR = "/Game/Blueprints/Bosses"
BP_NAME = "BP_BossPlate2_VritraArena"
BP_PATH = f"{BP_DIR}/{BP_NAME}"
BP_VRITRA_PATH = "/Game/Blueprints/Bosses/BP_VritraBoss"


def log(message):
    unreal.log(f"[CreateVritraArenaPlateBlueprint] {message}")


def set_property_if_exists(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
        log(f"Set {prop_name} = {value}")
        return True
    except Exception as exc:
        log(f"Skipped {prop_name}: {exc}")
        return False


def create_or_load_blueprint():
    parent_class = unreal.load_class(None, "/Script/Exception.BRBossArenaTrigger")
    if not parent_class:
        raise RuntimeError("BRBossArenaTrigger class not found.")

    existing = unreal.EditorAssetLibrary.load_asset(BP_PATH) if unreal.EditorAssetLibrary.does_asset_exist(BP_PATH) else None
    if existing:
        log(f"Using existing blueprint: {BP_PATH}")
        return existing

    unreal.EditorAssetLibrary.make_directory(BP_DIR)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    blueprint = asset_tools.create_asset(BP_NAME, BP_DIR, unreal.Blueprint, factory)
    if not blueprint:
        raise RuntimeError(f"Failed to create blueprint: {BP_PATH}")

    log(f"Created blueprint: {BP_PATH}")
    return blueprint


def get_cdo(blueprint):
    try:
        generated_class = blueprint.generated_class()
    except Exception:
        generated_class = None

    if not generated_class:
        generated_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)

    if not generated_class:
        raise RuntimeError("Blueprint GeneratedClass is missing after compile.")

    return unreal.get_default_object(generated_class)


def main():
    vritra_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_VRITRA_PATH)
    if not vritra_class:
        raise RuntimeError(f"Vritra blueprint class not found: {BP_VRITRA_PATH}")

    blueprint = create_or_load_blueprint()
    cdo = get_cdo(blueprint)

    set_property_if_exists(cdo, "BossClassToSpawn", vritra_class)
    set_property_if_exists(cdo, "BossSpawnOffset", unreal.Vector(700.0, 0.0, 0.0))
    set_property_if_exists(cdo, "bSpawnBossOnArenaStart", True)
    set_property_if_exists(cdo, "bResetBossOnEnter", True)
    set_property_if_exists(cdo, "bAutoIncludeNearbyBosses", False)
    set_property_if_exists(cdo, "bAutoIncludeTeamMembers", False)
    set_property_if_exists(cdo, "bStartOnPlayerOverlap", True)
    set_property_if_exists(cdo, "bDeactivateUnmanagedBossesOnStart", True)

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    log(f"Saved blueprint: {BP_PATH}")


main()
