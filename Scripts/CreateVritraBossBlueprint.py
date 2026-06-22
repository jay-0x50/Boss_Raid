import unreal


BP_DIR = "/Game/Blueprints/Bosses"
BP_NAME = "BP_VritraBoss"
BP_PATH = f"{BP_DIR}/{BP_NAME}"

MESH_PATH = "/Game/Bosses/Perl/Perl_model_Animation_Walking_withSkin"
ANIM_PATH = "/Game/Bosses/Perl/Perl_model_Animation_Walking_withSkin_Anim"
MATERIAL_PATH = "/Game/Bosses/Perl/Perl"


def log(message):
    unreal.log(f"[CreateVritraBossBlueprint] {message}")


def load_required_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Asset not found: {path}")
    log(f"Loaded {path}: {asset.get_class().get_name()}")
    return asset


def get_parent_class():
    parent = unreal.load_class(None, "/Script/Exception.BRVritraBoss")
    if parent:
        return parent

    # Fallback keeps the asset creatable if the C++ class was not compiled yet.
    parent = unreal.load_class(None, "/Script/Exception.BRPatternBossBase")
    if parent:
        log("BRVritraBoss class not found. Falling back to BRPatternBossBase.")
        return parent

    raise RuntimeError("Could not find BRVritraBoss or BRPatternBossBase.")


def create_or_load_blueprint(parent_class):
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


def set_property_if_exists(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
        log(f"Set {prop_name} = {value}")
        return True
    except Exception as exc:
        log(f"Skipped {prop_name}: {exc}")
        return False


def enum_value(enum_class, *candidate_names):
    for name in candidate_names:
        if hasattr(enum_class, name):
            return getattr(enum_class, name)
    raise RuntimeError(f"Missing enum value on {enum_class}: {candidate_names}")


def make_pattern(name, pattern_type, min_range, max_range, damage, windup, cooldown, radius, forward_offset=0.0, dash_distance=0.0, phase1=True, phase2=True):
    pattern = unreal.BRBossPatternData()
    pattern.set_editor_property("PatternName", name)
    pattern.set_editor_property("PatternType", pattern_type)
    pattern.set_editor_property("MinRange", min_range)
    pattern.set_editor_property("MaxRange", max_range)
    pattern.set_editor_property("Damage", damage)
    pattern.set_editor_property("Windup", windup)
    pattern.set_editor_property("Cooldown", cooldown)
    pattern.set_editor_property("Radius", radius)
    pattern.set_editor_property("ForwardOffset", forward_offset)
    pattern.set_editor_property("DashDistance", dash_distance)
    pattern.set_editor_property("bEnableInPhase1", phase1)
    pattern.set_editor_property("bEnableInPhase2", phase2)
    return pattern


def configure_vritra_patterns(cdo):
    try:
        melee = enum_value(unreal.BRBossPatternType, "MELEE", "Melee")
        dash = enum_value(unreal.BRBossPatternType, "DASH", "Dash")
        aoe = enum_value(unreal.BRBossPatternType, "AOE")

        patterns = [
            make_pattern("Sigil_HumpCrash", aoe, 0.0, 560.0, 30.0, 0.9, 2.4, 300.0, phase1=True, phase2=True),
            make_pattern("Regex_SpitLine", melee, 560.0, 1550.0, 24.0, 1.0, 2.8, 140.0, forward_offset=1150.0, phase1=True, phase2=True),
            make_pattern("Caravan_Rush", dash, 420.0, 1150.0, 36.0, 1.1, 3.8, 165.0, forward_offset=190.0, dash_distance=620.0, phase1=True, phase2=True),
            make_pattern("Hash_Sandstorm", aoe, 260.0, 900.0, 28.0, 1.25, 4.2, 420.0, phase1=True, phase2=True),
            make_pattern("Backtracking_Stomp", aoe, 0.0, 760.0, 40.0, 1.35, 4.6, 500.0, phase1=False, phase2=True),
            make_pattern("OneLiner_Pierce", dash, 700.0, 1700.0, 44.0, 1.2, 5.0, 135.0, forward_offset=200.0, dash_distance=820.0, phase1=False, phase2=True),
        ]
        set_property_if_exists(cdo, "AttackPatterns", patterns)
    except Exception as exc:
        log(f"Skipped Vritra pattern table: {exc}")


def find_component(cdo, component_class, preferred_name):
    try:
        component = cdo.get_editor_property(preferred_name)
        if component:
            return component
    except Exception:
        pass

    try:
        components = cdo.get_components_by_class(component_class)
        if components:
            return components[0]
    except Exception:
        pass

    return None


def configure_blueprint(blueprint):
    skeletal_mesh = load_required_asset(MESH_PATH)
    anim_asset = unreal.EditorAssetLibrary.load_asset(ANIM_PATH)
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)

    try:
        generated_class = blueprint.generated_class()
    except Exception:
        generated_class = None

    if not generated_class:
        generated_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)

    if not generated_class:
        raise RuntimeError("Blueprint GeneratedClass is missing after compile.")

    cdo = unreal.get_default_object(generated_class)

    # Boss base visual settings.
    set_property_if_exists(cdo, "VisualMeshType", unreal.BRBossVisualMeshType.SKELETAL_MESH)
    set_property_if_exists(cdo, "MeshRelativeLocation", unreal.Vector(0.0, 0.0, -90.0))
    set_property_if_exists(cdo, "MeshRelativeRotation", unreal.Rotator(0.0, -90.0, 0.0))
    set_property_if_exists(cdo, "MeshRelativeScale", unreal.Vector(100.0, 100.0, 100.0))
    set_property_if_exists(cdo, "BossCollisionRadius", 95.0)
    set_property_if_exists(cdo, "BossCollisionHalfHeight", 130.0)

    # Keep Vritra gameplay values on the blueprint CDO too, so placed/spawned BP instances are ready.
    set_property_if_exists(cdo, "InitialMaxHP", 560.0)
    set_property_if_exists(cdo, "InitialMaxGroggy", 140.0)
    set_property_if_exists(cdo, "GroggyDuration", 3.4)
    set_property_if_exists(cdo, "Phase2StartHPRatio", 0.5)
    set_property_if_exists(cdo, "bCombatAIEnabled", False)
    set_property_if_exists(cdo, "DetectionRange", 2300.0)
    set_property_if_exists(cdo, "MoveSpeed", 165.0)
    set_property_if_exists(cdo, "Phase2MoveSpeedMultiplier", 1.22)
    set_property_if_exists(cdo, "Phase2CooldownMultiplier", 0.7)
    set_property_if_exists(cdo, "RotationInterpSpeed", 2.8)
    set_property_if_exists(cdo, "MeleeStandbyDistance", 260.0)
    set_property_if_exists(cdo, "RangedStandbyDistance", 1020.0)
    set_property_if_exists(cdo, "RangedComfortMinDistance", 620.0)
    configure_vritra_patterns(cdo)

    skeletal_component = find_component(cdo, unreal.SkeletalMeshComponent, "SkeletalMeshComponent")
    if not skeletal_component:
        raise RuntimeError("Could not find SkeletalMeshComponent on blueprint CDO.")

    skeletal_component.set_skeletal_mesh(skeletal_mesh)
    skeletal_component.set_editor_property("hidden_in_game", False)
    skeletal_component.set_editor_property("visible", True)

    if anim_asset:
        skeletal_component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        skeletal_component.set_animation(anim_asset)
        skeletal_component.play(True)
        log(f"Applied animation: {ANIM_PATH}")

    if material and isinstance(material, unreal.MaterialInterface):
        skeletal_component.set_material(0, material)
        log(f"Applied material slot 0: {MATERIAL_PATH}")
    elif material:
        log(f"Material asset exists but is not a MaterialInterface: {material.get_class().get_name()}")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    log(f"Saved blueprint: {BP_PATH}")


def main():
    parent_class = get_parent_class()
    blueprint = create_or_load_blueprint(parent_class)
    configure_blueprint(blueprint)


main()
