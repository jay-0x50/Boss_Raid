from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets" / "ThirdParty"
DEST_ROOT = "/Game/ThirdParty/BossEnvironment"


def log(message):
    unreal.log(f"[ImportBossEnvironmentAssets] {message}")


def make_fbx_task(source_file, destination_name, destination_path, combine_meshes=False):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)

    mesh_data = options.get_editor_property("static_mesh_import_data")
    mesh_data.set_editor_property("combine_meshes", combine_meshes)
    mesh_data.set_editor_property("auto_generate_collision", True)
    mesh_data.set_editor_property("generate_lightmap_u_vs", True)
    mesh_data.set_editor_property("convert_scene", True)
    mesh_data.set_editor_property("convert_scene_unit", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def make_texture_task(source_file, destination_name, destination_path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    return task


def import_sources():
    kenney_source = SOURCE_ROOT / "Kenney_ModularDungeon"
    kenney_dest = f"{DEST_ROOT}/KenneyDungeon"
    kenney_meshes = {
        "corridor-corner.fbx": "SM_KD_CorridorCorner",
        "corridor-end.fbx": "SM_KD_CorridorEnd",
        "gate.fbx": "SM_KD_Gate",
        "gate-metal-bars.fbx": "SM_KD_GateBars",
        "room-corner.fbx": "SM_KD_RoomCorner",
        "stairs-wide.fbx": "SM_KD_StairsWide",
        "template-floor-big.fbx": "SM_KD_FloorBig",
        "template-floor-detail-a.fbx": "SM_KD_FloorDetail",
        "template-wall.fbx": "SM_KD_Wall",
        "template-wall-corner.fbx": "SM_KD_WallCorner",
        "template-wall-detail-a.fbx": "SM_KD_WallDetail",
        "template-wall-half.fbx": "SM_KD_WallHalf",
        "template-wall-top.fbx": "SM_KD_WallTop",
    }

    tasks = []
    for filename, asset_name in kenney_meshes.items():
        tasks.append(make_fbx_task(kenney_source / filename, asset_name, kenney_dest))
    tasks.append(make_texture_task(kenney_source / "colormap.png", "T_KD_Color", kenney_dest))

    poly_assets = [
        (
            "PolyHaven_Boulder01",
            "boulder_01_1k.fbx",
            "SM_PH_Boulder01",
            {
                "textures/boulder_01_diff_1k.jpg": "T_PH_Boulder01_D",
                "textures/boulder_01_nor_gl_1k.exr": "T_PH_Boulder01_N",
                "textures/boulder_01_rough_1k.exr": "T_PH_Boulder01_R",
            },
        ),
        (
            "PolyHaven_Mountainside",
            "mountainside_1k.fbx",
            "SM_PH_Mountainside",
            {
                "textures/mountainside_diff_1k.jpg": "T_PH_Mountainside_D",
                "textures/mountainside_nor_gl_1k.exr": "T_PH_Mountainside_N",
                "textures/mountainside_rough_1k.exr": "T_PH_Mountainside_R",
            },
        ),
        (
            "PolyHaven_ModularFort01",
            "modular_fort_01_1k.fbx",
            "SM_PH_ModularFort01",
            {
                "textures/modular_fort_01_plaster_diff_1k.png": "T_PH_Fort_Plaster_D",
                "textures/modular_fort_01_plaster_nor_gl_1k.png": "T_PH_Fort_Plaster_N",
                "textures/modular_fort_01_plaster_rough_1k.png": "T_PH_Fort_Plaster_R",
                "textures/modular_fort_01_trim_diff_1k.png": "T_PH_Fort_Trim_D",
                "textures/modular_fort_01_trim_nor_gl_1k.png": "T_PH_Fort_Trim_N",
                "textures/modular_fort_01_trim_rough_1k.png": "T_PH_Fort_Trim_R",
                "textures/modular_fort_01_wall_diff_1k.png": "T_PH_Fort_Wall_D",
                "textures/modular_fort_01_wall_nor_gl_1k.png": "T_PH_Fort_Wall_N",
                "textures/modular_fort_01_wall_rough_1k.png": "T_PH_Fort_Wall_R",
            },
        ),
    ]

    poly_dest = f"{DEST_ROOT}/PolyHaven"
    for folder, fbx_name, asset_name, textures in poly_assets:
        source_dir = SOURCE_ROOT / folder
        tasks.append(make_fbx_task(source_dir / fbx_name, asset_name, poly_dest, combine_meshes=True))
        for texture_file, texture_name in textures.items():
            tasks.append(make_texture_task(source_dir / texture_file, texture_name, poly_dest))

    missing = [task.get_editor_property("filename") for task in tasks if not Path(task.get_editor_property("filename")).is_file()]
    if missing:
        raise RuntimeError(f"Missing source files: {missing}")

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        imported.extend(task.get_editor_property("imported_object_paths"))
    log(f"Imported or updated {len(imported)} objects.")


def get_asset(path, expected_type):
    asset = unreal.load_asset(path)
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"Missing {expected_type.__name__}: {path}")
    return asset


def configure_texture(path, normal=False, linear=False):
    texture = get_asset(path, unreal.Texture2D)
    if normal:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("flip_green_channel", True)
    elif linear:
        texture.set_editor_property("srgb", False)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def create_material(name, base_color, normal=None, roughness=None, roughness_value=0.82):
    material_path = f"{DEST_ROOT}/Materials/{name}"
    material = unreal.load_asset(material_path)
    if not isinstance(material, unreal.Material):
        unreal.EditorAssetLibrary.make_directory(f"{DEST_ROOT}/Materials")
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            f"{DEST_ROOT}/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create material: {material_path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    color_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -420, -80
    )
    color_sample.set_editor_property("texture", get_asset(base_color, unreal.Texture2D))
    unreal.MaterialEditingLibrary.connect_material_property(
        color_sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    if normal:
        normal_sample = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -420, 140
        )
        normal_sample.set_editor_property("texture", configure_texture(normal, normal=True))
        normal_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        unreal.MaterialEditingLibrary.connect_material_property(
            normal_sample, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if roughness:
        rough_sample = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -420, 340
        )
        rough_sample.set_editor_property("texture", configure_texture(roughness, linear=True))
        unreal.MaterialEditingLibrary.connect_material_property(
            rough_sample, "R", unreal.MaterialProperty.MP_ROUGHNESS
        )
    else:
        rough_constant = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -220, 300
        )
        rough_constant.set_editor_property("r", roughness_value)
        unreal.MaterialEditingLibrary.connect_material_property(
            rough_constant, "", unreal.MaterialProperty.MP_ROUGHNESS
        )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def assign_materials():
    kd_material = create_material(
        "M_KD_Stone",
        f"{DEST_ROOT}/KenneyDungeon/T_KD_Color",
        roughness_value=0.9,
    )
    boulder_material = create_material(
        "M_PH_Boulder01",
        f"{DEST_ROOT}/PolyHaven/T_PH_Boulder01_D",
        f"{DEST_ROOT}/PolyHaven/T_PH_Boulder01_N",
        f"{DEST_ROOT}/PolyHaven/T_PH_Boulder01_R",
    )
    mountain_material = create_material(
        "M_PH_Mountainside",
        f"{DEST_ROOT}/PolyHaven/T_PH_Mountainside_D",
        f"{DEST_ROOT}/PolyHaven/T_PH_Mountainside_N",
        f"{DEST_ROOT}/PolyHaven/T_PH_Mountainside_R",
    )
    fort_materials = {
        "plaster": create_material(
            "M_PH_Fort_Plaster",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Plaster_D",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Plaster_N",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Plaster_R",
        ),
        "trim": create_material(
            "M_PH_Fort_Trim",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Trim_D",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Trim_N",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Trim_R",
        ),
        "wall": create_material(
            "M_PH_Fort_Wall",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Wall_D",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Wall_N",
            f"{DEST_ROOT}/PolyHaven/T_PH_Fort_Wall_R",
        ),
    }

    kenney_assets = unreal.EditorAssetLibrary.list_assets(
        f"{DEST_ROOT}/KenneyDungeon", recursive=True, include_folder=False
    )
    for path in kenney_assets:
        mesh = unreal.load_asset(path)
        if isinstance(mesh, unreal.StaticMesh):
            for index in range(mesh.get_num_sections(0)):
                mesh.set_material(index, kd_material)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    for mesh_name, material in (
        ("SM_PH_Boulder01", boulder_material),
        ("SM_PH_Mountainside", mountain_material),
    ):
        mesh = get_asset(f"{DEST_ROOT}/PolyHaven/{mesh_name}", unreal.StaticMesh)
        for index in range(max(1, mesh.get_num_sections(0))):
            mesh.set_material(index, material)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    fort = get_asset(f"{DEST_ROOT}/PolyHaven/SM_PH_ModularFort01", unreal.StaticMesh)
    static_materials = list(fort.get_editor_property("static_materials"))
    material_order = [fort_materials["wall"], fort_materials["plaster"], fort_materials["trim"]]
    for index in range(max(1, fort.get_num_sections(0))):
        slot_name = ""
        if index < len(static_materials):
            slot_name = str(static_materials[index].get_editor_property("material_slot_name")).lower()
        selected = next((value for key, value in fort_materials.items() if key in slot_name), material_order[index % 3])
        fort.set_material(index, selected)
    unreal.EditorAssetLibrary.save_loaded_asset(fort)


def main():
    import_sources()
    assign_materials()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Boss environment assets are ready.")


main()
