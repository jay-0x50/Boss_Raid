import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ASSET_ROOT = "/Game/ThirdParty/BossEnvironment"
ACTOR_PREFIX = "BossEnv_"
REQUIRED_MESHES = [
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Gate",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_RoomCorner",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Wall",
    f"{ASSET_ROOT}/PolyHavenLOD/Boulder01/boulder_01_LOD1",
    f"{ASSET_ROOT}/PolyHavenLOD/Mountainside/mountainside_LOD1",
    f"{ASSET_ROOT}/PolyHaven/SM_PH_ModularFort01",
]
ARENAS = {
    "Vritra": (4300.0, -5200.0),
    "Python": (4300.0, 5200.0),
    "Selvara": (9500.0, 5200.0),
    "CMD": (14800.0, 0.0),
}


def fail(message):
    raise RuntimeError(f"[AuditBossEnvironmentUpgrade] {message}")


def describe_mesh(path):
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        fail(f"Missing StaticMesh: {path}")
    bounds = mesh.get_bounds()
    unreal.log(
        f"[AuditBossEnvironmentUpgrade] ASSET {path} "
        f"extent=({bounds.box_extent.x:.1f},{bounds.box_extent.y:.1f},{bounds.box_extent.z:.1f}) "
        f"sections={mesh.get_num_sections(0)} materials={len(mesh.get_editor_property('static_materials'))}"
    )
    return mesh


def current_map_path():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    return world.get_path_name().split(".", 1)[0]


def main():
    for path in REQUIRED_MESHES:
        describe_mesh(path)

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail(f"Could not load map: {MAP_PATH}")
    if current_map_path() != MAP_PATH:
        fail(f"Wrong map loaded: {current_map_path()}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor_subsystem:
        fail("EditorActorSubsystem is unavailable.")
    actors = list(actor_subsystem.get_all_level_actors())
    upgrade_actors = [actor for actor in actors if actor.get_actor_label().startswith(ACTOR_PREFIX)]
    if not upgrade_actors:
        unreal.log("[AuditBossEnvironmentUpgrade] Asset import passed; map upgrade not built yet.")
        return

    labels = {actor.get_actor_label() for actor in upgrade_actors}
    if len(labels) != len(upgrade_actors):
        fail("Duplicate BossEnv actor labels found.")

    for arena_name, center in ARENAS.items():
        arena_actors = [actor for actor in upgrade_actors if actor.get_actor_label().startswith(f"{ACTOR_PREFIX}{arena_name}_")]
        if len(arena_actors) < 12:
            fail(f"{arena_name} has only {len(arena_actors)} environment actors.")
        center_blockers = []
        for actor in arena_actors:
            location = actor.get_actor_location()
            distance = math.hypot(location.x - center[0], location.y - center[1])
            if distance < 850.0 and actor.get_actor_enable_collision():
                center_blockers.append(actor.get_actor_label())
        if center_blockers:
            fail(f"{arena_name} combat center is blocked by {center_blockers}")

    old_box_walls = [
        actor.get_actor_label()
        for actor in actors
        if actor.get_actor_label().startswith("Demo_Field_") and "Arena_Wall_" in actor.get_actor_label()
    ]
    if old_box_walls:
        fail(f"Old rectangular arena walls remain: {old_box_walls}")

    for actor in actors:
        if isinstance(actor, unreal.DirectionalLight):
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            unreal.log(
                f"[AuditBossEnvironmentUpgrade] LIGHT Directional {actor.get_actor_label()} "
                f"intensity={component.get_editor_property('intensity')}"
            )
        elif isinstance(actor, unreal.SkyLight):
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            unreal.log(
                f"[AuditBossEnvironmentUpgrade] LIGHT Sky {actor.get_actor_label()} "
                f"intensity={component.get_editor_property('intensity')} "
                f"realtime={component.get_editor_property('real_time_capture')}"
            )

    unreal.log(
        f"[AuditBossEnvironmentUpgrade] PASS: {len(upgrade_actors)} dressed arena actors, "
        "4 themed boss rooms, no old box walls, combat centers clear."
    )


main()
