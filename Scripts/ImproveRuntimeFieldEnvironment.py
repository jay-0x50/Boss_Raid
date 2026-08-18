import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
LABEL_PREFIX = "Demo_Env_"
FOLDER_ROOT = "Demo/Environment"
BLOCK_ALL_PROFILE = unreal.Name("BlockAll")
NO_COLLISION_PROFILE = unreal.Name("NoCollision")
MAX_DESIGN_ACTORS = 190

ROLE_GROUND = "ground"
ROLE_OBSTACLE = "obstacle"
ROLE_DETAIL = "detail"
ROLE_SILHOUETTE = "silhouette"
VALID_ROLES = {ROLE_GROUND, ROLE_OBSTACLE, ROLE_DETAIL, ROLE_SILHOUETTE}

MESH_PATHS = {
    "rock": "/Game/LevelPrototyping/Meshes/SM_ChamferCube",
    "column": "/Game/LevelPrototyping/Meshes/SM_Cylinder",
    "ramp": "/Game/LevelPrototyping/Meshes/SM_Ramp",
    "arch": "/Game/LevelPrototyping/Meshes/SM_QuarterCylinder",
    "arc": "/Game/LevelPrototyping/Meshes/SM_QuarterCylinderOuter",
    "door_corner": "/Game/LevelPrototyping/Interactable/Door/Meshes/SM_DoorFrame_Corner",
    "ring": "/Game/LevelPrototyping/Interactable/JumpPad/Assets/Meshes/SM_CircularBand",
}

# The imported SymbolTree, BossFogGate, bonfire, grave, and altar meshes are
# intentionally excluded here. Each mesh is roughly 30-69 MB before its texture
# set, while this pass reuses sub-0.3 MB meshes and the six materials already
# loaded by the field.

MATERIAL_PATHS = {
    "field_floor": "/Game/World/Environment/Materials/M_Floor_Field_Default",
    "python_floor": "/Game/World/Environment/Materials/M_Floor_Boss_Python",
    "vritra_floor": "/Game/World/Environment/Materials/M_Floor_Boss_Camel",
    "cmd_floor": "/Game/World/Environment/Materials/M_Floor_Boss_CMD",
    "field_wall": "/Game/World/Environment/Materials/M_Wall_Corridor_Default",
    "cmd_wall": "/Game/World/Environment/Materials/M_Wall_Boss_CMD",
}

# These lanes must stay open for the player, boss spawns, and arena entrances.
# ROLE_GROUND actors are the walkable route itself; only ROLE_OBSTACLE actors are
# rejected from these volumes.
MAIN_LANE = (500.0, 13600.0, 850.0)
APPROACH_LANES = [
    (1820.0, 550.0, 5200.0, 720.0),
    (1820.0, -5200.0, -550.0, 720.0),
    (7020.0, 550.0, 5200.0, 720.0),
]
ARENA_CENTERS = [
    (4300.0, -5200.0),
    (4300.0, 5200.0),
    (9500.0, 5200.0),
    (14800.0, 0.0),
]
ARENA_COMBAT_CLEAR_HALF_EXTENT = (1320.0, 1120.0)
# BuildDemoRuntimeField uses a 520 x 900 trigger box; add 100 cm clearance.
ARENA_ENTRY_CLEAR_HALF_EXTENT = (620.0, 1000.0)
ARENA_EXIT_CLEAR_RADIUS = 620.0

APPROACH_ROUTE_DATA = [
    ("Vritra", 1820.0, -1.0, "vritra_floor", 2000.0),
    ("Python", 1820.0, 1.0, "python_floor", 2000.0),
    ("Selvara", 7020.0, 1.0, "python_floor", 7200.0),
]


def log(message):
    unreal.log(f"[ImproveRuntimeFieldEnvironment] {message}")


def get_editor_world():
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor_subsystem.get_editor_world() if editor_subsystem else None
    if not world:
        raise RuntimeError("No editor world is open.")
    return world


def get_world_package_name(world):
    return world.get_path_name().split(".", 1)[0]


def check_target_map():
    world = get_editor_world()
    current_map = get_world_package_name(world)
    if current_map != MAP_PATH:
        raise RuntimeError(
            f"Open {MAP_PATH} before running this script. Current map: {current_map}"
        )

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem:
        raise RuntimeError("LevelEditorSubsystem is unavailable.")
    current_level = level_subsystem.get_current_level()
    current_level_package = get_world_package_name(current_level) if current_level else "<none>"
    if current_level_package != MAP_PATH:
        raise RuntimeError(
            f"Make {MAP_PATH} the current level before running this script. "
            f"Current level: {current_level_package}"
        )
    return world, current_level


def load_required_assets():
    meshes = {}
    materials = {}

    for key, path in MESH_PATHS.items():
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.StaticMesh):
            raise RuntimeError(f"Missing StaticMesh: {path}")
        meshes[key] = asset

    for key, path in MATERIAL_PATHS.items():
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.MaterialInterface):
            raise RuntimeError(f"Missing MaterialInterface: {path}")
        materials[key] = asset

    return meshes, materials


def make_spec(
    label,
    mesh,
    material,
    location,
    scale,
    rotation=(0.0, 0.0, 0.0),
    role=ROLE_OBSTACLE,
    folder="Ruins",
    route_id=None,
    route_order=None,
):
    if not label.startswith(LABEL_PREFIX):
        raise ValueError(f"Environment labels must start with {LABEL_PREFIX}: {label}")
    if role not in VALID_ROLES:
        raise ValueError(f"Unknown environment role on {label}: {role}")
    return {
        "label": label,
        "mesh": mesh,
        "material": material,
        "location": location,
        "scale": scale,
        "rotation": rotation,
        "role": role,
        "collision": role in {ROLE_GROUND, ROLE_OBSTACLE},
        "folder": folder,
        "route_id": route_id,
        "route_order": route_order,
    }


def center_on_ground(x, y, ground_z, scale_z):
    # LevelPrototyping primitives are centered on their pivot and are about 100 cm tall.
    return (x, y, ground_z + scale_z * 50.0)


def add_main_road_surface(specs):
    """Bridge the prototype-floor gaps and climb gently into the CMD arena."""
    y_offsets = (-28.0, 34.0, -12.0, 21.0, -36.0, 18.0, 8.0)
    dark_segments = {2, 6, 9}
    for index in range(11):
        x = 1600.0 + index * 900.0
        y = y_offsets[index % len(y_offsets)]
        scale = (9.65 + (index % 3) * 0.18, 9.0 - (index % 2) * 0.25, 0.08)
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Ground_MainRoad_{index:02d}",
                "rock",
                "field_wall" if index in dark_segments else "field_floor",
                (x, y, 32.5 - scale[2] * 50.0),
                scale,
                (0.0, (-1.4, 0.8, 1.6, -0.6)[index % 4], 0.0),
                role=ROLE_GROUND,
                folder="Ground/MainRoad",
                route_id="MainRoad",
                route_order=index,
            )
        )

    # The original CMD room floor is 61 cm above the field. Eight overlapping
    # slabs keep every rise at or below 8 cm and hide the abrupt vertical lip.
    for stair_index in range(8):
        order = 11 + stair_index
        x = 11050.0 + stair_index * 240.0
        top_z = 34.0 + stair_index * 8.0
        scale = (2.8, 8.8 - (stair_index % 2) * 0.2, 0.18)
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Ground_MainRoad_{order:02d}",
                "rock",
                "cmd_wall" if stair_index in {2, 5} else "cmd_floor",
                (x, y_offsets[stair_index % len(y_offsets)] * 0.45, top_z - scale[2] * 50.0),
                scale,
                (0.0, (-0.8, 0.6)[stair_index % 2], 0.0),
                role=ROLE_GROUND,
                folder="Ground/CMDApproach",
                route_id="MainRoad",
                route_order=order,
            )
        )


def add_approach_surfaces(specs):
    """Bridge the three gaps from the main road to the elevated arena floors."""
    for name, route_x, y_side, material, bridge_x in APPROACH_ROUTE_DATA:
        route_id = f"Approach_{name}"
        for step in range(7):
            center_y = y_side * (850.0 + step * 550.0)
            top_z = 32.0 + step * 7.0
            scale = (8.6 + (step % 2) * 0.25, 5.8, 0.24)
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Ground_{name}_Route_{step:02d}",
                    "rock",
                    material,
                    (route_x, center_y, top_z - scale[2] * 50.0),
                    scale,
                    (0.0, y_side * (-1.2 + (step % 3) * 1.1), 0.0),
                    role=ROLE_GROUND,
                    folder=f"Ground/Approaches/{name}",
                    route_id=route_id,
                    route_order=step,
                )
            )

        landing_scale = (9.0, 6.5, 0.24)
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Ground_{name}_Route_07",
                "rock",
                material,
                (route_x, y_side * 4670.0, 82.0 - landing_scale[2] * 50.0),
                landing_scale,
                (0.0, y_side * 1.0, 0.0),
                role=ROLE_GROUND,
                folder=f"Ground/Approaches/{name}",
                route_id=route_id,
                route_order=7,
            )
        )

        bridge_scale = (7.2, 5.2, 0.24)
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Ground_{name}_Route_08",
                "rock",
                material,
                (bridge_x, y_side * 5150.0, 90.0 - bridge_scale[2] * 50.0),
                bridge_scale,
                (0.0, 0.0, 0.0),
                role=ROLE_GROUND,
                folder=f"Ground/Approaches/{name}",
                route_id=route_id,
                route_order=8,
            )
        )


def add_side_terrain(specs):
    """Build broad, asymmetric terrain shelves instead of rows of loose cubes."""
    stations = [
        (3900.0, 0.0),
        (5350.0, 190.0),
        (8450.0, 70.0),
        (11400.0, 230.0),
    ]
    for index, (base_x, extra_y) in enumerate(stations):
        for side_index, side in enumerate((-1.0, 1.0)):
            y = side * (1540.0 + extra_y + side_index * 45.0)
            base_scale = (
                7.4 + ((index + side_index) % 2) * 0.9,
                4.0 + (index % 3) * 0.35,
                1.15 + ((index * 2 + side_index) % 3) * 0.22,
            )
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Terrain_Shelf_{index:02d}_{side_index}_Base",
                    "rock",
                    "cmd_floor" if base_x > 10800.0 else "field_floor",
                    center_on_ground(base_x, y, 30.0, base_scale[2]),
                    base_scale,
                    (side * 2.5, 11.0 + index * 29.0 + side_index * 47.0, side * 2.0),
                    role=ROLE_OBSTACLE,
                    folder="Terrain/MainRoad",
                )
            )

            cap_scale = (
                4.3 + (side_index % 2) * 0.5,
                3.0 + (index % 2) * 0.35,
                2.0 + ((index + side_index) % 3) * 0.35,
            )
            cap_ground = 30.0 + base_scale[2] * 62.0
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Terrain_Shelf_{index:02d}_{side_index}_Crown",
                    "rock",
                    "cmd_wall" if base_x > 10800.0 else "field_wall",
                    center_on_ground(
                        base_x + side * (120.0 + index * 24.0),
                        y + side * 250.0,
                        cap_ground,
                        cap_scale[2],
                    ),
                    cap_scale,
                    (-4.0 + side_index * 7.0, 37.0 + index * 43.0, side * 3.5),
                    role=ROLE_OBSTACLE,
                    folder="Terrain/MainRoad",
                )
            )


def add_main_ruin_beats(specs):
    """Create four readable beats so the long road has a sense of progression."""
    beat_xs = [3450.0, 5900.0, 9650.0, 12450.0]
    for index, x in enumerate(beat_xs):
        for side_index, side in enumerate((-1.0, 1.0)):
            height = 5.8 + ((index + side_index) % 3) * 0.75
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Ruin_RoadBeat_{index:02d}_Pillar_{side_index}",
                    "column",
                    "cmd_wall" if index == len(beat_xs) - 1 else "field_wall",
                    center_on_ground(x + side * 85.0, side * 1280.0, 30.0, height),
                    (1.15, 1.15, height),
                    (0.0, index * 31.0 + side_index * 17.0, 0.0),
                    role=ROLE_OBSTACLE,
                    folder="Ruins/MainRoad",
                )
            )

        fallen_side = -1.0 if index % 2 == 0 else 1.0
        fallen_x = x - 240.0 if index == 1 else x + 210.0
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Ruin_RoadBeat_{index:02d}_Fallen",
                "column",
                "cmd_wall" if index == len(beat_xs) - 1 else "field_wall",
                (fallen_x, fallen_side * 1730.0, 135.0),
                (1.05, 1.05, 4.1),
                (77.0, 24.0 + index * 53.0, fallen_side * 7.0),
                role=ROLE_OBSTACLE,
                folder="Ruins/MainRoad",
            )
        )


def add_branch_landmarks(specs):
    """Frame each branch without putting collision inside the route corridor."""
    for route_index, (name, route_x, y_side, _, _) in enumerate(APPROACH_ROUTE_DATA):
        marker_y = y_side * 1060.0
        for side_index, x_side in enumerate((-1.0, 1.0)):
            pillar_x = route_x + x_side * 1020.0
            height = 7.0 + ((route_index + side_index) % 2) * 1.0
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Landmark_{name}_Pillar_{side_index}",
                    "column",
                    "field_wall",
                    center_on_ground(pillar_x, marker_y, 30.0, height),
                    (1.3, 1.3, height),
                    (0.0, route_index * 41.0 + side_index * 23.0, 0.0),
                    role=ROLE_OBSTACLE,
                    folder=f"Landmarks/{name}",
                )
            )
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Landmark_{name}_Crown_{side_index}",
                    "door_corner",
                    "field_wall",
                    (pillar_x, marker_y, 30.0 + height * 100.0),
                    (2.1, 2.1, 2.1),
                    (0.0, 90.0 * side_index + route_index * 19.0, 0.0),
                    role=ROLE_DETAIL,
                    folder=f"Landmarks/{name}",
                )
            )


def add_distant_silhouettes(specs):
    silhouette_xs = [2300.0, 5600.0, 8800.0, 11800.0]
    for index, x in enumerate(silhouette_xs):
        for side_index, side in enumerate((-1.0, 1.0)):
            height = 11.0 + ((index + side_index) % 3) * 2.8
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Silhouette_Main_{index:02d}_{side_index}",
                    "arch" if (index + side_index) % 2 else "column",
                    "cmd_wall" if x > 10800.0 else "field_wall",
                    center_on_ground(x + side * 280.0, side * 3500.0, 20.0, height),
                    (4.2 + index % 2, 4.2 + index % 2, height),
                    (0.0, 25.0 * index + 90.0 * side_index, 0.0),
                    role=ROLE_SILHOUETTE,
                    folder="Silhouette",
                )
            )


def add_approach_cliffs(specs):
    for name, path_x, y_side, material, _ in APPROACH_ROUTE_DATA:
        # Two staggered rows frame the climb without turning it into a rock tunnel.
        for row in range(2):
            y = y_side * (1450.0 + row * 930.0)
            for side_index, x_side in enumerate((-1.0, 1.0)):
                x = path_x + x_side * (1120.0 + (row % 2) * 180.0)
                scale = (4.2, 3.4, 4.8 + row * 1.2)
                specs.append(
                    make_spec(
                        f"{LABEL_PREFIX}Approach_{name}_Cliff_{row:02d}_{side_index}",
                        "rock",
                        material,
                        center_on_ground(x, y, 35.0, scale[2]),
                        scale,
                        (5.0 * row, 33.0 * row + 41.0 * side_index, 4.0 * x_side),
                        role=ROLE_OBSTACLE,
                        folder=f"Terrain/Approaches/{name}",
                    )
                )

        # Broken ramps sit outside the clear lane and suggest collapsed side routes.
        for side_index, x_side in enumerate((-1.0, 1.0)):
            specs.append(
                make_spec(
                    f"{LABEL_PREFIX}Approach_{name}_BrokenRamp_{side_index}",
                    "ramp",
                    material,
                    (path_x + x_side * 1250.0, y_side * 3550.0, 155.0),
                    (4.0, 2.6, 1.4),
                    (0.0, 90.0 if x_side < 0.0 else -90.0, 7.0 * y_side),
                    role=ROLE_OBSTACLE,
                    folder=f"Ruins/Approaches/{name}",
                )
            )


def add_arena_ruins(specs, name, center, floor_material, wall_material):
    center_x, center_y, floor_z = center
    ground_z = floor_z + 11.0

    corner_offsets = [
        (-1550.0, -1430.0),
        (-1550.0, 1430.0),
        (1660.0, -1430.0),
        (1660.0, 1430.0),
    ]
    for index, (offset_x, offset_y) in enumerate(corner_offsets):
        height = 5.0 + (index % 2) * 1.4
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_Column_{index:02d}",
                "column",
                wall_material,
                center_on_ground(center_x + offset_x, center_y + offset_y, ground_z, height),
                (1.35, 1.35, height),
                (0.0, 45.0 * index, 0.0),
                role=ROLE_OBSTACLE,
                folder=f"Arenas/{name}/Ruins",
            )
        )

    fallen_offsets = [(-650.0, -1510.0), (720.0, 1510.0)]
    for index, (offset_x, offset_y) in enumerate(fallen_offsets):
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_FallenColumn_{index:02d}",
                "column",
                wall_material,
                (center_x + offset_x, center_y + offset_y, ground_z + 75.0),
                (1.1, 1.1, 3.7),
                (79.0, 35.0 + index * 92.0, 6.0),
                role=ROLE_OBSTACLE,
                folder=f"Arenas/{name}/Ruins",
            )
        )

    rock_offsets = [
        (-1300.0, -2050.0),
        (-1300.0, 2050.0),
        (1300.0, -2050.0),
        (1300.0, 2050.0),
    ]
    for index, (offset_x, offset_y) in enumerate(rock_offsets):
        scale = (4.8 + index % 2, 3.8, 4.5 + (index + 1) % 3)
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_OuterRock_{index:02d}",
                "rock",
                floor_material,
                center_on_ground(center_x + offset_x, center_y + offset_y, ground_z, scale[2]),
                scale,
                (7.0 * index, 27.0 + 61.0 * index, -5.0 + 3.0 * index),
                role=ROLE_OBSTACLE,
                folder=f"Arenas/{name}/Terrain",
            )
        )

    for side_index, side in enumerate((-1.0, 1.0)):
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_FarArch_{side_index}",
                "arch",
                wall_material,
                center_on_ground(center_x + 2850.0, center_y + side * 1700.0, ground_z, 9.0),
                (5.0, 5.0, 9.0),
                (0.0, 90.0 + side * 20.0, 0.0),
                role=ROLE_SILHOUETTE,
                folder=f"Arenas/{name}/Silhouette",
            )
        )

        entry_height = 6.8 + side_index * 0.8
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_EntryPillar_{side_index}",
                "column",
                wall_material,
                center_on_ground(center_x - 1600.0, center_y + side * 1280.0, ground_z, entry_height),
                (1.35, 1.35, entry_height),
                (0.0, 24.0 + side_index * 67.0, 0.0),
                role=ROLE_OBSTACLE,
                folder=f"Arenas/{name}/Entrance",
            )
        )

    # Thin, non-colliding fragments break up the giant square floor while keeping
    # the boss movement and hit traces completely unobstructed.
    for quarter in (0, 2):
        specs.append(
            make_spec(
                f"{LABEL_PREFIX}Arena_{name}_FloorArc_{quarter:02d}",
                "arc",
                wall_material,
                center_on_ground(center_x, center_y, floor_z + 11.0, 0.02),
                (8.2, 8.2, 0.02),
                (0.0, quarter * 90.0, 0.0),
                role=ROLE_DETAIL,
                folder=f"Arenas/{name}/FloorDetail",
            )
        )

    specs.append(
        make_spec(
            f"{LABEL_PREFIX}Arena_{name}_FloorRing",
            "ring",
            floor_material,
            center_on_ground(center_x, center_y, floor_z + 11.0, 0.025),
            (15.0, 15.0, 0.025),
            (0.0, 45.0, 0.0),
            role=ROLE_DETAIL,
            folder=f"Arenas/{name}/FloorDetail",
        )
    )


def build_specs():
    specs = []
    add_main_road_surface(specs)
    add_approach_surfaces(specs)
    add_side_terrain(specs)
    add_main_ruin_beats(specs)
    add_branch_landmarks(specs)
    add_distant_silhouettes(specs)
    add_approach_cliffs(specs)

    arena_data = [
        ("Vritra", (4300.0, -5200.0, 80.0), "vritra_floor", "field_wall"),
        ("Python", (4300.0, 5200.0, 80.0), "python_floor", "field_wall"),
        ("Selvara", (9500.0, 5200.0, 80.0), "python_floor", "field_wall"),
        ("CMD", (14800.0, 0.0, 80.0), "cmd_floor", "cmd_wall"),
    ]
    for name, center, floor_material, wall_material in arena_data:
        add_arena_ruins(specs, name, center, floor_material, wall_material)

    validate_specs(specs)
    return specs


def estimate_footprint(spec):
    scale_x, scale_y, scale_z = spec["scale"]
    pitch, _, roll = spec["rotation"]
    if abs(pitch) > 0.001 or abs(roll) > 0.001:
        return max(scale_x, scale_y, scale_z) * 50.0
    return max(scale_x, scale_y) * 50.0


def is_inside_clear_lane(x, y, footprint):
    main_min_x, main_max_x, main_half_width = MAIN_LANE
    if main_min_x - footprint <= x <= main_max_x + footprint and abs(y) < main_half_width + footprint:
        return True

    for lane_x, min_y, max_y, half_width in APPROACH_LANES:
        low_y = min(min_y, max_y)
        high_y = max(min_y, max_y)
        if low_y - footprint <= y <= high_y + footprint and abs(x - lane_x) < half_width + footprint:
            return True

    for center_x, center_y in ARENA_CENTERS:
        delta_x = abs(x - center_x)
        delta_y = abs(y - center_y)
        combat_half_x, combat_half_y = ARENA_COMBAT_CLEAR_HALF_EXTENT
        if delta_x < combat_half_x + footprint and delta_y < combat_half_y + footprint:
            return True

        entry_x = center_x - 2480.0
        exit_x = center_x + 2500.0
        entry_half_x, entry_half_y = ARENA_ENTRY_CLEAR_HALF_EXTENT
        if (
            abs(x - entry_x) < entry_half_x + footprint
            and abs(y - center_y) < entry_half_y + footprint
        ):
            return True
        if (
            (x - exit_x) ** 2 + (y - center_y) ** 2
            < (ARENA_EXIT_CLEAR_RADIUS + footprint) ** 2
        ):
            return True

    return False


def get_surface_top(spec):
    return spec["location"][2] + spec["scale"][2] * 50.0


def validate_route_continuity(specs):
    routes = {}
    for spec in specs:
        route_id = spec["route_id"]
        if route_id:
            routes.setdefault(route_id, []).append(spec)

    expected_routes = {"MainRoad", *(f"Approach_{row[0]}" for row in APPROACH_ROUTE_DATA)}
    if set(routes) != expected_routes:
        raise RuntimeError(
            f"Approach route set mismatch. Expected {sorted(expected_routes)}, got {sorted(routes)}"
        )

    arena_targets = {
        "Approach_Vritra": (4300.0, -5200.0),
        "Approach_Python": (4300.0, 5200.0),
        "Approach_Selvara": (9500.0, 5200.0),
    }
    for route_id, route_specs in routes.items():
        ordered = sorted(route_specs, key=lambda item: item["route_order"])
        orders = [item["route_order"] for item in ordered]
        expected_count = 19 if route_id == "MainRoad" else 9
        if orders != list(range(expected_count)):
            raise RuntimeError(
                f"{route_id} must contain route orders 0 through {expected_count - 1}: {orders}"
            )

        if route_id == "MainRoad":
            first = ordered[0]
            last = ordered[-1]
            if first["location"][0] - first["scale"][0] * 50.0 > 1200.0:
                raise RuntimeError("MainRoad does not overlap the original field start.")
            if last["location"][0] + last["scale"][0] * 50.0 < 12550.0:
                raise RuntimeError("MainRoad does not overlap the CMD arena floor.")
            if abs(get_surface_top(last) - 91.0) > 2.0:
                raise RuntimeError("MainRoad does not meet the CMD arena floor height.")

            for previous, current in zip(ordered, ordered[1:]):
                center_gap = abs(current["location"][0] - previous["location"][0])
                half_lengths = (previous["scale"][0] + current["scale"][0]) * 50.0
                height_step = abs(get_surface_top(current) - get_surface_top(previous))
                if center_gap > half_lengths + 1.0:
                    raise RuntimeError(
                        f"Walkable route gap is too wide on MainRoad: "
                        f"{center_gap - half_lengths:.1f} cm"
                    )
                if height_step > 8.1:
                    raise RuntimeError(
                        f"Walkable route step is too high on MainRoad: {height_step:.1f} cm"
                    )
            continue

        first = ordered[0]
        first_half_length = first["scale"][1] * 50.0
        if abs(first["location"][1]) - first_half_length > 600.0:
            raise RuntimeError(f"{route_id} does not overlap the existing main road.")

        for previous, current in zip(ordered, ordered[1:]):
            previous_x, previous_y, _ = previous["location"]
            current_x, current_y, _ = current["location"]
            center_gap = math.hypot(current_x - previous_x, current_y - previous_y)
            height_step = abs(get_surface_top(current) - get_surface_top(previous))
            if center_gap > 575.0:
                raise RuntimeError(
                    f"Walkable route gap is too wide on {route_id}: {center_gap:.1f} cm"
                )
            if height_step > 8.1:
                raise RuntimeError(
                    f"Walkable route step is too high on {route_id}: {height_step:.1f} cm"
                )

        last = ordered[-1]
        arena_x, arena_y = arena_targets[route_id]
        last_x, last_y, _ = last["location"]
        last_half_x = last["scale"][0] * 50.0
        last_half_y = last["scale"][1] * 50.0
        arena_west_edge = arena_x - 2250.0
        if last_x + last_half_x < arena_west_edge or abs(last_y - arena_y) > last_half_y:
            raise RuntimeError(f"{route_id} does not overlap its arena floor.")
        if abs(get_surface_top(last) - 91.0) > 2.0:
            raise RuntimeError(f"{route_id} does not meet the arena floor height.")


def validate_specs(specs):
    if len(specs) > MAX_DESIGN_ACTORS:
        raise RuntimeError(
            f"Environment actor budget exceeded: {len(specs)} > {MAX_DESIGN_ACTORS}"
        )

    labels = set()
    for spec in specs:
        label = spec["label"]
        if label in labels:
            raise RuntimeError(f"Duplicate environment label: {label}")
        labels.add(label)

        if spec["mesh"] not in MESH_PATHS:
            raise RuntimeError(f"Unknown mesh key on {label}: {spec['mesh']}")
        if spec["material"] not in MATERIAL_PATHS:
            raise RuntimeError(f"Unknown material key on {label}: {spec['material']}")

        role = spec["role"]
        if role not in VALID_ROLES:
            raise RuntimeError(f"Unknown role on {label}: {role}")
        if spec["collision"] != (role in {ROLE_GROUND, ROLE_OBSTACLE}):
            raise RuntimeError(f"Collision does not match role on {label}: {role}")

        values = (*spec["location"], *spec["scale"], *spec["rotation"])
        if not all(math.isfinite(value) for value in values):
            raise RuntimeError(f"Non-finite transform value on {label}")
        if any(value <= 0.0 for value in spec["scale"]):
            raise RuntimeError(f"Non-positive scale on {label}: {spec['scale']}")
        if max(spec["scale"]) > 20.0:
            raise RuntimeError(f"Excessive scale on {label}: {spec['scale']}")
        if ".." in spec["folder"] or spec["folder"].startswith("/"):
            raise RuntimeError(f"Unsafe environment folder on {label}: {spec['folder']}")

        if role == ROLE_GROUND:
            pitch, _, roll = spec["rotation"]
            if spec["scale"][2] > 0.35 or abs(pitch) > 5.0 or abs(roll) > 5.0:
                raise RuntimeError(f"Unsafe walkable surface transform on {label}")
            if not spec["route_id"] or spec["route_order"] is None:
                raise RuntimeError(f"Walkable surface lacks route metadata: {label}")
        elif spec["route_id"] is not None or spec["route_order"] is not None:
            raise RuntimeError(f"Only walkable surfaces may have route metadata: {label}")

        if role == ROLE_OBSTACLE:
            x, y, _ = spec["location"]
            footprint = estimate_footprint(spec)
            if is_inside_clear_lane(x, y, footprint):
                raise RuntimeError(f"Obstacle enters a protected lane: {label}")

    validate_route_continuity(specs)


def collect_existing_environment(actor_subsystem, target_level):
    actors_by_label = {}
    for actor in list(actor_subsystem.get_all_level_actors()):
        label = actor.get_actor_label()
        if not label.startswith(LABEL_PREFIX):
            continue
        if actor.get_level() != target_level:
            raise RuntimeError(
                f"Refusing to touch {label}: it belongs to a different loaded level."
            )
        if label in actors_by_label:
            raise RuntimeError(f"Duplicate live environment label: {label}")
        if not isinstance(actor, unreal.StaticMeshActor):
            raise RuntimeError(f"Refusing to replace non-StaticMeshActor: {label}")
        actors_by_label[label] = actor
    return actors_by_label


def get_static_mesh_component(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        component = components[0] if components else None
    if not component:
        raise RuntimeError(f"StaticMeshComponent is missing on {actor.get_actor_label()}")
    return component


def configure_spec(actor, spec, meshes, materials):
    location = unreal.Vector(*spec["location"])
    pitch, yaw, roll = spec["rotation"]
    rotation = unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)
    actor.set_actor_label(spec["label"])
    actor.set_folder_path(unreal.Name(f"{FOLDER_ROOT}/{spec['folder']}"))
    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(rotation, False)
    actor.set_actor_scale3d(unreal.Vector(*spec["scale"]))

    component = get_static_mesh_component(actor)
    target_mesh = meshes[spec["mesh"]]
    component.set_static_mesh(target_mesh)
    assigned_mesh = component.get_editor_property("static_mesh")
    if not assigned_mesh or assigned_mesh.get_path_name() != target_mesh.get_path_name():
        raise RuntimeError(f"Failed to assign mesh on environment actor: {spec['label']}")
    component.set_material(0, materials[spec["material"]])
    component.set_mobility(unreal.ComponentMobility.STATIC)

    if spec["collision"]:
        component.set_collision_profile_name(BLOCK_ALL_PROFILE)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        actor.set_actor_enable_collision(True)
    else:
        component.set_collision_profile_name(NO_COLLISION_PROFILE)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_enable_collision(False)
    return actor


def spawn_or_update_spec(actor_subsystem, existing_by_label, spec, meshes, materials):
    actor = existing_by_label.pop(spec["label"], None)
    created = actor is None
    if created:
        actor = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor.static_class(),
            unreal.Vector(*spec["location"]),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0),
        )
        if not actor:
            raise RuntimeError(f"Failed to spawn environment actor: {spec['label']}")

    try:
        configure_spec(actor, spec, meshes, materials)
    except Exception:
        if created:
            actor_subsystem.destroy_actor(actor)
        raise
    return actor, created


def main():
    _, target_level = check_target_map()
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor_subsystem:
        raise RuntimeError("EditorActorSubsystem is unavailable.")
    meshes, materials = load_required_assets()
    specs = build_specs()

    existing_by_label = collect_existing_environment(actor_subsystem, target_level)
    created_actors = []
    updated_count = 0
    try:
        for spec in specs:
            actor, created = spawn_or_update_spec(
                actor_subsystem, existing_by_label, spec, meshes, materials
            )
            if created:
                created_actors.append(actor)
            else:
                updated_count += 1
    except Exception:
        for actor in created_actors:
            actor_subsystem.destroy_actor(actor)
        raise

    # Stale prefixed actors are removed only after every desired actor was
    # successfully created or updated. Non-prefixed level content is never touched.
    removed = 0
    for label, actor in existing_by_label.items():
        if not actor_subsystem.destroy_actor(actor):
            raise RuntimeError(f"Failed to remove stale environment actor: {label}")
        removed += 1

    # Re-check the live editor state before saving so no other current level is touched.
    check_target_map()

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem or not level_subsystem.save_current_level():
        raise RuntimeError(f"Failed to save current level: {MAP_PATH}")
    log(
        f"Reconciled {len(specs)} environment actors "
        f"({len(created_actors)} created, {updated_count} updated, {removed} stale removed), "
        f"and saved only {MAP_PATH}."
    )


main()
