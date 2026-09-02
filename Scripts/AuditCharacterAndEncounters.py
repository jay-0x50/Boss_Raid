import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"


def log(message):
    unreal.log(f"[AuditCharacterAndEncounters] {message}")


def asset_name(value):
    return value.get_path_name() if value else "None"


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    for actor in actors:
        class_name = actor.get_class().get_name()
        label = actor.get_actor_label()
        if any(token in f"{class_name} {label}".lower() for token in ("enemy", "spawner", "nel", "storyintro")):
            location = actor.get_actor_location()
            log(f"ACTOR {label} class={class_name} loc=({location.x:.0f},{location.y:.0f},{location.z:.0f})")

    bp_class = unreal.load_class(None, "/Game/Blueprints/Core/BP_ExceptionCharacter.BP_ExceptionCharacter_C")
    if not bp_class:
        raise RuntimeError("BP_ExceptionCharacter_C was not found")
    cdo = unreal.get_default_object(bp_class)
    mesh = cdo.get_component_by_class(unreal.SkeletalMeshComponent)
    log(f"PLAYER class={bp_class.get_name()} mesh={asset_name(mesh.get_editor_property('skeletal_mesh_asset'))}")
    log(f"PLAYER anim={asset_name(mesh.get_editor_property('anim_class'))} materials={mesh.get_num_materials()}")
    for index in range(mesh.get_num_materials()):
        log(f"PLAYER material[{index}]={asset_name(mesh.get_material(index))}")

    enemy_bp = unreal.load_class(None, "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemy.BP_CombatEnemy_C")
    spawner_bp = unreal.load_class(None, "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemySpawner.BP_CombatEnemySpawner_C")
    log(f"ENEMY_BP={asset_name(enemy_bp)} SPAWNER_BP={asset_name(spawner_bp)}")


main()
