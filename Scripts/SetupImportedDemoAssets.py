import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
GRAVE_SOURCE_DIR = "/Game/Materials/World/Graves/PlayerGrave"
GRAVE_TARGET_DIR = "/Game/World/Graves/PlayerGrave"
BP_DIR = "/Game/Blueprints/World"
PLAYER_BP_PATH = "/Game/Blueprints/Core/BP_ExceptionCharacter"


def log(message):
    unreal.log(f"[SetupImportedDemoAssets] {message}")


def rename_asset(old_path, new_path):
    if unreal.EditorAssetLibrary.does_asset_exist(new_path):
        if unreal.EditorAssetLibrary.does_asset_exist(old_path):
            log(f"Deleting duplicate imported asset because target already exists: {old_path}")
            unreal.EditorAssetLibrary.delete_asset(old_path)
        return unreal.EditorAssetLibrary.load_asset(new_path)

    if not unreal.EditorAssetLibrary.does_asset_exist(old_path):
        return None

    unreal.EditorAssetLibrary.make_directory("/".join(new_path.split("/")[:-1]))
    if unreal.EditorAssetLibrary.rename_asset(old_path, new_path):
        log(f"Renamed {old_path} -> {new_path}")
        return unreal.EditorAssetLibrary.load_asset(new_path)

    log(f"Failed to rename {old_path} -> {new_path}")
    return unreal.EditorAssetLibrary.load_asset(old_path)


def add_download_set(rename_map, source_dir, target_dir, code_name, base_texture_name):
    for import_dir in {source_dir, target_dir}:
        rename_map[f"{import_dir}/{base_texture_name}_texture"] = f"{target_dir}/SM_{code_name}"
        rename_map[f"{import_dir}/Material_001"] = f"{target_dir}/M_{code_name}"
        rename_map[f"{import_dir}/{base_texture_name}_texture_emission"] = f"{target_dir}/T_{code_name}_Emission"
        rename_map[f"{import_dir}/{base_texture_name}_texture_metallic"] = f"{target_dir}/T_{code_name}_Metallic"
        rename_map[f"{import_dir}/{base_texture_name}_texture_normal"] = f"{target_dir}/T_{code_name}_Normal"
        rename_map[f"{import_dir}/{base_texture_name}_texture_roughness"] = f"{target_dir}/T_{code_name}_Roughness"
        rename_map[f"{import_dir}/Image_0"] = f"{target_dir}/T_{code_name}_Preview_A"
        rename_map[f"{import_dir}/Image_2"] = f"{target_dir}/T_{code_name}_Preview_B"
        rename_map[f"{import_dir}/Image_3"] = f"{target_dir}/T_{code_name}_Preview_C"


def rename_imported_assets():
    rename_map = {}
    add_download_set(
        rename_map,
        GRAVE_SOURCE_DIR,
        GRAVE_TARGET_DIR,
        "PlayerGrave",
        "Meshy_AI_Hendel_exe_Terminal__0626103736",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/World/Portals/BossFogGate",
        "/Game/World/Portals/BossFogGate",
        "BossFogGate",
        "Meshy_AI_Syntax_Gate_0624124801",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/World/Altars/MimikatzAltar",
        "/Game/World/Altars/HiddenWeaponAltar",
        "HiddenWeaponAltar",
        "Meshy_AI_Crossed_Neon_Swords_0626104931",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Items/Weapons/Mimikatz/Left",
        "/Game/Items/Weapons/Mimikatz/Left",
        "MimikatzAuthoritySeized_L",
        "Meshy_AI_Override_Root_Authori_0626093242",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Items/Weapons/Mimikatz/Right",
        "/Game/Items/Weapons/Mimikatz/Right",
        "MimikatzAuthoritySeized_R",
        "Meshy_AI_Override_Root_Authori_0626093228",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Items/KeyItems/NellHiddenMemoryFragment",
        "/Game/Items/KeyItems/NellHiddenMemoryFragment",
        "NellHiddenMemoryFragment",
        "Meshy_AI_Fragmented_Memory_0626103722",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Enemies/FieldMonsters/FieldMonster_01",
        "/Game/Enemies/FieldMonsters/FieldMonster_01",
        "FieldMonster_01",
        "Meshy_AI_Memory_Leak_0626103713",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Items/Consumables/RuntimeFlask/Filled",
        "/Game/Items/Consumables/RuntimeFlask/Filled",
        "RuntimeFlask_Filled",
        "Meshy_AI_Verdant_Aether_Vessel_0626110724",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Items/Consumables/RuntimeFlask/Empty",
        "/Game/Items/Consumables/RuntimeFlask/Empty",
        "RuntimeFlask_Empty",
        "Meshy_AI_Aetherbound_Elixir_Ve_0626110922",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/World/Landmarks/SymbolTree",
        "/Game/World/Landmarks/SymbolTree",
        "SymbolTree",
        "Meshy_AI_Neon_Code_Tree_0626111301",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/World/Checkpoints/CheckpointBonfire",
        "/Game/World/Checkpoints/CheckpointBonfire",
        "CheckpointBonfire",
        "Meshy_AI_Emerald_Nexus_Portal_0626113025",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Bosses/CMD/Props/Throne",
        "/Game/Bosses/CMD/Props/Throne",
        "CMD_Throne",
        "Meshy_AI_The_First_Command_0626112504",
    )
    add_download_set(
        rename_map,
        "/Game/Materials/Player/Hendel/Weapons/DefaultWeapon",
        "/Game/Player/Hendel/Weapons/DefaultWeapon",
        "Hendel_DefaultWeapon",
        "Meshy_AI_Codeblade_0626112358",
    )

    renamed_assets = {}
    for old_path, new_path in rename_map.items():
        asset = rename_asset(old_path, new_path)
        if asset:
            renamed_assets[new_path] = asset

    return renamed_assets


def create_or_load_blueprint(bp_name, parent_class_path):
    bp_path = f"{BP_DIR}/{bp_name}"
    existing = unreal.EditorAssetLibrary.load_asset(bp_path) if unreal.EditorAssetLibrary.does_asset_exist(bp_path) else None
    if existing:
        return existing, unreal.EditorAssetLibrary.load_blueprint_class(bp_path)

    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(f"Missing parent class: {parent_class_path}")

    unreal.EditorAssetLibrary.make_directory(BP_DIR)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(bp_name, BP_DIR, unreal.Blueprint, factory)
    if not blueprint:
        raise RuntimeError(f"Failed to create blueprint: {bp_path}")

    log(f"Created blueprint: {bp_path}")
    return blueprint, unreal.EditorAssetLibrary.load_blueprint_class(bp_path)


def get_component(obj, component_class):
    try:
        component = obj.get_component_by_class(component_class)
        if component:
            return component
    except Exception:
        pass

    try:
        components = obj.get_components_by_class(component_class)
        if components:
            return components[0]
    except Exception:
        pass

    return None


def configure_blueprints(renamed_assets):
    grave_bp, grave_class = create_or_load_blueprint("BP_PlayerGraveMarker", "/Script/Exception.BRPlayerGraveMarker")
    altar_bp, altar_class = create_or_load_blueprint("BP_HiddenWeaponAltar", "/Script/Exception.BRHiddenWeaponAltar")
    checkpoint_bp, checkpoint_class = create_or_load_blueprint("BP_CheckpointBonfire", "/Script/Exception.BRCheckpoint")

    grave_material = renamed_assets.get(f"{GRAVE_TARGET_DIR}/M_PlayerGrave") or unreal.EditorAssetLibrary.load_asset(f"{GRAVE_TARGET_DIR}/M_PlayerGrave")
    altar_material = renamed_assets.get("/Game/World/Altars/HiddenWeaponAltar/M_HiddenWeaponAltar") or unreal.EditorAssetLibrary.load_asset("/Game/World/Altars/HiddenWeaponAltar/M_HiddenWeaponAltar")
    checkpoint_material = unreal.EditorAssetLibrary.load_asset("/Game/World/Checkpoints/CheckpointBonfire/M_CheckpointBonfire")
    grave_mesh = unreal.EditorAssetLibrary.load_asset(f"{GRAVE_TARGET_DIR}/SM_PlayerGrave")
    altar_mesh = unreal.EditorAssetLibrary.load_asset("/Game/World/Altars/HiddenWeaponAltar/SM_HiddenWeaponAltar")
    checkpoint_mesh = unreal.EditorAssetLibrary.load_asset("/Game/World/Checkpoints/CheckpointBonfire/SM_CheckpointBonfire")

    for blueprint, bp_class, component_name, mesh_asset, material in [
        (grave_bp, grave_class, "GraveMeshComponent", grave_mesh, grave_material),
        (altar_bp, altar_class, "AltarMeshComponent", altar_mesh, altar_material),
        (checkpoint_bp, checkpoint_class, "MeshComponent", checkpoint_mesh, checkpoint_material),
    ]:
        if not bp_class:
            continue

        cdo = unreal.get_default_object(bp_class)
        component = get_component(cdo, unreal.StaticMeshComponent)
        if component:
            cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
            if mesh_asset and isinstance(mesh_asset, unreal.StaticMesh):
                component.set_static_mesh(mesh_asset)
            elif cube:
                component.set_static_mesh(cube)
            if material and isinstance(material, unreal.MaterialInterface):
                component.set_material(0, material)
            if component_name == "GraveMeshComponent":
                component.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
            else:
                component.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))

        unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    player_bp = unreal.EditorAssetLibrary.load_asset(PLAYER_BP_PATH)
    player_class = unreal.EditorAssetLibrary.load_blueprint_class(PLAYER_BP_PATH)
    if player_bp and player_class and grave_class:
        player_cdo = unreal.get_default_object(player_class)
        try:
            player_cdo.set_editor_property("PlayerGraveClass", grave_class)
            unreal.EditorAssetLibrary.save_loaded_asset(player_bp)
            log("Assigned BP_PlayerGraveMarker to BP_ExceptionCharacter.PlayerGraveClass")
        except Exception as exc:
            log(f"Skipped PlayerGraveClass assignment: {exc}")

    return grave_class, altar_class, checkpoint_class


def load_map():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def place_hidden_weapon_altar(altar_class):
    if not altar_class:
        return

    load_map()
    label = "Demo_Field_HiddenWeaponAltar"
    actor = find_actor(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            altar_class,
            unreal.Vector(2550.0, -820.0, 115.0),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=35.0),
        )
        actor.set_actor_label(label)
        log(f"Spawned {label}")
    else:
        actor.set_actor_location(unreal.Vector(2550.0, -820.0, 115.0), False, False)
        actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=35.0), False)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved hidden weapon altar placement.")


def place_checkpoint_bonfire(checkpoint_class):
    if not checkpoint_class:
        return

    load_map()
    legacy_checkpoint = find_actor("BRCheckpoint")
    if legacy_checkpoint:
        legacy_checkpoint.set_actor_location(unreal.Vector(770.0, 260.0, -5000.0), False, False)
        legacy_checkpoint.set_actor_hidden_in_game(True)
        legacy_checkpoint.set_actor_enable_collision(False)

    static_visual = find_actor("Demo_Field_CheckpointBonfire")
    if static_visual:
        unreal.EditorLevelLibrary.destroy_actor(static_visual)

    label = "Demo_Field_CheckpointBonfire"
    actor = find_actor(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            checkpoint_class,
            unreal.Vector(2180.0, 520.0, 150.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        actor.set_actor_label(label)
        log(f"Spawned {label}")
    else:
        actor.set_actor_location(unreal.Vector(2180.0, 520.0, 150.0), False, False)
        actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)

    actor.set_actor_scale3d(unreal.Vector(1.3, 1.3, 1.3))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved checkpoint bonfire placement.")


def spawn_or_move_static(label, location, scale, material_path, collision=False, mesh_path=None, rotation=None):
    actor = find_actor(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor.static_class(), location)
        actor.set_actor_label(label)
        log(f"Spawned {label}")
    actor.set_actor_location(location, False, False)
    if rotation:
        actor.set_actor_rotation(rotation, False)
    actor.set_actor_scale3d(scale)
    actor.set_actor_enable_collision(collision)

    component = get_component(actor, unreal.StaticMeshComponent)
    if component:
        cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path) if mesh_path else None
        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if mesh and isinstance(mesh, unreal.StaticMesh):
            component.set_static_mesh(mesh)
        elif cube:
            component.set_static_mesh(cube)
        if material and isinstance(material, unreal.MaterialInterface):
            component.set_material(0, material)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)

    return actor


def place_demo_visuals():
    load_map()
    fog_material = "/Game/World/Portals/BossFogGate/M_BossFogGate"
    for name, location in [
        ("Demo_Field_FogGate_Vritra", unreal.Vector(1850.0, -5200.0, 330.0)),
        ("Demo_Field_FogGate_Python", unreal.Vector(1850.0, 5200.0, 330.0)),
        ("Demo_Field_FogGate_Selvara", unreal.Vector(7050.0, 5200.0, 330.0)),
        ("Demo_Field_FogGate_CMD", unreal.Vector(12350.0, 0.0, 330.0)),
    ]:
        spawn_or_move_static(
            name,
            location,
            unreal.Vector(1.8, 1.8, 1.8),
            fog_material,
            collision=False,
            mesh_path="/Game/World/Portals/BossFogGate/SM_BossFogGate",
            rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0),
        )

    spawn_or_move_static(
        "Demo_Field_NellHiddenMemoryFragment",
        unreal.Vector(2420.0, -650.0, 175.0),
        unreal.Vector(0.85, 0.85, 0.85),
        "/Game/Items/KeyItems/NellHiddenMemoryFragment/M_NellHiddenMemoryFragment",
        collision=False,
        mesh_path="/Game/Items/KeyItems/NellHiddenMemoryFragment/SM_NellHiddenMemoryFragment",
    )
    spawn_or_move_static(
        "Demo_Field_MimikatzAuthoritySeized_L",
        unreal.Vector(2550.0, -780.0, 205.0),
        unreal.Vector(0.85, 0.85, 0.85),
        "/Game/Items/Weapons/Mimikatz/Left/M_MimikatzAuthoritySeized_L",
        collision=False,
        mesh_path="/Game/Items/Weapons/Mimikatz/Left/SM_MimikatzAuthoritySeized_L",
        rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=35.0),
    )
    spawn_or_move_static(
        "Demo_Field_MimikatzAuthoritySeized_R",
        unreal.Vector(2630.0, -845.0, 205.0),
        unreal.Vector(0.85, 0.85, 0.85),
        "/Game/Items/Weapons/Mimikatz/Right/M_MimikatzAuthoritySeized_R",
        collision=False,
        mesh_path="/Game/Items/Weapons/Mimikatz/Right/SM_MimikatzAuthoritySeized_R",
        rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=-35.0),
    )
    spawn_or_move_static(
        "Demo_Field_FieldMonster_01_Visual",
        unreal.Vector(3300.0, 440.0, 145.0),
        unreal.Vector(1.0, 1.0, 1.0),
        "/Game/Enemies/FieldMonsters/FieldMonster_01/M_FieldMonster_01",
        collision=False,
        mesh_path="/Game/Enemies/FieldMonsters/FieldMonster_01/SM_FieldMonster_01",
    )
    spawn_or_move_static(
        "Demo_Field_RuntimeFlask_Filled",
        unreal.Vector(2140.0, -360.0, 155.0),
        unreal.Vector(0.75, 0.75, 0.75),
        "/Game/Items/Consumables/RuntimeFlask/Filled/M_RuntimeFlask_Filled",
        collision=False,
        mesh_path="/Game/Items/Consumables/RuntimeFlask/Filled/SM_RuntimeFlask_Filled",
    )
    spawn_or_move_static(
        "Demo_Field_RuntimeFlask_Empty",
        unreal.Vector(2220.0, -360.0, 155.0),
        unreal.Vector(0.75, 0.75, 0.75),
        "/Game/Items/Consumables/RuntimeFlask/Empty/M_RuntimeFlask_Empty",
        collision=False,
        mesh_path="/Game/Items/Consumables/RuntimeFlask/Empty/SM_RuntimeFlask_Empty",
    )
    spawn_or_move_static(
        "Demo_Field_SymbolTree",
        unreal.Vector(6350.0, 0.0, 120.0),
        unreal.Vector(5.0, 5.0, 5.0),
        "/Game/World/Landmarks/SymbolTree/M_SymbolTree",
        collision=False,
        mesh_path="/Game/World/Landmarks/SymbolTree/SM_SymbolTree",
    )
    spawn_or_move_static(
        "Demo_Field_HendelDefaultWeapon",
        unreal.Vector(1810.0, -420.0, 170.0),
        unreal.Vector(0.9, 0.9, 0.9),
        "/Game/Player/Hendel/Weapons/DefaultWeapon/M_Hendel_DefaultWeapon",
        collision=False,
        mesh_path="/Game/Player/Hendel/Weapons/DefaultWeapon/SM_Hendel_DefaultWeapon",
        rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=20.0),
    )
    spawn_or_move_static(
        "Demo_CMD_Throne",
        unreal.Vector(15520.0, 0.0, 105.0),
        unreal.Vector(2.3, 2.3, 2.3),
        "/Game/Bosses/CMD/Props/Throne/M_CMD_Throne",
        collision=False,
        mesh_path="/Game/Bosses/CMD/Props/Throne/SM_CMD_Throne",
        rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
    )

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved demo visual placements.")


def main():
    renamed_assets = rename_imported_assets()
    _, altar_class, checkpoint_class = configure_blueprints(renamed_assets)
    place_hidden_weapon_altar(altar_class)
    place_checkpoint_bonfire(checkpoint_class)
    place_demo_visuals()
    unreal.EditorAssetLibrary.save_directory(GRAVE_TARGET_DIR)
    unreal.EditorAssetLibrary.save_directory("/Game/World")
    unreal.EditorAssetLibrary.save_directory("/Game/Items")
    unreal.EditorAssetLibrary.save_directory("/Game/Enemies")
    unreal.EditorAssetLibrary.save_directory("/Game/Bosses")
    unreal.EditorAssetLibrary.save_directory("/Game/Player")
    unreal.EditorAssetLibrary.save_directory(BP_DIR)


main()
