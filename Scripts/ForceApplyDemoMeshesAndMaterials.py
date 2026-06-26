import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"

ENV_LABELS = {
    "Demo_Field_MainPath_A": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Field_Default"),
    "Demo_Field_MainPath_B": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Field_Default"),
    "Demo_Field_MainPath_C": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Field_Default"),
    "Demo_Field_MainPath_D": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Field_Default"),
    "Demo_Field_1_VritraArena_Floor": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Boss_Camel"),
    "Demo_Field_2_PythonArena_Floor": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Boss_Python"),
    "Demo_Field_3_SelvaraArena_Floor": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Boss_Python"),
    "Demo_Field_4_CMDArena_Floor": (CUBE_PATH, "/Game/World/Environment/Materials/M_Floor_Boss_CMD"),
}

for arena in ["1_VritraArena", "2_PythonArena", "3_SelvaraArena"]:
    for suffix in ["Wall_N", "Wall_S", "Wall_E", "Wall_W_L", "Wall_W_R"]:
        ENV_LABELS[f"Demo_Field_{arena}_{suffix}"] = (CUBE_PATH, "/Game/World/Environment/Materials/M_Wall_Corridor_Default")

for suffix in ["Wall_N", "Wall_S", "Wall_E", "Wall_W_L", "Wall_W_R"]:
    ENV_LABELS[f"Demo_Field_4_CMDArena_{suffix}"] = (CUBE_PATH, "/Game/World/Environment/Materials/M_Wall_Boss_CMD")


VISUAL_LABELS = {
    "Demo_Field_HiddenWeaponAltar": (
        "/Game/World/Altars/HiddenWeaponAltar/SM_HiddenWeaponAltar",
        "/Game/World/Altars/HiddenWeaponAltar/M_HiddenWeaponAltar",
    ),
    "Demo_Field_FogGate_Vritra": (
        "/Game/World/Portals/BossFogGate/SM_BossFogGate",
        "/Game/World/Portals/BossFogGate/M_BossFogGate",
    ),
    "Demo_Field_FogGate_Python": (
        "/Game/World/Portals/BossFogGate/SM_BossFogGate",
        "/Game/World/Portals/BossFogGate/M_BossFogGate",
    ),
    "Demo_Field_FogGate_Selvara": (
        "/Game/World/Portals/BossFogGate/SM_BossFogGate",
        "/Game/World/Portals/BossFogGate/M_BossFogGate",
    ),
    "Demo_Field_FogGate_CMD": (
        "/Game/World/Portals/BossFogGate/SM_BossFogGate",
        "/Game/World/Portals/BossFogGate/M_BossFogGate",
    ),
    "Demo_Field_NellHiddenMemoryFragment": (
        "/Game/Items/KeyItems/NellHiddenMemoryFragment/SM_NellHiddenMemoryFragment",
        "/Game/Items/KeyItems/NellHiddenMemoryFragment/M_NellHiddenMemoryFragment",
    ),
    "Demo_Field_MimikatzAuthoritySeized_L": (
        "/Game/Items/Weapons/Mimikatz/Left/SM_MimikatzAuthoritySeized_L",
        "/Game/Items/Weapons/Mimikatz/Left/M_MimikatzAuthoritySeized_L",
    ),
    "Demo_Field_MimikatzAuthoritySeized_R": (
        "/Game/Items/Weapons/Mimikatz/Right/SM_MimikatzAuthoritySeized_R",
        "/Game/Items/Weapons/Mimikatz/Right/M_MimikatzAuthoritySeized_R",
    ),
    "Demo_Field_FieldMonster_01_Visual": (
        "/Game/Enemies/FieldMonsters/FieldMonster_01/SM_FieldMonster_01",
        "/Game/Enemies/FieldMonsters/FieldMonster_01/M_FieldMonster_01",
    ),
    "Demo_Field_RuntimeFlask_Filled": (
        "/Game/Items/Consumables/RuntimeFlask/Filled/SM_RuntimeFlask_Filled",
        "/Game/Items/Consumables/RuntimeFlask/Filled/M_RuntimeFlask_Filled",
    ),
    "Demo_Field_RuntimeFlask_Empty": (
        "/Game/Items/Consumables/RuntimeFlask/Empty/SM_RuntimeFlask_Empty",
        "/Game/Items/Consumables/RuntimeFlask/Empty/M_RuntimeFlask_Empty",
    ),
    "Demo_Field_SymbolTree": (
        "/Game/World/Landmarks/SymbolTree/SM_SymbolTree",
        "/Game/World/Landmarks/SymbolTree/M_SymbolTree",
    ),
    "Demo_Field_HendelDefaultWeapon": (
        "/Game/Player/Hendel/Weapons/DefaultWeapon/SM_Hendel_DefaultWeapon",
        "/Game/Player/Hendel/Weapons/DefaultWeapon/M_Hendel_DefaultWeapon",
    ),
    "Demo_CMD_Throne": (
        "/Game/Bosses/CMD/Props/Throne/SM_CMD_Throne",
        "/Game/Bosses/CMD/Props/Throne/M_CMD_Throne",
    ),
    "Demo_Field_CheckpointBonfire": (
        "/Game/World/Checkpoints/CheckpointBonfire/SM_CheckpointBonfire",
        "/Game/World/Checkpoints/CheckpointBonfire/M_CheckpointBonfire",
    ),
}


def log(message):
    unreal.log(f"[ForceApplyDemoMeshesAndMaterials] {message}")


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


def load_asset(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        log(f"Missing asset: {path}")
        return None
    if expected_type and not isinstance(asset, expected_type):
        log(f"Wrong asset type for {path}: {asset.get_class().get_name()}")
        return None
    return asset


def apply_to_actor(label, mesh_path, material_path):
    actor = find_actor(label)
    if not actor:
        log(f"Missing actor: {label}")
        return False

    component = get_component(actor, unreal.StaticMeshComponent)
    if not component:
        log(f"Missing StaticMeshComponent: {label}")
        return False

    mesh = load_asset(mesh_path, unreal.StaticMesh)
    material = load_asset(material_path, unreal.MaterialInterface)

    actor.modify()
    component.modify()
    if mesh:
        component.set_editor_property("static_mesh", mesh)
        try:
            component.set_static_mesh(mesh)
        except Exception:
            pass
    if material:
        component.set_material(0, material)
        try:
            component.set_editor_property("override_materials", [material])
        except Exception:
            pass

    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    for obj in [component, actor]:
        try:
            obj.post_edit_change()
        except Exception:
            pass
    try:
        component.mark_render_state_dirty()
    except Exception:
        pass
    log(f"Applied {label}: mesh={mesh_path} material={material_path}")
    return True


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Failed to load map: {MAP_PATH}")

    applied = 0
    for label, (mesh_path, material_path) in {**ENV_LABELS, **VISUAL_LABELS}.items():
        if apply_to_actor(label, mesh_path, material_path):
            applied += 1

    unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Saved forced demo mesh/material assignment. Applied={applied}")


main()
