from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets" / "ThirdParty"
DEST_ROOT = "/Game/ThirdParty/BossEnvironment/PolyHavenLOD"


def import_fbx(source, destination):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    mesh_data = options.get_editor_property("static_mesh_import_data")
    mesh_data.set_editor_property("combine_meshes", False)
    mesh_data.set_editor_property("auto_generate_collision", True)
    mesh_data.set_editor_property("generate_lightmap_u_vs", True)
    mesh_data.set_editor_property("convert_scene", True)
    mesh_data.set_editor_property("convert_scene_unit", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return list(task.get_editor_property("imported_object_paths"))


def main():
    imported = []
    imported.extend(
        import_fbx(
            SOURCE_ROOT / "PolyHaven_Boulder01" / "boulder_01_1k.fbx",
            f"{DEST_ROOT}/Boulder01",
        )
    )
    imported.extend(
        import_fbx(
            SOURCE_ROOT / "PolyHaven_Mountainside" / "mountainside_1k.fbx",
            f"{DEST_ROOT}/Mountainside",
        )
    )
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ImportPolyHavenLODMeshes] Imported {len(imported)} LOD objects: {imported}")


main()
