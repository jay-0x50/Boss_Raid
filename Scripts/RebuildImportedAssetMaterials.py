import unreal


ASSET_SETS = [
    ("/Game/World/Graves/PlayerGrave", "PlayerGrave"),
    ("/Game/World/Portals/BossFogGate", "BossFogGate"),
    ("/Game/World/Altars/HiddenWeaponAltar", "HiddenWeaponAltar"),
    ("/Game/World/Landmarks/SymbolTree", "SymbolTree"),
    ("/Game/World/Checkpoints/CheckpointBonfire", "CheckpointBonfire"),
    ("/Game/Items/Consumables/RuntimeFlask/Filled", "RuntimeFlask_Filled"),
    ("/Game/Items/Consumables/RuntimeFlask/Empty", "RuntimeFlask_Empty"),
    ("/Game/Items/KeyItems/NellHiddenMemoryFragment", "NellHiddenMemoryFragment"),
    ("/Game/Items/Weapons/Mimikatz/Left", "MimikatzAuthoritySeized_L"),
    ("/Game/Items/Weapons/Mimikatz/Right", "MimikatzAuthoritySeized_R"),
    ("/Game/Player/Hendel/Weapons/DefaultWeapon", "Hendel_DefaultWeapon"),
    ("/Game/Bosses/CMD/Props/Throne", "CMD_Throne"),
    ("/Game/Enemies/FieldMonsters/FieldMonster_01", "FieldMonster_01"),
]


def log(message):
    unreal.log(f"[RebuildImportedAssetMaterials] {message}")


def load_asset(path):
    return unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


def choose_texture(directory, code_name):
    preferred = [
        f"{directory}/T_{code_name}_Preview_A",
        f"{directory}/T_{code_name}_Emission",
        f"{directory}/T_{code_name}_Normal",
        f"{directory}/T_{code_name}_Roughness",
    ]
    for path in preferred:
        asset = load_asset(path)
        if isinstance(asset, unreal.Texture2D):
            return asset

    for path in unreal.EditorAssetLibrary.list_assets(directory, recursive=False, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Texture2D):
            return asset
    return None


def rebuild_material(directory, code_name, texture):
    material_name = f"M_{code_name}"
    material_path = f"{directory}/{material_name}"
    old_material = load_asset(material_path)
    if old_material:
        unreal.EditorAssetLibrary.delete_asset(material_path)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        material_name,
        directory,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError(f"Failed to create {material_path}")

    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        -320,
        0,
    )
    texture_sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_property(texture_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def assign_material_to_static_mesh(directory, code_name, material):
    mesh = load_asset(f"{directory}/SM_{code_name}")
    if not isinstance(mesh, unreal.StaticMesh):
        return

    try:
        mesh.set_material(0, material)
    except Exception as exc:
        log(f"Failed to set material on SM_{code_name}: {exc}")
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def main():
    for directory, code_name in ASSET_SETS:
        if not unreal.EditorAssetLibrary.does_directory_exist(directory):
            continue

        texture = choose_texture(directory, code_name)
        if not texture:
            log(f"Skipped {code_name}: no Texture2D found")
            continue

        material = rebuild_material(directory, code_name, texture)
        assign_material_to_static_mesh(directory, code_name, material)
        log(f"Rebuilt material and assigned mesh slot for {code_name}")

    unreal.EditorAssetLibrary.save_directory("/Game/World")
    unreal.EditorAssetLibrary.save_directory("/Game/Items")
    unreal.EditorAssetLibrary.save_directory("/Game/Bosses")
    unreal.EditorAssetLibrary.save_directory("/Game/Player")
    unreal.EditorAssetLibrary.save_directory("/Game/Enemies")


main()
