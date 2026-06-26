import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"


ENV_SETS = {
    "Floor_Field_Default": {
        "dirs": ["/Game/World/Environment/Floors/Field_Default"],
        "actor_labels": [
            "Demo_Field_MainPath_A",
            "Demo_Field_MainPath_B",
            "Demo_Field_MainPath_C",
            "Demo_Field_MainPath_D",
        ],
    },
    "Floor_Boss_Python": {
        "dirs": ["/Game/World/Environment/Floors/Boss_Python"],
        "actor_labels": ["Demo_Field_2_PythonArena_Floor", "Demo_Field_3_SelvaraArena_Floor"],
    },
    "Floor_Boss_Camel": {
        "dirs": ["/Game/World/Environment/Floors/Boss_Camel", "/Game/World/Environment/Floors/Boss_Perl"],
        "actor_labels": ["Demo_Field_1_VritraArena_Floor"],
    },
    "Floor_Boss_CMD": {
        "dirs": ["/Game/World/Environment/Floors/Boss_CMD"],
        "actor_labels": ["Demo_Field_4_CMDArena_Floor"],
    },
    "Wall_Corridor_Default": {
        "dirs": ["/Game/World/Environment/Walls/Corridor_Default"],
        "actor_labels": [
            "Demo_Field_1_VritraArena_Wall_N",
            "Demo_Field_1_VritraArena_Wall_S",
            "Demo_Field_1_VritraArena_Wall_E",
            "Demo_Field_1_VritraArena_Wall_W_L",
            "Demo_Field_1_VritraArena_Wall_W_R",
            "Demo_Field_2_PythonArena_Wall_N",
            "Demo_Field_2_PythonArena_Wall_S",
            "Demo_Field_2_PythonArena_Wall_E",
            "Demo_Field_2_PythonArena_Wall_W_L",
            "Demo_Field_2_PythonArena_Wall_W_R",
            "Demo_Field_3_SelvaraArena_Wall_N",
            "Demo_Field_3_SelvaraArena_Wall_S",
            "Demo_Field_3_SelvaraArena_Wall_E",
            "Demo_Field_3_SelvaraArena_Wall_W_L",
            "Demo_Field_3_SelvaraArena_Wall_W_R",
        ],
    },
    "Wall_Boss_CMD": {
        "dirs": ["/Game/World/Environment/Walls/Boss_CMD"],
        "actor_labels": [
            "Demo_Field_4_CMDArena_Wall_N",
            "Demo_Field_4_CMDArena_Wall_S",
            "Demo_Field_4_CMDArena_Wall_E",
            "Demo_Field_4_CMDArena_Wall_W_L",
            "Demo_Field_4_CMDArena_Wall_W_R",
        ],
    },
}


def log(message):
    unreal.log(f"[ApplyDemoEnvironmentMaterials] {message}")


def load_map():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")


def get_component(actor, component_class):
    try:
        component = actor.get_component_by_class(component_class)
        if component:
            return component
    except Exception:
        pass
    try:
        components = actor.get_components_by_class(component_class)
        if components:
            return components[0]
    except Exception:
        pass
    return None


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def list_assets_in_dirs(dirs):
    paths = []
    for directory in dirs:
        if unreal.EditorAssetLibrary.does_directory_exist(directory):
            paths.extend(unreal.EditorAssetLibrary.list_assets(directory, recursive=False, include_folder=False))
    return sorted(set(paths))


def load_first_texture(dirs):
    for path in list_assets_in_dirs(dirs):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Texture2D):
            return asset, path
    return None, None


def rename_texture_if_needed(texture_path, code_name):
    target_dir = "/".join(texture_path.split("/")[:-1])
    target_path = f"{target_dir}/T_{code_name}_BaseColor"
    if texture_path == target_path or unreal.EditorAssetLibrary.does_asset_exist(target_path):
        if unreal.EditorAssetLibrary.does_asset_exist(texture_path) and texture_path != target_path:
            unreal.EditorAssetLibrary.delete_asset(texture_path)
        return unreal.EditorAssetLibrary.load_asset(target_path), target_path

    if unreal.EditorAssetLibrary.rename_asset(texture_path, target_path):
        log(f"Renamed {texture_path} -> {target_path}")
        return unreal.EditorAssetLibrary.load_asset(target_path), target_path

    return unreal.EditorAssetLibrary.load_asset(texture_path), texture_path


def create_or_update_material(code_name, texture):
    material_dir = "/Game/World/Environment/Materials"
    material_path = f"{material_dir}/M_{code_name}"
    unreal.EditorAssetLibrary.make_directory(material_dir)

    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            f"M_{code_name}",
            material_dir,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
        if not material:
            raise RuntimeError(f"Failed to create material: {material_path}")
        log(f"Created material {material_path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    tex_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        -320,
        0,
    )
    tex_sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_property(tex_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(tex_sample, "A", unreal.MaterialProperty.MP_OPACITY)
    try:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    except Exception:
        pass
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def apply_material_to_labels(material, labels):
    applied = 0
    for label in labels:
        actor = find_actor(label)
        if not actor:
            log(f"Missing actor label: {label}")
            continue
        component = get_component(actor, unreal.StaticMeshComponent)
        if not component:
            log(f"Missing StaticMeshComponent: {label}")
            continue
        component.set_material(0, material)
        applied += 1
    return applied


def main():
    load_map()

    for code_name, config in ENV_SETS.items():
        texture, texture_path = load_first_texture(config["dirs"])
        if not texture:
            log(f"Skipped {code_name}: no Texture2D found in {config['dirs']}")
            continue

        texture, texture_path = rename_texture_if_needed(texture_path, code_name)
        material = create_or_update_material(code_name, texture)
        applied = apply_material_to_labels(material, config["actor_labels"])
        log(f"Applied M_{code_name} to {applied} actor(s)")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.EditorAssetLibrary.save_directory("/Game/World/Environment")
    log("Saved environment material setup.")


main()
