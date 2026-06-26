import unreal


ROOTS = [
    "/Game/World/Graves/PlayerGrave",
    "/Game/World/Portals/BossFogGate",
    "/Game/World/Altars/HiddenWeaponAltar",
    "/Game/World/Landmarks/SymbolTree",
    "/Game/World/Checkpoints/CheckpointBonfire",
    "/Game/Items/Consumables/RuntimeFlask/Filled",
    "/Game/Items/Consumables/RuntimeFlask/Empty",
    "/Game/Items/KeyItems/NellHiddenMemoryFragment",
    "/Game/Items/Weapons/Mimikatz/Left",
    "/Game/Items/Weapons/Mimikatz/Right",
    "/Game/Player/Hendel/Weapons/DefaultWeapon",
    "/Game/Bosses/CMD/Props/Throne",
    "/Game/Enemies/FieldMonsters/FieldMonster_01",
]


def main():
    for root in ROOTS:
        if not unreal.EditorAssetLibrary.does_directory_exist(root):
            unreal.log(f"[AuditImportedAssetClasses] Missing dir: {root}")
            continue
        unreal.log(f"[AuditImportedAssetClasses] DIR {root}")
        for path in sorted(unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False)):
            asset = unreal.EditorAssetLibrary.load_asset(path)
            class_name = asset.get_class().get_name() if asset else "None"
            unreal.log(f"[AuditImportedAssetClasses] {class_name} {path}")


main()
