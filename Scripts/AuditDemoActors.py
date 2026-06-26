import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
LABELS = [
    "Demo_Field_CheckpointBonfire",
    "Demo_Field_PlayerStart",
    "Demo_Field_HiddenWeaponAltar",
    "Demo_Field_MainPath_A",
    "Demo_Field_1_VritraArena_Floor",
    "Demo_Field_2_PythonArena_Floor",
    "Demo_Field_4_CMDArena_Floor",
    "Demo_Field_4_CMDArena_Wall_N",
    "Demo_Field_FogGate_CMD",
    "Demo_Field_RuntimeFlask_Filled",
    "Demo_Field_RuntimeFlask_Empty",
    "Demo_Field_SymbolTree",
    "Demo_Field_HendelDefaultWeapon",
    "Demo_CMD_Throne",
]


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


def get_asset_path(asset):
    if not asset:
        return "None"
    try:
        return asset.get_path_name()
    except Exception:
        return asset.get_name()


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() in LABELS:
            component = get_component(actor, unreal.StaticMeshComponent)
            mesh_path = "NoStaticMeshComponent"
            material_path = "NoStaticMeshComponent"
            if component:
                mesh_path = get_asset_path(component.static_mesh)
                material_path = get_asset_path(component.get_material(0))
            unreal.log(
                f"[AuditDemoActors] {actor.get_actor_label()} class={actor.get_class().get_name()} loc={actor.get_actor_location()} mesh={mesh_path} material={material_path}"
            )


main()
