import math
from pathlib import Path

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
PREFIX = "Explore_"

BOULDER = "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD1"
FORT = "/Game/ThirdParty/BossEnvironment/PolyHaven/SM_PH_ModularFort01"
FLOOR = "/Game/LevelPrototyping/Meshes/SM_ChamferCube"
GATE = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_Gate"
BAR_GATE = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/gate-metal-bars"
WALL = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallHalf"
CORNER = "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallCorner"

BOULDER_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Boulder01"
STONE_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_KD_Stone"
FORT_WALL_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Wall"
FORT_TRIM_MAT = "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Trim"
FIELD_MAT = "/Game/World/Environment/Materials/M_Floor_Field_Default"
PYTHON_MAT = "/Game/World/Environment/Materials/M_Floor_Boss_Python"
VRITRA_MAT = "/Game/World/Environment/Materials/M_Floor_Boss_Camel"
CMD_MAT = "/Game/World/Environment/Materials/M_Wall_Boss_CMD"

SPAWNER_CLASS_PATH = "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemySpawner.BP_CombatEnemySpawner_C"
ENEMY_CLASS_PATH = "/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemy.BP_CombatEnemy_C"
CHECKPOINT_CLASS_PATH = "/Game/Blueprints/World/BP_CheckpointBonfire.BP_CheckpointBonfire_C"
TERRAIN_DEST = "/Game/World/Environment/Terrain"
TERRAIN_ASSETS = {
    "Field1": f"{TERRAIN_DEST}/SM_OpenField_Field1",
    "Field2": f"{TERRAIN_DEST}/SM_OpenField_Field2",
    "Field3": f"{TERRAIN_DEST}/SM_OpenField_Field3",
}


# Arena positions stay intact. The route points now provide only progression and
# height anchors; each field is a broad, rolling landmass instead of a rock corridor.
ROUTES = {
    "Field1": {
        "points": [
            (1200.0, 0.0, 25.0), (2050.0, 250.0, 34.0), (2520.0, 1050.0, 48.0),
            (2920.0, 1880.0, 68.0), (2600.0, 2750.0, 88.0), (3040.0, 3650.0, 112.0),
            (2420.0, 4480.0, 136.0), (1820.0, 5200.0, 150.0),
        ],
        "material": PYTHON_MAT,
        "field_width": 2250.0,
        "phase": 0.35,
        "hills": [
            (650.0, 2200.0, 175.0, 920.0),
            (4150.0, 2050.0, 145.0, 850.0),
            (950.0, 4050.0, 125.0, 760.0),
        ],
    },
    "Field2": {
        "points": [
            (6460.0, 5200.0, 150.0), (7140.0, 4240.0, 162.0), (7300.0, 2750.0, 174.0),
            (6620.0, 1120.0, 184.0), (5480.0, -720.0, 188.0), (4300.0, -2500.0, 180.0),
            (3000.0, -4140.0, 164.0), (1820.0, -5200.0, 150.0),
        ],
        "material": VRITRA_MAT,
        "field_width": 2450.0,
        "phase": 1.7,
        "hills": [
            (8420.0, 3350.0, 190.0, 980.0),
            (7700.0, 420.0, 140.0, 820.0),
            (3700.0, -1120.0, 155.0, 900.0),
            (4550.0, -4150.0, 165.0, 930.0),
        ],
    },
    "Field3": {
        "points": [
            (6460.0, -5200.0, 150.0), (7800.0, -4380.0, 166.0), (9020.0, -3120.0, 182.0),
            (10100.0, -1820.0, 194.0), (11220.0, -760.0, 182.0), (12320.0, 0.0, 150.0),
        ],
        "material": CMD_MAT,
        "field_width": 2350.0,
        "phase": 3.1,
        "hills": [
            (7350.0, -6200.0, 150.0, 850.0),
            (7100.0, -3200.0, 145.0, 800.0),
            (9650.0, -4500.0, 195.0, 940.0),
            (10200.0, 100.0, 135.0, 820.0),
        ],
    },
}

ROCK_OUTCROPS = {
    "Field1": [
        (900.0, 1700.0), (1150.0, 2050.0), (620.0, 2300.0),
        (4200.0, 1700.0), (4440.0, 2100.0), (4280.0, 2500.0),
        (1260.0, 3660.0), (1130.0, 4020.0), (900.0, 4430.0),
    ],
    "Field2": [
        (5950.0, 4420.0), (6000.0, 4050.0), (5900.0, 3680.0),
        (8480.0, 3520.0), (8750.0, 3200.0),
        (7900.0, 700.0), (8160.0, 350.0),
        (4200.0, -900.0), (4000.0, -1210.0),
        (4400.0, -4000.0), (4700.0, -4260.0),
    ],
    "Field3": [
        (7200.0, -6100.0), (7600.0, -6250.0),
        (7000.0, -3500.0), (7200.0, -3150.0),
        (9300.0, -4500.0), (9700.0, -4550.0),
        (9900.0, -300.0), (10300.0, 200.0), (11600.0, -2200.0),
    ],
}

ENCOUNTERS = {
    "Field1_A": ((2530.0, 1260.0, 126.0), 1.0, 3, 2, "Python"),
    "Field1_B": ((2920.0, 3410.0, 144.0), -1.0, 4, 2, "Python"),
    "Field2_A": ((7000.0, 3520.0, 174.0), 1.0, 4, 2, "Perl"),
    "Field2_B": ((6100.0, 260.0, 198.0), -1.0, 5, 3, "Perl"),
    "Field2_C": ((3900.0, -2920.0, 190.0), 1.0, 4, 2, "Perl"),
    "Field3_A": ((7850.0, -4210.0, 182.0), -1.0, 4, 2, "CMD"),
    "Field3_B": ((10050.0, -2040.0, 206.0), 1.0, 5, 3, "CMD"),
}


def log(message):
    unreal.log(f"[BuildExplorationWorld] {message}")


def make_rotator(pitch=0.0, yaw=0.0, roll=0.0):
    rotation = unreal.Rotator()
    rotation.pitch = float(pitch)
    rotation.yaw = float(yaw)
    rotation.roll = float(roll)
    return rotation


def make_color(red, green, blue, alpha=255):
    return unreal.Color(b=int(blue), g=int(green), r=int(red), a=int(alpha))


def subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def actors():
    return list(subsystem().get_all_level_actors())


def find(label):
    return next((actor for actor in actors() if actor.get_actor_label() == label), None)


def asset(path, expected):
    value = unreal.load_asset(path)
    if not isinstance(value, expected):
        raise RuntimeError(f"Missing {expected.__name__}: {path}")
    return value


def actor_class(path):
    value = unreal.load_class(None, path)
    if not value:
        raise RuntimeError(f"Missing actor class: {path}")
    return value


def spawn(label, cls, location, rotation=(0.0, 0.0, 0.0), folder="World/Exploration"):
    actor = find(label)
    if actor and actor.get_class() != cls:
        subsystem().destroy_actor(actor)
        actor = None
    rot = make_rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2])
    if not actor:
        actor = subsystem().spawn_actor_from_class(cls, unreal.Vector(*location), rot)
    if not actor:
        raise RuntimeError(f"Could not spawn {label}")
    actor.modify()
    actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_actor_rotation(rot, False)
    actor.set_folder_path(unreal.Name(folder))
    return actor


def mesh(label, mesh_asset, material, location, scale, rotation=(0.0, 0.0, 0.0), collision=False,
         folder="World/Exploration", mobility=unreal.ComponentMobility.STATIC):
    actor = spawn(label, unreal.StaticMeshActor.static_class(), location, rotation, folder)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh_asset)
    if material:
        for index in range(max(1, component.get_num_materials())):
            component.set_material(index, material)
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    component.set_collision_profile_name(unreal.Name("BlockAll" if collision else "NoCollision"))
    component.set_mobility(mobility)
    actor.set_actor_enable_collision(collision)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def point_light(label, location, color, intensity=260.0, radius=820.0, folder="World/Exploration/Lights"):
    actor = spawn(label, unreal.PointLight.static_class(), location, folder=folder)
    light = actor.get_component_by_class(unreal.PointLightComponent)
    light.set_editor_property("light_color", color)
    light.set_editor_property("intensity", intensity)
    light.set_editor_property("attenuation_radius", radius)
    light.set_editor_property("cast_shadows", True)
    actor.set_actor_enable_collision(False)
    return actor


def segment_samples(points, spacing=560.0):
    for index in range(len(points) - 1):
        start, end = points[index], points[index + 1]
        dx, dy = end[0] - start[0], end[1] - start[1]
        distance = math.hypot(dx, dy)
        count = max(1, int(math.ceil(distance / spacing)))
        yaw = math.degrees(math.atan2(dy, dx))
        for step in range(count):
            alpha = step / count
            yield start[0] + dx * alpha, start[1] + dy * alpha, start[2] + (end[2] - start[2]) * alpha, yaw
    x, y, z = points[-1]
    previous = points[-2]
    yield x, y, z, math.degrees(math.atan2(y - previous[1], x - previous[0]))


def route_surface(route, x, y):
    data = ROUTES[route]
    nearest_distance = float("inf")
    base_z = data["points"][0][2]
    for start, end in zip(data["points"], data["points"][1:]):
        dx, dy = end[0] - start[0], end[1] - start[1]
        length_squared = dx * dx + dy * dy
        alpha = 0.0 if length_squared <= 1.0 else max(0.0, min(1.0, ((x - start[0]) * dx + (y - start[1]) * dy) / length_squared))
        nearest_x, nearest_y = start[0] + dx * alpha, start[1] + dy * alpha
        distance = math.hypot(x - nearest_x, y - nearest_y)
        if distance < nearest_distance:
            nearest_distance = distance
            base_z = start[2] + (end[2] - start[2]) * alpha

    phase = data["phase"]
    distance_blend = min(1.0, nearest_distance / 820.0)
    rolling = (
        math.sin(x / 1080.0 + phase) * 56.0
        + math.cos(y / 940.0 - phase * 0.7) * 42.0
        + math.sin((x + y) / 1550.0 + phase * 1.4) * 27.0
    ) * (0.18 + distance_blend * 0.82)
    edge_ratio = min(1.0, nearest_distance / data["field_width"])
    edge_lift = edge_ratio * edge_ratio * 72.0
    hill_height = 0.0
    for hill_x, hill_y, amplitude, radius in data["hills"]:
        distance_squared = (x - hill_x) ** 2 + (y - hill_y) ** 2
        hill_height += amplitude * math.exp(-distance_squared / (2.0 * radius * radius))
    return base_z - 12.0 + rolling + edge_lift + hill_height, nearest_distance


def terrain_contains(route, x, y):
    _height, distance = route_surface(route, x, y)
    data = ROUTES[route]
    width_wobble = 0.95 + 0.05 * math.sin((x - y) / 1750.0 + data["phase"])
    if distance > data["field_width"] * width_wobble:
        return False
    if route == "Field1":
        if y >= 4200.0:
            return x <= 2850.0
        if y <= 1000.0:
            return x <= 3500.0
        return x <= 4700.0
    if route == "Field2":
        if y >= 4200.0:
            return x >= 5900.0
        if y >= 1000.0:
            return 5000.0 <= x <= 9200.0
        if y <= -4300.0:
            return x <= 2850.0
        if y >= -1000.0:
            return 4000.0 <= x <= 8800.0
        if y >= -2600.0:
            return 3800.0 <= x <= 6200.0
        return x <= 6200.0
    if y <= -4300.0:
        return x >= 5900.0
    if y >= -1000.0:
        return x >= 9800.0
    return x >= 6800.0


def audit_field_separation():
    overlaps = []
    for x in range(-1200, 14801, 200):
        for y in range(-7600, 7601, 200):
            present = [route for route in ROUTES if terrain_contains(route, float(x), float(y))]
            if len(present) > 1:
                overlaps.append((x, y, present))
    if overlaps:
        raise RuntimeError(f"Open field surfaces overlap outside boss gates: {overlaps[:6]}")


def write_terrain_obj(route):
    data = ROUTES[route]
    output_dir = Path(unreal.Paths.project_saved_dir()) / "GeneratedTerrain"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / f"SM_OpenField_{route}.obj"
    grid_step = 300.0
    width = data["field_width"]
    min_x = math.floor((min(point[0] for point in data["points"]) - width) / grid_step) * grid_step
    max_x = math.ceil((max(point[0] for point in data["points"]) + width) / grid_step) * grid_step
    min_y = math.floor((min(point[1] for point in data["points"]) - width) / grid_step) * grid_step
    max_y = math.ceil((max(point[1] for point in data["points"]) + width) / grid_step) * grid_step
    columns = int(round((max_x - min_x) / grid_step)) + 1
    rows = int(round((max_y - min_y) / grid_step)) + 1

    vertices = []
    for row in range(rows):
        y = min_y + row * grid_step
        for column in range(columns):
            x = min_x + column * grid_step
            z, _distance = route_surface(route, x, y)
            vertices.append((x, y, z))

    def vertex_index(column, row):
        return row * columns + column

    top_faces = []
    for row in range(rows - 1):
        for column in range(columns - 1):
            center_x = min_x + (column + 0.5) * grid_step
            center_y = min_y + (row + 0.5) * grid_step
            if not terrain_contains(route, center_x, center_y):
                continue
            a = vertex_index(column, row)
            b = vertex_index(column + 1, row)
            c = vertex_index(column + 1, row + 1)
            d = vertex_index(column, row + 1)
            top_faces.extend(((a, b, c), (a, c, d)))

    edge_counts = {}
    for face in top_faces:
        for first, second in ((face[0], face[1]), (face[1], face[2]), (face[2], face[0])):
            key = (min(first, second), max(first, second))
            if key in edge_counts:
                edge_counts[key][0] += 1
            else:
                edge_counts[key] = [1, first, second]

    faces = list(top_faces)
    bottom_vertices = {}
    for count, first, second in edge_counts.values():
        if count != 1:
            continue
        for top_index in (first, second):
            if top_index not in bottom_vertices:
                x, y, z = vertices[top_index]
                bottom_vertices[top_index] = len(vertices)
                vertices.append((x, y, z - 1150.0))
        bottom_first, bottom_second = bottom_vertices[first], bottom_vertices[second]
        faces.extend(((first, bottom_second, second), (first, bottom_first, bottom_second)))

    lines = [f"o SM_OpenField_{route}"]
    # Unreal's OBJ importer converts from a right-handed source system by
    # mirroring Y. Pre-mirror the authored coordinates and reverse winding so
    # the imported mesh lands on the gameplay coordinates with upward normals.
    lines.extend(f"v {x:.3f} {-y:.3f} {z:.3f}" for x, y, z in vertices)
    lines.extend(f"vt {x / 620.0:.6f} {y / 620.0:.6f}" for x, y, _z in vertices)
    lines.extend(f"f {a + 1}/{a + 1} {c + 1}/{c + 1} {b + 1}/{b + 1}" for a, b, c in faces)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return output_path


def import_terrain_assets():
    audit_field_separation()
    unreal.EditorAssetLibrary.make_directory(TERRAIN_DEST)
    terrain_assets = {}
    for route in ROUTES:
        source_path = write_terrain_obj(route)
        asset_name = TERRAIN_ASSETS[route].rsplit("/", 1)[-1]
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        mesh_data = options.get_editor_property("static_mesh_import_data")
        mesh_data.set_editor_property("combine_meshes", True)
        mesh_data.set_editor_property("auto_generate_collision", False)
        mesh_data.set_editor_property("generate_lightmap_u_vs", True)
        mesh_data.set_editor_property("convert_scene", False)
        mesh_data.set_editor_property("convert_scene_unit", False)

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source_path))
        task.set_editor_property("destination_path", TERRAIN_DEST)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        task.set_editor_property("options", options)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        terrain_asset = unreal.load_asset(TERRAIN_ASSETS[route])
        if not isinstance(terrain_asset, unreal.StaticMesh):
            raise RuntimeError(f"Could not import rolling terrain asset: {TERRAIN_ASSETS[route]}")
        body_setup = terrain_asset.get_editor_property("body_setup")
        body_setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        terrain_asset.modify()
        unreal.EditorAssetLibrary.save_loaded_asset(terrain_asset)
        terrain_assets[route] = terrain_asset
    return terrain_assets


def build_open_terrain(loaded, terrain_assets):
    desired = []
    for route, terrain_asset in terrain_assets.items():
        label = f"{PREFIX}Terrain_{route}"
        desired.append(label)
        actor = mesh(label, terrain_asset, loaded[ROUTES[route]["material"]], (0.0, 0.0, 0.0), (1.0, 1.0, 1.0),
                     collision=True, folder=f"World/Exploration/Terrain/{route}")
        actor.set_editor_property("tags", [unreal.Name("OpenWorldTerrain"), unreal.Name("RollingSurface")])
    return desired


def grounded_mesh(label, mesh_asset, material, x, y, ground_z, scale, rotation=(0.0, 0.0, 0.0),
                  collision=True, folder="World/Exploration", embed=18.0):
    actor = mesh(label, mesh_asset, material, (x, y, ground_z), scale, rotation, collision, folder)
    bounds_origin, bounds_extent = actor.get_actor_bounds(False)
    bottom_z = bounds_origin.z - bounds_extent.z
    location = actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(location.x, location.y, location.z + ground_z - bottom_z - embed), False, False)
    actor.set_editor_property("tags", [unreal.Name("GroundedRock"), unreal.Name(f"GroundZ_{ground_z:.2f}")])
    return actor


def build_rock_outcrops(loaded):
    desired = []
    rock = loaded[BOULDER]
    rock_material = loaded[BOULDER_MAT]
    for route_index, (route, locations) in enumerate(ROCK_OUTCROPS.items()):
        for index, (x, y) in enumerate(locations):
            ground_z, _distance = route_surface(route, x, y)
            label = f"{PREFIX}Rock_{route}_{index:02d}"
            desired.append(label)
            scale = (
                1.85 + ((index * 7 + route_index) % 5) * 0.24,
                1.7 + ((index * 11 + route_index * 3) % 5) * 0.21,
                1.65 + ((index * 13 + route_index * 2) % 5) * 0.22,
            )
            grounded_mesh(label, rock, rock_material, x, y, ground_z, scale,
                          ((index % 3 - 1) * 4.0, 19.0 + index * 47.0 + route_index * 31.0, (index % 2 - 0.5) * 6.0),
                          True, f"World/Exploration/RockOutcrops/{route}", 22.0)
    return desired


def build_landmarks(loaded):
    desired = []
    fort, gate, wall, corner = loaded[FORT], loaded[GATE], loaded[WALL], loaded[CORNER]
    ruin_mat = loaded[BOULDER_MAT]
    landmarks = [
        ("PythonRuin", "Field1", (2680.0, 2540.0), 102.0, make_color(44, 138, 255)),
        ("PerlDescent", "Field2", (6720.0, 2100.0), -112.0, make_color(255, 135, 38)),
        ("PerlArchive", "Field2", (4700.0, -1820.0), -128.0, make_color(255, 105, 28)),
        ("RuntimeCollapse", "Field3", (9100.0, -3200.0), 46.0, make_color(255, 38, 82)),
        ("CMDThreshold", "Field3", (11340.0, -620.0), 35.0, make_color(255, 28, 68)),
    ]
    for name, route, (x, y), yaw, color in landmarks:
        z, _distance = route_surface(route, x, y)
        radians = math.radians(yaw)
        forward_x, forward_y = math.cos(radians), math.sin(radians)
        right_x, right_y = -forward_y, forward_x

        def offset_location(forward, right, height):
            return x + forward_x * forward + right_x * right, y + forward_y * forward + right_y * right, z + height

        specs = [
            ("Ruin", corner, ruin_mat, offset_location(90.0, 720.0, -25.0), (0.95, 0.95, 1.25), yaw + 35.0, True),
            ("Arch", gate, ruin_mat, (x, y, z + 55.0), (0.68, 0.68, 0.76), yaw, False),
            ("WallL", loaded[BOULDER], ruin_mat, offset_location(100.0, -540.0, -35.0), (2.7, 2.4, 2.1), yaw + 22.0, True),
            ("WallR", loaded[BOULDER], ruin_mat, offset_location(100.0, 540.0, -35.0), (2.5, 2.7, 2.0), yaw - 27.0, True),
        ]
        for suffix, mesh_asset, material, loc, scale, mesh_yaw, collision in specs:
            label = f"{PREFIX}Landmark_{name}_{suffix}"
            desired.append(label)
            if mesh_asset == loaded[BOULDER]:
                rock_ground, _distance = route_surface(route, loc[0], loc[1])
                grounded_mesh(label, mesh_asset, material, loc[0], loc[1], rock_ground, scale,
                              (0.0, mesh_yaw, 0.0), collision, f"World/Exploration/Landmarks/{name}", 24.0)
            else:
                mesh(label, mesh_asset, material, loc, scale, (0.0, mesh_yaw, 0.0), collision,
                     f"World/Exploration/Landmarks/{name}")
        light_label = f"{PREFIX}Landmark_{name}_Light"
        desired.append(light_label)
        point_light(light_label, (x, y, z + 260.0), color, folder=f"World/Exploration/Landmarks/{name}")

    for offset, suffix in ((-360.0, "L"), (0.0, "C"), (360.0, "R")):
        label = f"{PREFIX}SQLSeal_{suffix}"
        desired.append(label)
        mesh(label, loaded[BAR_GATE], loaded[CMD_MAT], (7020.0, 5200.0 + offset, 260.0),
             (1.0, 1.0, 1.25), collision=True, folder="World/Exploration/SealedSQL")
    return desired


def build_encounters(loaded):
    desired = []
    spawner_class = actor_class(SPAWNER_CLASS_PATH)
    enemy_class = actor_class(ENEMY_CLASS_PATH)
    theme_material = {"Python": loaded[PYTHON_MAT], "Perl": loaded[VRITRA_MAT], "CMD": loaded[CMD_MAT]}
    theme_color = {"Python": make_color(42, 142, 255), "Perl": make_color(255, 122, 35), "CMD": make_color(255, 32, 75)}
    for key, (anchor, side, count, alive, theme) in ENCOUNTERS.items():
        x, y, _authored_z = anchor
        route = key.split("_", 1)[0]
        z, _distance = route_surface(route, x, y)
        spawner_label = f"{PREFIX}Encounter_{key}_Spawner"
        desired.append(spawner_label)
        spawner_y = y + side * 180.0
        spawner_z, _distance = route_surface(route, x, spawner_y)
        spawner = spawn(spawner_label, spawner_class, (x, spawner_y, spawner_z + 12.0),
                        (0.0, -90.0 if side > 0 else 90.0, 0.0), f"World/Exploration/Encounters/{key}/Logic")
        for prop, value in (
            ("enemy_class", enemy_class), ("should_spawn_enemies_immediately", True), ("initial_spawn_delay", 0.6),
            ("wait_for_player_when_auto_spawning", True), ("auto_activation_distance", 1650.0),
            ("spawn_count", count), ("max_alive_enemies", alive), ("spawn_spread_radius", 110.0), ("respawn_delay", 1.8),
        ):
            spawner.set_editor_property(prop, value)

        outward = -90.0 if side > 0 else 90.0
        building_y = y + side * 390.0
        building_z, _distance = route_surface(route, x, building_y)
        door_y = y + side * 105.0
        door_z, _distance = route_surface(route, x, door_y)
        parts = [
            ("Shelter", loaded[FORT], loaded[BOULDER_MAT], (x, building_y, building_z - 92.0), (0.105, 0.105, 0.23), outward, False),
            ("Door", loaded[GATE], loaded[BOULDER_MAT], (x, door_y, door_z + 82.0), (0.78, 0.78, 0.88), outward, False),
            ("WingL", loaded[BOULDER], loaded[BOULDER_MAT], (x - 285.0, y + side * 225.0, z - 32.0), (2.4, 2.2, 2.0), outward + 24.0, True),
            ("WingR", loaded[BOULDER], loaded[BOULDER_MAT], (x + 285.0, y + side * 225.0, z - 32.0), (2.2, 2.5, 1.9), outward - 29.0, True),
            ("Threshold", loaded[FLOOR], theme_material[theme], (x, y + side * 50.0, z - 16.0), (4.6, 3.0, 0.28), outward, True),
        ]
        for suffix, mesh_asset, material, loc, scale, yaw, collision in parts:
            label = f"{PREFIX}Encounter_{key}_{suffix}"
            desired.append(label)
            if mesh_asset == loaded[BOULDER]:
                rock_ground, _distance = route_surface(route, loc[0], loc[1])
                grounded_mesh(label, mesh_asset, material, loc[0], loc[1], rock_ground, scale,
                              (0.0, yaw, 0.0), collision, f"World/Exploration/Encounters/{key}/Building", 20.0)
            else:
                mesh(label, mesh_asset, material, loc, scale, (0.0, yaw, 0.0), collision,
                     f"World/Exploration/Encounters/{key}/Building")
        light_label = f"{PREFIX}Encounter_{key}_Light"
        desired.append(light_label)
        point_light(light_label, (x, y + side * 210.0, z + 210.0), theme_color[theme], 260.0, 650.0,
                    f"World/Exploration/Encounters/{key}/Building")
    return desired


def look_at(source, target):
    return unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*source), unreal.Vector(*target))


def build_story_gate(loaded, key, location, route_yaw, boss_id, line, camera_location, camera_target):
    desired = []
    pieces = []
    radians = math.radians(route_yaw)
    right_x, right_y = -math.sin(radians), math.cos(radians)
    material = loaded[BOULDER_MAT]
    light_color = make_color(48, 145, 255) if boss_id == "SerpentPython" else make_color(255, 126, 35)
    for index, offset in enumerate((-300.0, 0.0, 300.0)):
        label = f"{PREFIX}StoryGate_{key}_Piece_{index}"
        desired.append(label)
        piece = mesh(label, loaded[BAR_GATE], material,
                     (location[0] + right_x * offset, location[1] + right_y * offset, location[2]),
                     (0.86, 0.86, 1.32), (0.0, route_yaw + 90.0, 0.0), True,
                     f"Story/World/Gates/{key}/Pieces", unreal.ComponentMobility.MOVABLE)
        pieces.append(piece)

    camera_label = f"{PREFIX}StoryGate_{key}_RevealCamera"
    desired.append(camera_label)
    camera = spawn(camera_label, unreal.CameraActor.static_class(), camera_location, folder=f"Story/World/Gates/{key}")
    camera.set_actor_rotation(look_at(camera_location, camera_target), False)
    camera.get_component_by_class(unreal.CameraComponent).set_editor_property("field_of_view", 62.0)

    gate_label = f"{PREFIX}StoryGate_{key}"
    desired.append(gate_label)
    gate_actor = spawn(gate_label, actor_class("/Script/Exception.BRStoryPathGate"), location,
                       (0.0, route_yaw, 0.0), f"Story/World/Gates/{key}")
    gate_actor.set_editor_property("required_boss_id", boss_id)
    gate_actor.set_editor_property("gate_pieces", pieces)
    gate_actor.set_editor_property("reveal_camera", camera)
    clean_gate_lines = {
        "Python": "Python 봉인이 끝났어. 아래쪽 Perl 레이어로 가는 길이 복구되고 있어.",
        "Vritra": "두 번째 봉인도 멎었어. 이제 최초의 명령이 있는 곳으로 내려갈 수 있어.",
    }
    gate_actor.set_editor_property("gate_open_line", clean_gate_lines.get(key, line))
    gate_actor.set_editor_property("open_duration", 2.8)
    gate_actor.set_editor_property("sink_distance", 450.0)
    gate_actor.set_editor_property("collision_release_alpha", 0.55)
    gate_actor.set_editor_property("play_reveal_on_unlock", True)
    light_label = f"{PREFIX}StoryGate_{key}_Light"
    desired.append(light_label)
    point_light(light_label, (location[0], location[1], location[2] + 190.0), light_color, 240.0, 560.0,
                f"Story/World/Gates/{key}")
    return desired


def build_story_gates(loaded):
    desired = []
    desired += build_story_gate(loaded, "Python", (6460.0, 5200.0, 270.0), -55.0, "SerpentPython",
                                "Python 봉인이 끝났어. 아래쪽 Perl 레이어로 가는 길이 복구되고 있어.",
                                (5200.0, 5200.0, 1500.0), (6460.0, 5200.0, 245.0))
    desired += build_story_gate(loaded, "Vritra", (6460.0, -5200.0, 270.0), 32.0, "VritraPerl",
                                "두 번째 봉인도 멎었어. 이제 최초의 명령이 있는 곳으로 내려갈 수 있어.",
                                (5200.0, -5200.0, 1500.0), (6460.0, -5200.0, 245.0))
    return desired


def build_checkpoints():
    desired = []
    checkpoint_class = actor_class(CHECKPOINT_CLASS_PATH)
    for key, (route, location, yaw) in {
        "Vritra": ("Field2", (2510.0, -4460.0), 43.0),
        "CMD": ("Field3", (11480.0, -280.0), 28.0),
    }.items():
        x, y = location
        z, _distance = route_surface(route, x, y)
        label = f"{PREFIX}Checkpoint_{key}"
        desired.append(label)
        checkpoint = spawn(label, checkpoint_class, (x, y, z + 8.0), (0.0, yaw, 0.0), "World/Exploration/Checkpoints")
        checkpoint.set_actor_scale3d(unreal.Vector(0.92, 0.92, 0.92))
    return desired


def build_lore():
    lore_class = actor_class("/Script/Exception.BRLoreLogTrigger")
    entries = [
        ("PythonTrail", "Field1", (3150.0, 2150.0), "PYTHON TRACE // SPLIT", "> TWO SIGNALS SHARE ONE SEAL\n> ONLY THEIR FALL RESTORES THE LOWER LAYER"),
        ("PerlDescent", "Field2", (6800.0, 1500.0), "PERL DESCENT // RECOVERING", "> PYTHON: TERMINATED\n> LOWER ROUTE: REBUILDING"),
        ("PerlArchive", "Field2", (5000.0, -1450.0), "PERL ARCHIVE // LAST LOOP", "> THE ROAD REPEATED UNTIL IT BECAME SAND\n> VRITRA KEPT WALKING"),
        ("RuntimeCollapse", "Field3", (9300.0, -2800.0), "RUNTIME // ROOT ROUTE", "> PYTHON: SEALED\n> PERL: SEALED\n> CMD: LISTENING"),
        ("SQLSeal", "Field2", (6500.0, 4680.0), "SQL ARCHIVE // QUARANTINED", "> OPTIONAL PROCESS INCOMPLETE\n> MAIN HANDLER ROUTE UNAFFECTED"),
    ]
    desired = []
    for key, route, location, title, text in entries:
        x, y = location
        z, _distance = route_surface(route, x, y)
        label = f"{PREFIX}Lore_{key}"
        desired.append(label)
        actor = spawn(label, lore_class, (x, y, z + 38.0), folder="Story/World/ExplorationLore")
        actor.set_editor_property("beat_id", unreal.Name(label))
        actor.set_editor_property("log_title", title)
        actor.set_editor_property("log_text", text)
        actor.set_editor_property("trigger_once", True)
    return desired


def build_map_fragments():
    fragment_class = actor_class("/Script/Exception.BRMapFragmentPickup")
    fragments = {
        "Field1": ((2360.0, 720.0), "Python Ruins", "This fragment remembers the road to Python. The blue route is visible now."),
        "Field2": ((6900.0, 4600.0), "Perl Descent", "The lower layer is mapped. The amber road bends toward Vritra."),
        "Field3": ((7050.0, -4840.0), "CMD Runtime", "The broken runtime is visible. Follow the red route to the first command."),
    }
    desired = []
    for region_id, (location, display_name, nel_line) in fragments.items():
        x, y = location
        z, _distance = route_surface(region_id, x, y)
        label = f"{PREFIX}MapFragment_{region_id}"
        desired.append(label)
        actor = spawn(label, fragment_class, (x, y, z + 62.0), folder="World/Exploration/MapFragments")
        actor.set_editor_property("region_id", region_id)
        actor.set_editor_property("region_display_name", display_name)
        actor.set_editor_property("unlock_line", nel_line)
    return desired


def configure_story_arenas():
    setup = {
        "BossPlate_2_PythonArena": ("SerpentPython", [], False, ""),
        "BossPlate_1_VritraArena": ("VritraPerl", ["SerpentPython"], True, "Python 봉인이 아직 살아 있어. 먼저 위쪽 폐허의 두 신호를 끝내야 해."),
        "BossPlate_4_CMDArena": ("CMDFinal", ["SerpentPython", "VritraPerl"], True, "아직 두 봉인이 모두 멎지 않았어. CMD는 지금 깨울 수 없어."),
    }
    setup = {
        "BossPlate_2_PythonArena": ("SerpentPython", [], False, ""),
        "BossPlate_1_VritraArena": (
            "VritraPerl",
            ["SerpentPython"],
            True,
            "Python 봉인이 아직 살아 있어. 먼저 위쪽에서 두 신호를 끝내야 해.",
        ),
        "BossPlate_4_CMDArena": (
            "CMDFinal",
            ["SerpentPython", "VritraPerl"],
            True,
            "아직 두 봉인이 모두 멎지 않았어. CMD는 지금 깨울 수 없어.",
        ),
    }
    for label, (story_id, required, blocked, locked_line) in setup.items():
        actor = find(label)
        if not actor:
            raise RuntimeError(f"Missing main story arena: {label}")
        actor.set_editor_property("boss_story_id", story_id)
        actor.set_editor_property("required_boss_story_ids", required)
        actor.set_editor_property("block_arena_until_story_ready", blocked)
        actor.set_editor_property("story_locked_line", locked_line)
        if label in ("BossPlate_2_PythonArena", "BossPlate_1_VritraArena"):
            actor.set_editor_property("gate_actor_to_hide_on_defeat", None)

    selvara = find("BossPlate_3_SelvaraArena")
    if selvara:
        selvara.set_editor_property("start_on_player_overlap", False)
        selvara.set_editor_property("block_arena_until_story_ready", False)
        selvara.set_folder_path(unreal.Name("World/Optional/SealedSQL"))


def configure_python_dual_boss():
    """Keep the Python arena wired to its two authored boss identities."""
    vethara = spawn(
        "Python_Vethara",
        actor_class("/Game/Blueprints/Bosses/BP_VetharaBoss.BP_VetharaBoss_C"),
        (4300.0, 4650.0, 260.0),
        (0.0, 180.0, 0.0),
        "Bosses/Python",
    )
    aurathos = spawn(
        "Python_Aurathos",
        actor_class("/Game/Blueprints/Bosses/BP_AurathosBoss.BP_AurathosBoss_C"),
        (4300.0, 5750.0, 260.0),
        (0.0, 180.0, 0.0),
        "Bosses/Python",
    )
    coordinator = spawn(
        "Python_TeamCoordinator",
        actor_class("/Script/Exception.BRBossTeamCoordinator"),
        (4300.0, 5200.0, 180.0),
        folder="Bosses/Python",
    )
    coordinator.set_editor_property("team_members", [vethara, aurathos])
    coordinator.set_editor_property("allow_simultaneous_attacks", False)
    coordinator.set_editor_property("team_attack_gap", 0.65)
    vethara.set_editor_property("team_coordinator", coordinator)
    aurathos.set_editor_property("team_coordinator", coordinator)
    vethara.set_editor_property("combat_ai_enabled", False)
    aurathos.set_editor_property("combat_ai_enabled", False)

    arena = find("BossPlate_2_PythonArena")
    if not arena:
        raise RuntimeError("Missing Python arena trigger: BossPlate_2_PythonArena")
    arena.set_editor_property("boss_actors", [vethara, aurathos])
    arena.set_editor_property("boss_class_to_spawn", None)
    arena.set_editor_property("spawn_boss_on_arena_start", False)
    arena.set_editor_property("reset_boss_on_enter", True)
    arena.set_editor_property("auto_include_team_members", True)
    arena.set_editor_property("auto_include_nearby_bosses", False)
    arena.set_editor_property("deactivate_unmanaged_bosses_on_start", True)
    arena.set_editor_property("boss_intro_delay", 1.25)
    arena.set_editor_property("hide_boss_status_until_intro_finished", True)


def retire_old_generated_content():
    retired = 0
    old_labels = {"Demo_Field_EnemySpawner_A", "Demo_Field_EnemySpawner_B", "Demo_Field_EnemySpawner_C",
                  "Demo_Field_EnemySpawner_D", "Demo_Field_2_PythonArena_ExitGate", "Demo_Field_1_VritraArena_ExitGate"}
    for actor in actors():
        label = actor.get_actor_label()
        if (label in old_labels or label.startswith("Demo_Env_") or
                label.startswith("Demo_Field_MainPath_") or label.startswith("Demo_Field_Cover_") or
                label.startswith("EncounterBuild_") or
                label.startswith("Story_Hill_") or label.startswith("Story_HillRamp_")):
            actor.modify()
            actor.set_actor_hidden_in_game(True)
            actor.set_actor_enable_collision(False)
            actor.set_actor_tick_enabled(False)
            if label.startswith("Demo_Field_EnemySpawner_"):
                actor.set_editor_property("should_spawn_enemies_immediately", False)
            retired += 1
    log(f"Retired {retired} prototype actors without deleting user content.")


def relocate_story_beats():
    locations = {
        "Story_HiddenFragment_1": ("Field1", 3460.0, 2660.0, 34.0),
        "Story_HiddenFragment_2": ("Field2", 5230.0, -1620.0, 34.0),
        "Story_HiddenFragment_3": ("Field3", 10050.0, -2580.0, 34.0),
        "Demo_Field_HiddenWeaponAltar": ("Field3", 11750.0, -820.0, 20.0),
        "Story_NelCompanion_PythonTrace": ("Field1", 2140.0, 4380.0, 8.0),
        "Story_NelCompanion_PerlSigil": ("Field2", 4680.0, -1780.0, 8.0),
        "Story_NelCompanion_RuntimeShard": ("Field3", 11250.0, -650.0, 8.0),
        "Story_Nel_PythonTrace": ("Field1", 2180.0, 4240.0, 8.0),
        "Story_Nel_PerlSigil": ("Field2", 4750.0, -1660.0, 8.0),
        "Story_Nel_RuntimeShard": ("Field3", 11080.0, -780.0, 8.0),
    }
    for label, (route, x, y, offset_z) in locations.items():
        actor = find(label)
        if actor:
            ground_z, _distance = route_surface(route, x, y)
            actor.set_actor_location(unreal.Vector(x, y, ground_z + offset_z), False, False)


def expand_sky_shell():
    sky = find("SM_SkySphere")
    if sky:
        sky.set_actor_scale3d(unreal.Vector(1000.0, 1000.0, 1000.0))
        sky.set_actor_enable_collision(False)


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")
    terrain_assets = import_terrain_assets()
    asset_paths = {
        BOULDER: unreal.StaticMesh, FORT: unreal.StaticMesh, FLOOR: unreal.StaticMesh, GATE: unreal.StaticMesh,
        BAR_GATE: unreal.StaticMesh, WALL: unreal.StaticMesh, CORNER: unreal.StaticMesh,
        BOULDER_MAT: unreal.MaterialInterface, STONE_MAT: unreal.MaterialInterface,
        FORT_WALL_MAT: unreal.MaterialInterface, FORT_TRIM_MAT: unreal.MaterialInterface,
        FIELD_MAT: unreal.MaterialInterface, PYTHON_MAT: unreal.MaterialInterface,
        VRITRA_MAT: unreal.MaterialInterface, CMD_MAT: unreal.MaterialInterface,
    }
    loaded = {path: asset(path, expected) for path, expected in asset_paths.items()}

    desired = set()
    desired.update(build_open_terrain(loaded, terrain_assets))
    desired.update(build_rock_outcrops(loaded))
    desired.update(build_landmarks(loaded))
    desired.update(build_encounters(loaded))
    desired.update(build_story_gates(loaded))
    desired.update(build_checkpoints())
    desired.update(build_lore())
    desired.update(build_map_fragments())
    configure_python_dual_boss()
    configure_story_arenas()
    retire_old_generated_content()
    relocate_story_beats()
    expand_sky_shell()

    for actor in actors():
        if actor.get_actor_label().startswith(PREFIX) and actor.get_actor_label() not in desired:
            subsystem().destroy_actor(actor)

    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
        raise RuntimeError(f"Could not save {MAP_PATH}")
    log(f"Saved {MAP_PATH} with {len(desired)} managed actors across three open rolling fields.")


main()
