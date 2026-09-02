import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
ASSET_ROOT = "/Game/ThirdParty/BossEnvironment"
ACTOR_PREFIX = "BossEnv_"
REQUIRED_MESHES = [
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Gate",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_RoomCorner",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Wall",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_WallHalf",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_WallDetail",
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
PYTHON_CENTER = ARENAS["Python"]
PYTHON_ENTRANCE = (1820.0, 5200.0)
PYTHON_EXIT = (6460.0, 5200.0)
PYTHON_PLAYER_STAGING = (2700.0, 5200.0)
PYTHON_BOSS_ANCHORS = {
    "Python_Vethara": (4300.0, 4650.0),
    "Python_Aurathos": (4300.0, 5750.0),
}
PYTHON_ROUTE_HALF_WIDTH = 600.0
PYTHON_CENTER_VISUAL_CLEARANCE = 500.0
PYTHON_BOSS_BOUNDS_CLEARANCE = 650.0
PYTHON_PLAYER_BOUNDS_CLEARANCE = 600.0
PYTHON_ENTRANCE_BOUNDS_CLEARANCE = 50.0
PYTHON_REQUIRED_LABELS = {
    "BossEnv_Python_Entry_VetharaPier",
    "BossEnv_Python_Entry_AurathosPier",
    "BossEnv_Python_Exit_VetharaPier",
    "BossEnv_Python_Exit_AurathosPier",
    "BossEnv_Python_Vethara_KeepFragment",
    "BossEnv_Python_Vethara_BackArch",
    "BossEnv_Python_Vethara_KeyLight",
    "BossEnv_Python_Aurathos_KeepFragment",
    "BossEnv_Python_Aurathos_BackArch",
    "BossEnv_Python_Aurathos_KeyLight",
}
PYTHON_REQUIRED_LABELS.update(
    f"BossEnv_Python_{side}_OuterBoulder_{index:02d}"
    for side in ("Vethara", "Aurathos")
    for index in range(4)
)
PYTHON_REQUIRED_LABELS.update(
    f"BossEnv_Python_{side}_BrokenWall_{index:02d}"
    for side in ("Vethara", "Aurathos")
    for index in range(3)
)
PYTHON_EXPECTED_ACTOR_COUNT = len(PYTHON_REQUIRED_LABELS)
PYTHON_ALLOWED_MESHES = {
    f"{ASSET_ROOT}/PolyHavenLOD/Boulder01/boulder_01_LOD1",
    f"{ASSET_ROOT}/PolyHaven/SM_PH_ModularFort01",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Gate",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_RoomCorner",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_Wall",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_WallHalf",
    f"{ASSET_ROOT}/KenneyDungeon/SM_KD_WallDetail",
}
PYTHON_ALLOWED_MATERIALS = {
    f"{ASSET_ROOT}/Materials/M_PH_Boulder01",
    f"{ASSET_ROOT}/Materials/M_KD_Stone",
    f"{ASSET_ROOT}/Materials/M_PH_Fort_Wall",
    f"{ASSET_ROOT}/Materials/M_PH_Fort_Trim",
}
PYTHON_LIGHTS = {
    "BossEnv_Python_Vethara_KeyLight": ((0, 102, 255), 520.0),
    "BossEnv_Python_Aurathos_KeyLight": ((255, 140, 0), 500.0),
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


def package_path(value):
    return value.get_path_name().split(".", 1)[0] if value else "None"


def distance_2d(actor, anchor):
    location = actor.get_actor_location()
    return math.hypot(location.x - anchor[0], location.y - anchor[1])


def bounds_2d(actor):
    origin, extent = actor.get_actor_bounds(False, False)
    return origin, extent


def point_to_bounds_clearance_2d(actor, anchor):
    origin, extent = bounds_2d(actor)
    dx = max(abs(anchor[0] - origin.x) - extent.x, 0.0)
    dy = max(abs(anchor[1] - origin.y) - extent.y, 0.0)
    return math.hypot(dx, dy)


def opening_width(south_actor, north_actor):
    south_origin, south_extent = bounds_2d(south_actor)
    north_origin, north_extent = bounds_2d(north_actor)
    return (north_origin.y - north_extent.y) - (south_origin.y + south_extent.y)


def actor_bounds_clearance_2d(first, second, first_collision_only=False, second_collision_only=False):
    first_origin, first_extent = first.get_actor_bounds(first_collision_only, False)
    second_origin, second_extent = second.get_actor_bounds(second_collision_only, False)
    dx = max(abs(first_origin.x - second_origin.x) - first_extent.x - second_extent.x, 0.0)
    dy = max(abs(first_origin.y - second_origin.y) - first_extent.y - second_extent.y, 0.0)
    return math.hypot(dx, dy)


def audit_python_arena(actors, by_label, python_actors):
    labels = {actor.get_actor_label() for actor in python_actors}
    missing = sorted(PYTHON_REQUIRED_LABELS - labels)
    if missing:
        fail(f"Python arena is missing required presentation actors: {missing}")
    if "BossEnv_Python_CitadelShell" in labels:
        fail("Python arena still has the center-filling CitadelShell")
    unexpected = sorted(labels - PYTHON_REQUIRED_LABELS)
    if unexpected:
        fail(f"Python arena has unmanaged presentation actors: {unexpected}")
    if len(python_actors) != PYTHON_EXPECTED_ACTOR_COUNT:
        fail(
            f"Python environment actor count differs: actual={len(python_actors)}, "
            f"expected={PYTHON_EXPECTED_ACTOR_COUNT}"
        )

    unique_labels = PYTHON_REQUIRED_LABELS | set(PYTHON_BOSS_ANCHORS) | {"BossPlate_2_PythonArena"}
    for label in sorted(unique_labels):
        matches = [actor for actor in actors if actor.get_actor_label() == label]
        if len(matches) != 1:
            fail(f"Expected one actor labelled {label}, found {len(matches)}")

    python_mesh_actors = []
    for actor in python_actors:
        label = actor.get_actor_label()
        location = actor.get_actor_location()
        if "Vethara" in label and location.y >= PYTHON_CENTER[1]:
            fail(f"Vethara presentation actor crossed into the Aurathos side: {label}")
        if "Aurathos" in label and location.y <= PYTHON_CENTER[1]:
            fail(f"Aurathos presentation actor crossed into the Vethara side: {label}")

        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        if not component:
            continue
        python_mesh_actors.append(actor)
        mesh_path = package_path(component.get_editor_property("static_mesh"))
        if mesh_path not in PYTHON_ALLOWED_MESHES:
            fail(f"Python arena uses an unapproved mesh: {actor.get_actor_label()} -> {mesh_path}")
        for material_index in range(component.get_num_materials()):
            material_path = package_path(component.get_material(material_index))
            if material_path not in PYTHON_ALLOWED_MATERIALS:
                fail(
                    f"Python arena uses an unapproved material: {actor.get_actor_label()} "
                    f"slot={material_index} -> {material_path}"
                )

    center_nearest = min(
        ((point_to_bounds_clearance_2d(actor, PYTHON_CENTER), actor.get_actor_label()) for actor in python_mesh_actors),
        default=(float("inf"), "None"),
    )
    if center_nearest[0] < PYTHON_CENTER_VISUAL_CLEARANCE:
        fail(
            f"Python combat center has only {center_nearest[0]:.1f} cm visual clearance "
            f"from {center_nearest[1]}"
        )

    colliding = [
        actor for actor in python_actors
        if actor.get_actor_enable_collision()
        and actor.get_component_by_class(unreal.StaticMeshComponent)
    ]
    route_blockers = []
    for actor in colliding:
        origin, extent = bounds_2d(actor)
        overlaps_route_x = (
            origin.x + extent.x > PYTHON_ENTRANCE[0]
            and origin.x - extent.x < PYTHON_EXIT[0]
        )
        intrudes_route_y = abs(origin.y - PYTHON_CENTER[1]) - extent.y < PYTHON_ROUTE_HALF_WIDTH
        if overlaps_route_x and intrudes_route_y:
            route_blockers.append(actor.get_actor_label())
    if route_blockers:
        fail(f"Python entrance-to-exit lane is narrower than 1200 cm at: {route_blockers}")

    for boss_label, anchor in PYTHON_BOSS_ANCHORS.items():
        boss = by_label.get(boss_label)
        if not boss:
            fail(f"Missing authored Python boss: {boss_label}")
        if distance_2d(boss, anchor) > 5.0:
            fail(f"{boss_label} moved away from its authored spawn anchor")
        blockers = [
            (point_to_bounds_clearance_2d(actor, anchor), actor.get_actor_label())
            for actor in colliding
        ]
        nearest = min(blockers, default=(float("inf"), "None"))
        if nearest[0] < PYTHON_BOSS_BOUNDS_CLEARANCE:
            fail(
                f"{boss_label} has only {nearest[0]:.1f} cm bounds clearance "
                f"from {nearest[1]}"
            )

    vethara_location = by_label["Python_Vethara"].get_actor_location()
    aurathos_location = by_label["Python_Aurathos"].get_actor_location()
    boss_spacing = math.hypot(
        vethara_location.x - aurathos_location.x,
        vethara_location.y - aurathos_location.y,
    )
    if abs(boss_spacing - 1100.0) > 10.0:
        fail(f"Python boss spacing changed from 1100 cm: actual={boss_spacing:.1f} cm")

    player_nearest = min(
        ((point_to_bounds_clearance_2d(actor, PYTHON_PLAYER_STAGING), actor.get_actor_label()) for actor in colliding),
        default=(float("inf"), "None"),
    )
    if player_nearest[0] < PYTHON_PLAYER_BOUNDS_CLEARANCE:
        fail(
            f"Python player staging has only {player_nearest[0]:.1f} cm bounds clearance "
            f"from {player_nearest[1]}"
        )

    entrance = by_label.get("BossPlate_2_PythonArena")
    if not entrance or distance_2d(entrance, PYTHON_ENTRANCE) > 25.0:
        fail("Python arena entrance trigger moved away from (1820, 5200)")
    entrance_nearest = min(
        (
            (actor_bounds_clearance_2d(entrance, actor, True, False), actor.get_actor_label())
            for actor in colliding
        ),
        default=(float("inf"), "None"),
    )
    if entrance_nearest[0] < PYTHON_ENTRANCE_BOUNDS_CLEARANCE:
        fail(
            f"Python entrance trigger has only {entrance_nearest[0]:.1f} cm bounds clearance "
            f"from {entrance_nearest[1]}"
        )
    exit_candidates = [
        by_label.get("Explore_StoryGate_Python"),
        by_label.get("Demo_Field_2_PythonArena_ExitGate"),
    ]
    if not any(candidate and distance_2d(candidate, PYTHON_EXIT) <= 100.0 for candidate in exit_candidates):
        fail("Python arena exit moved away from (6460, 5200)")

    entry_width = opening_width(
        by_label["BossEnv_Python_Entry_VetharaPier"],
        by_label["BossEnv_Python_Entry_AurathosPier"],
    )
    exit_width = opening_width(
        by_label["BossEnv_Python_Exit_VetharaPier"],
        by_label["BossEnv_Python_Exit_AurathosPier"],
    )
    if entry_width < 1200.0 or exit_width < 1200.0:
        fail(f"Python thresholds are too narrow: entrance={entry_width:.1f}, exit={exit_width:.1f}")

    for label, (expected_rgb, expected_intensity) in PYTHON_LIGHTS.items():
        actor = by_label.get(label)
        component = actor.get_component_by_class(unreal.PointLightComponent) if actor else None
        if not component:
            fail(f"Missing Python identity light: {label}")
        color = component.get_editor_property("light_color")
        actual_rgb = (int(color.r), int(color.g), int(color.b))
        intensity = float(component.get_editor_property("intensity"))
        if any(abs(actual - expected) > 1 for actual, expected in zip(actual_rgb, expected_rgb)):
            fail(f"Wrong identity color on {label}: actual={actual_rgb}, expected={expected_rgb}")
        if abs(intensity - expected_intensity) > 1.0:
            fail(f"Wrong restrained intensity on {label}: {intensity}")

    return entry_width, exit_width, player_nearest[0], center_nearest[0], entrance_nearest[0]


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
        fail("Boss environment upgrade has not been built")

    labels = {actor.get_actor_label() for actor in upgrade_actors}
    if len(labels) != len(upgrade_actors):
        fail("Duplicate BossEnv actor labels found.")

    by_label = {actor.get_actor_label(): actor for actor in actors}
    python_actors = []
    for arena_name, center in ARENAS.items():
        arena_actors = [actor for actor in upgrade_actors if actor.get_actor_label().startswith(f"{ACTOR_PREFIX}{arena_name}_")]
        if arena_name == "Python":
            python_actors = arena_actors
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

    entry_width, exit_width, player_clearance, center_clearance, entrance_clearance = audit_python_arena(
        actors,
        by_label,
        python_actors,
    )

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
            intensity = float(component.get_editor_property("intensity"))
            if intensity > 1.2:
                fail(f"Directional light is overexposed: {actor.get_actor_label()} intensity={intensity}")
            unreal.log(
                f"[AuditBossEnvironmentUpgrade] LIGHT Directional {actor.get_actor_label()} "
                f"intensity={intensity}"
            )
        elif isinstance(actor, unreal.SkyLight):
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            intensity = float(component.get_editor_property("intensity"))
            if intensity > 0.6:
                fail(f"Sky light is overexposed: {actor.get_actor_label()} intensity={intensity}")
            unreal.log(
                f"[AuditBossEnvironmentUpgrade] LIGHT Sky {actor.get_actor_label()} "
                f"intensity={intensity} "
                f"realtime={component.get_editor_property('real_time_capture')}"
            )

    unreal.log(
        f"[AuditBossEnvironmentUpgrade] PASS: {len(upgrade_actors)} dressed arena actors, "
        f"4 themed boss rooms, Python twin ruin actors={len(python_actors)}, "
        f"thresholds=({entry_width:.0f},{exit_width:.0f})cm, "
        f"player_clearance={player_clearance:.0f}cm, center_visual={center_clearance:.0f}cm, "
        f"entrance_clearance={entrance_clearance:.0f}cm, no old box walls, combat centers clear."
    )


main()
