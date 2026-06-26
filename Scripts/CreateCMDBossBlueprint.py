import unreal


BP_DIR = "/Game/Blueprints/Bosses"
BP_NAME = "BP_CMDBoss"
BP_PATH = f"{BP_DIR}/{BP_NAME}"
ASSET_DIR = "/Game/Bosses/CMD"


def log(message):
    unreal.log(f"[CreateCMDBossBlueprint] {message}")


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


def find_first_asset(asset_class_names, preferred_keywords=None):
    preferred_keywords = preferred_keywords or []
    if not unreal.EditorAssetLibrary.does_directory_exist(ASSET_DIR):
        raise RuntimeError(f"CMD asset directory not found: {ASSET_DIR}")

    candidates = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(ASSET_DIR, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            continue

        class_name = asset.get_class().get_name()
        if class_name in asset_class_names:
            candidates.append((asset, asset_path, class_name))

    for keyword in preferred_keywords:
        lowered_keyword = keyword.lower()
        for asset, asset_path, class_name in candidates:
            if lowered_keyword in asset_path.lower():
                log(f"Found preferred {class_name}: {asset_path}")
                return asset, asset_path

    if candidates:
        asset, asset_path, class_name = candidates[0]
        log(f"Found {class_name}: {asset_path}")
        return asset, asset_path

    return None, ""


def find_anim(preferred_keywords):
    anim_asset, _ = find_first_asset({"AnimSequence", "AnimationAsset"}, preferred_keywords)
    return anim_asset


def map_key_name(value):
    try:
        return unreal.Name(value)
    except Exception:
        return value


def get_parent_class():
    parent = unreal.load_class(None, "/Script/Exception.BRCMDBoss")
    if parent:
        return parent

    parent = unreal.load_class(None, "/Script/Exception.BRPatternBossBase")
    if parent:
        log("BRCMDBoss class not found. Falling back to BRPatternBossBase.")
        return parent

    raise RuntimeError("Could not find BRCMDBoss or BRPatternBossBase.")


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


def make_pattern(name, pattern_type, min_range, max_range, damage, windup, cooldown, radius, forward_offset=0.0, dash_distance=0.0, phase1=True, phase2=True):
    pattern = unreal.BRBossPatternData()
    pattern.set_editor_property("PatternName", name)
    set_property_if_exists(pattern, "AnimationActionName", name)
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


def configure_cmd_patterns(cdo):
    try:
        melee = enum_value(unreal.BRBossPatternType, "MELEE", "Melee")
        dash = enum_value(unreal.BRBossPatternType, "DASH", "Dash")
        aoe = enum_value(unreal.BRBossPatternType, "AOE")

        patterns = [
            make_pattern("DIR_Sweep", melee, 0.0, 620.0, 34.0, 0.75, 2.1, 210.0, forward_offset=360.0),
            make_pattern("PING_Flood", aoe, 250.0, 1100.0, 30.0, 1.05, 3.1, 460.0),
            make_pattern("TASKKILL_Charge", dash, 520.0, 1450.0, 42.0, 1.1, 4.0, 170.0, forward_offset=240.0, dash_distance=760.0),
            make_pattern("ROOT_PromptCrash", aoe, 0.0, 760.0, 46.0, 1.3, 4.6, 560.0),
            make_pattern("FORMAT_RuntimeZone", aoe, 420.0, 1700.0, 52.0, 1.55, 5.4, 720.0, phase1=False, phase2=True),
            make_pattern("AUTHORITY_Seize", dash, 820.0, 2200.0, 60.0, 1.35, 6.0, 190.0, forward_offset=260.0, dash_distance=1050.0, phase1=False, phase2=True),
        ]
        set_property_if_exists(cdo, "AttackPatterns", patterns)
    except Exception as exc:
        log(f"Skipped CMD pattern table: {exc}")


def configure_cmd_animations(cdo, default_anim):
    try:
        walking_anim = find_anim(["Walking_withSkin_Anim", default_anim.get_name() if default_anim else "_Anim"]) or default_anim
        running_anim = find_anim(["Running_withSkin_Anim", "Running"])
        hook_anim = find_anim(["Left_Hook_from_Guard_withSkin_Anim", "Left_Hook"])
        cast_anim = find_anim(["mage_soell_cast_3_withSkin_Anim", "mage", "spell", "soell"])

        idle = enum_value(unreal.BRBossAnimationStage, "IDLE", "Idle")
        intro = enum_value(unreal.BRBossAnimationStage, "INTRO", "Intro")
        move = enum_value(unreal.BRBossAnimationStage, "MOVE", "Move")
        windup = enum_value(unreal.BRBossAnimationStage, "PATTERN_WINDUP", "PatternWindup")
        impact = enum_value(unreal.BRBossAnimationStage, "PATTERN_IMPACT", "PatternImpact")
        recovery = enum_value(unreal.BRBossAnimationStage, "PATTERN_RECOVERY", "PatternRecovery")
        groggy = enum_value(unreal.BRBossAnimationStage, "GROGGY", "Groggy")
        death = enum_value(unreal.BRBossAnimationStage, "DEATH", "Death")

        stage_animations = {
            idle: walking_anim,
            intro: cast_anim or walking_anim,
            move: running_anim or walking_anim,
            windup: cast_anim or hook_anim or walking_anim,
            impact: hook_anim or cast_anim or walking_anim,
            recovery: walking_anim,
            groggy: walking_anim,
            death: walking_anim,
        }
        stage_animations = {stage: animation for stage, animation in stage_animations.items() if animation}
        set_property_if_exists(cdo, "StageAnimations", stage_animations)

        action_animations = {
            map_key_name("DIR_Sweep"): hook_anim or walking_anim,
            map_key_name("PING_Flood"): cast_anim or walking_anim,
            map_key_name("TASKKILL_Charge"): running_anim or walking_anim,
            map_key_name("ROOT_PromptCrash"): cast_anim or walking_anim,
            map_key_name("FORMAT_RuntimeZone"): cast_anim or walking_anim,
            map_key_name("AUTHORITY_Seize"): running_anim or cast_anim or walking_anim,
        }
        action_animations = {action: animation for action, animation in action_animations.items() if animation}
        set_property_if_exists(cdo, "ActionAnimations", action_animations)
    except Exception as exc:
        log(f"Skipped CMD animation maps: {exc}")


def configure_blueprint(blueprint):
    skeletal_mesh, mesh_path = find_first_asset({"SkeletalMesh"}, ["Walking_withSkin", "Running_withSkin", "withSkin"])
    if not skeletal_mesh:
        raise RuntimeError(f"No SkeletalMesh found under {ASSET_DIR}. Put CMD boss mesh assets there first.")

    anim_asset, anim_path = find_first_asset({"AnimSequence", "AnimationAsset"}, ["Walking_withSkin_Anim", "Running_withSkin_Anim", "_Anim"])
    material, material_path = find_first_asset({"Material", "MaterialInstanceConstant", "MaterialInstance"}, ["Material_1", "CMD"])

    cdo = get_cdo(blueprint)

    set_property_if_exists(cdo, "VisualMeshType", unreal.BRBossVisualMeshType.SKELETAL_MESH)
    set_property_if_exists(cdo, "MeshRelativeLocation", unreal.Vector(0.0, 0.0, -90.0))
    set_property_if_exists(cdo, "MeshRelativeRotation", unreal.Rotator(0.0, 0.0, 0.0))
    set_property_if_exists(cdo, "MeshRelativeScale", unreal.Vector(1.0, 1.0, 1.0))
    set_property_if_exists(cdo, "GroundTraceActorHalfHeight", 170.0)

    set_property_if_exists(cdo, "InitialMaxHP", 900.0)
    set_property_if_exists(cdo, "InitialMaxGroggy", 210.0)
    set_property_if_exists(cdo, "GroggyDuration", 3.0)
    set_property_if_exists(cdo, "Phase2StartHPRatio", 0.45)
    set_property_if_exists(cdo, "bCombatAIEnabled", False)
    set_property_if_exists(cdo, "DetectionRange", 2800.0)
    set_property_if_exists(cdo, "MoveSpeed", 260.0)
    set_property_if_exists(cdo, "Phase2MoveSpeedMultiplier", 1.18)
    set_property_if_exists(cdo, "Phase2CooldownMultiplier", 0.62)
    set_property_if_exists(cdo, "RotationInterpSpeed", 8.0)
    set_property_if_exists(cdo, "MeleeStandbyDistance", 420.0)
    set_property_if_exists(cdo, "RangedStandbyDistance", 1200.0)
    set_property_if_exists(cdo, "RangedComfortMinDistance", 700.0)
    configure_cmd_patterns(cdo)
    configure_cmd_animations(cdo, anim_asset)

    skeletal_component = find_component(cdo, unreal.SkeletalMeshComponent, "SkeletalMeshComponent")
    if not skeletal_component:
        raise RuntimeError("Could not find SkeletalMeshComponent on blueprint CDO.")

    skeletal_component.set_skeletal_mesh(skeletal_mesh)
    skeletal_component.set_editor_property("hidden_in_game", False)
    skeletal_component.set_editor_property("visible", True)
    log(f"Applied mesh: {mesh_path}")

    if anim_asset:
        skeletal_component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        skeletal_component.set_animation(anim_asset)
        skeletal_component.stop()
        log(f"Applied animation: {anim_path}")

    if material and isinstance(material, unreal.MaterialInterface):
        skeletal_component.set_material(0, material)
        log(f"Applied material slot 0: {material_path}")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    log(f"Saved blueprint: {BP_PATH}")


def main():
    parent_class = get_parent_class()
    blueprint = create_or_load_blueprint(parent_class)
    configure_blueprint(blueprint)


main()
