import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
PREFIX = "BossEnv_"
FOLDER_ROOT = "BossEnvironment"
NO_COLLISION = unreal.Name("NoCollision")
BLOCK_ALL = unreal.Name("BlockAll")

MESH = {
    # Poly Haven ships several FBX LOD objects in each source file. Importing
    # them as one combined mesh makes every LOD render at once, so the dressing
    # pass intentionally uses the individually imported LOD1 assets.
    "boulder": "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD1",
    # The mountainside scan has intentionally ragged photogrammetry borders
    # which read as torn sheets at this arena scale. Large, varied instances of
    # the clean boulder scan give the same natural silhouette without artifacts.
    "mountain": "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD1",
    "fort": "/Game/ThirdParty/BossEnvironment/PolyHaven/SM_PH_ModularFort01",
    "gate": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_Gate",
    "gate_bars": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/gate-metal-bars",
    "room_corner": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_RoomCorner",
    "wall": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_Wall",
    "wall_half": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallHalf",
    "wall_detail": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_WallDetail",
    "floor": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_FloorBig",
    "floor_detail": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_FloorDetail",
    "stairs": "/Game/ThirdParty/BossEnvironment/KenneyDungeon/SM_KD_StairsWide",
    "cube": "/Engine/BasicShapes/Cube.Cube",
}

MATERIAL = {
    "vritra_floor": "/Game/World/Environment/Materials/M_Floor_Boss_Camel",
    "python_floor": "/Game/World/Environment/Materials/M_Floor_Boss_Python",
    "selvara_floor": "/Game/World/Environment/Materials/M_Floor_Boss_Python",
    "cmd_floor": "/Game/World/Environment/Materials/M_Floor_Boss_CMD",
    "stone": "/Game/ThirdParty/BossEnvironment/Materials/M_KD_Stone",
    "boulder": "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Boulder01",
    "mountain": "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Boulder01",
    "fort_wall": "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Wall",
    "fort_trim": "/Game/ThirdParty/BossEnvironment/Materials/M_PH_Fort_Trim",
}

ARENAS = {
    "Vritra": {
        "center": (4300.0, -5200.0, 80.0),
        "floor": "vritra_floor",
        "light": (1.0, 0.19, 0.025),
        "mountain_angles": (-118.0, -68.0, -20.0, 28.0, 76.0, 122.0),
        "fort_scale": (0.56, 0.54, 0.62),
        "fort_yaw": 90.0,
    },
    "Python": {
        "center": (4300.0, 5200.0, 80.0),
        "floor": "python_floor",
        "light": (0.035, 0.22, 1.0),
        "mountain_angles": (-126.0, -75.0, -26.0, 22.0, 70.0, 116.0),
        "fort_scale": (0.50, 0.56, 0.60),
        "fort_yaw": -90.0,
    },
    "Selvara": {
        "center": (9500.0, 5200.0, 80.0),
        "floor": "selvara_floor",
        "light": (0.12, 1.0, 0.34),
        "mountain_angles": (-122.0, -72.0, -16.0, 34.0, 82.0, 128.0),
        "fort_scale": (0.55, 0.50, 0.64),
        "fort_yaw": 180.0,
    },
    "CMD": {
        "center": (14800.0, 0.0, 80.0),
        "floor": "cmd_floor",
        "light": (1.0, 0.018, 0.03),
        "mountain_angles": (-132.0, -86.0, -40.0, 8.0, 55.0, 102.0, 142.0),
    },
}

PYTHON_CENTER = ARENAS["Python"]["center"]
PYTHON_ENTRANCE = (1820.0, 5200.0)
PYTHON_EXIT = (6460.0, 5200.0)
PYTHON_PLAYER_STAGING = (2700.0, 5200.0)
PYTHON_BOSS_ANCHORS = {
    "Vethara": (4300.0, 4650.0),
    "Aurathos": (4300.0, 5750.0),
}
PYTHON_ROUTE_HALF_WIDTH = 700.0
PYTHON_BOSS_CLEARANCE = 1050.0
PYTHON_PLAYER_CLEARANCE = 800.0
PYTHON_REQUIRED_LABELS = {
    f"{PREFIX}Python_Entry_VetharaPier",
    f"{PREFIX}Python_Entry_AurathosPier",
    f"{PREFIX}Python_Exit_VetharaPier",
    f"{PREFIX}Python_Exit_AurathosPier",
    f"{PREFIX}Python_Vethara_KeepFragment",
    f"{PREFIX}Python_Vethara_BackArch",
    f"{PREFIX}Python_Vethara_KeyLight",
    f"{PREFIX}Python_Aurathos_KeepFragment",
    f"{PREFIX}Python_Aurathos_BackArch",
    f"{PREFIX}Python_Aurathos_KeyLight",
}
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_{side}_OuterBoulder_{index:02d}"
    for side in ("Vethara", "Aurathos")
    for index in range(4)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_Vethara_DataCrystal_{index:02d}" for index in range(3)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_Vethara_FrostTrace_{index:02d}" for index in range(3)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_Aurathos_HeatPlate_{index:02d}" for index in range(3)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_Aurathos_ErrorVein_{index:02d}" for index in range(3)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_CenterScar_{index:02d}" for index in range(3)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_Center{side}Trace_{index:02d}"
    for side in ("Cyan", "Gold")
    for index in range(2)
)
PYTHON_REQUIRED_LABELS.update(
    f"{PREFIX}Python_{side}_BrokenWall_{index:02d}"
    for side in ("Vethara", "Aurathos")
    for index in range(3)
)


def log(message):
    unreal.log(f"[BuildBossEnvironmentUpgrade] {message}")


def make_rotator(pitch=0.0, yaw=0.0, roll=0.0):
    rotation = unreal.Rotator()
    rotation.pitch = float(pitch)
    rotation.yaw = float(yaw)
    rotation.roll = float(roll)
    return rotation


def make_color(red, green, blue, alpha=255):
    return unreal.Color(b=int(blue), g=int(green), r=int(red), a=int(alpha))


def load_assets(paths, expected_type):
    result = {}
    for key, path in paths.items():
        asset = unreal.load_asset(path)
        if not isinstance(asset, expected_type):
            raise RuntimeError(f"Missing {expected_type.__name__}: {path}")
        result[key] = asset
    return result


def current_level():
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level = subsystem.get_current_level() if subsystem else None
    if not level:
        raise RuntimeError("No current editor level.")
    return level


def make_mesh_spec(label, arena, mesh, location, scale, rotation=(0.0, 0.0, 0.0), collision=True,
                   material=None, folder="Ruins", cast_shadow=True):
    return {
        "kind": "mesh",
        "label": f"{PREFIX}{arena}_{label}",
        "arena": arena,
        "mesh": mesh,
        "location": location,
        "scale": scale,
        "rotation": rotation,
        "collision": collision,
        "material": material,
        "folder": folder,
        "cast_shadow": cast_shadow,
    }


def make_light_spec(label, arena, location, color, intensity, radius, cast_shadows=False):
    return {
        "kind": "light",
        "label": f"{PREFIX}{arena}_{label}",
        "arena": arena,
        "location": location,
        "color": color,
        "intensity": intensity,
        "radius": radius,
        "folder": "Lighting",
        "cast_shadows": cast_shadows,
    }


def radial(center, radius, angle_degrees, z):
    angle = math.radians(angle_degrees)
    return (
        center[0] + math.cos(angle) * radius,
        center[1] + math.sin(angle) * radius,
        z,
    )


def elliptical(center, radius_x, radius_y, angle_degrees, z):
    angle = math.radians(angle_degrees)
    return (
        center[0] + math.cos(angle) * radius_x,
        center[1] + math.sin(angle) * radius_y,
        z,
    )


def distance_2d(location, anchor):
    return math.hypot(location[0] - anchor[0], location[1] - anchor[1])


def add_common_arena(specs, arena, data):
    cx, cy, cz = data["center"]

    # A ring of enlarged photogrammetry rocks masks the rectangular prototype
    # floor. The west side stays open for the existing approach and arena trigger.
    for index, angle in enumerate(data["mountain_angles"]):
        radius = 2240.0 + (index % 3) * 150.0
        scale = (
            8.2 + (index % 2) * 1.15,
            6.4 + ((index + 1) % 3) * 0.85,
            9.0 + (index % 3) * 1.2,
        )
        location = radial((cx, cy), radius, angle, cz - 5.0)
        specs.append(
            make_mesh_spec(
                f"Cliff_{index:02d}", arena, "mountain", location, scale,
                (0.0, angle + 92.0 + (index % 2) * 11.0, (-4.0, 3.0, 6.0)[index % 3]),
                collision=True, material="mountain", folder="Cliffs",
            )
        )

    boulder_angles = (-142.0, -92.0, -43.0, 7.0, 58.0, 112.0, 146.0)
    for index, angle in enumerate(boulder_angles):
        radius = 1720.0 + (index % 2) * 180.0
        scale = (4.4 + (index % 3) * 0.55, 4.8 + ((index + 1) % 2) * 0.65, 4.0 + ((index + 2) % 3) * 0.5)
        specs.append(
            make_mesh_spec(
                f"Boulder_{index:02d}", arena, "boulder", radial((cx, cy), radius, angle, cz), scale,
                (index * 9.0 - 18.0, angle * 0.7, 5.0 - index * 1.2),
                collision=True, material="boulder", folder="Cliffs",
            )
        )

    # The three early bosses use the same realistic fort kit with different
    # rotations and proportions. It blocks long views across the flat prototype
    # while leaving the existing combat logic and collision floor untouched.
    if arena != "CMD":
        specs.append(
            make_mesh_spec(
                "CitadelShell", arena, "fort", (cx + 180.0, cy, cz), data["fort_scale"],
                (0.0, data["fort_yaw"], 0.0), collision=False, folder="Keep",
            )
        )

    color = data["light"]
    specs.extend(
        [
            make_light_spec("KeyLight", arena, (cx + 780.0, cy - 960.0, cz + 430.0), color, 650.0, 2600.0),
            make_light_spec("RimLight", arena, (cx + 930.0, cy + 1050.0, cz + 360.0), color, 320.0, 1900.0),
        ]
    )


def add_python_dual_arena(specs, data):
    """Dress the Python room as an open, asymmetric twin-boss ruin.

    The west/east route remains a wide clear lane. Vethara owns the lower,
    cobalt-lit ruin and Aurathos the upper, gold-lit ruin; all collision stays
    on the oval perimeter and outside the authored boss spawn clearances.
    """
    cx, cy, cz = data["center"]

    perimeter = (
        ("Vethara_OuterBoulder_00", -118.0, 2200.0, 2100.0, (5.2, 5.8, 5.6), (-8.0, -34.0, 4.0)),
        ("Vethara_OuterBoulder_01", -91.0, 2260.0, 2000.0, (6.0, 5.1, 6.4), (6.0, -12.0, -5.0)),
        ("Vethara_OuterBoulder_02", -66.0, 2180.0, 2000.0, (5.5, 6.2, 5.8), (-4.0, 19.0, 7.0)),
        ("Vethara_OuterBoulder_03", -40.0, 2240.0, 2000.0, (6.1, 5.4, 6.2), (5.0, 48.0, -6.0)),
        ("Aurathos_OuterBoulder_00", 38.0, 2200.0, 2100.0, (5.8, 5.0, 6.5), (-6.0, 123.0, 5.0)),
        ("Aurathos_OuterBoulder_01", 68.0, 2250.0, 2050.0, (6.4, 5.6, 5.9), (4.0, 151.0, -7.0)),
        ("Aurathos_OuterBoulder_02", 94.0, 2190.0, 2000.0, (5.4, 6.3, 6.7), (-3.0, 188.0, 6.0)),
        ("Aurathos_OuterBoulder_03", 118.0, 2270.0, 2100.0, (6.2, 5.5, 6.1), (7.0, 222.0, -4.0)),
    )
    for label, angle, radius_x, radius_y, scale, rotation in perimeter:
        specs.append(
            make_mesh_spec(
                label,
                "Python",
                "boulder",
                elliptical((cx, cy), radius_x, radius_y, angle, cz - 8.0),
                scale,
                rotation,
                collision=True,
                material="boulder",
                folder="TwinRuins/Perimeter",
            )
        )

    # Uneven wall remnants complete the oval without reconstructing a box.
    boundary_ruins = (
        ("Vethara_BrokenWall_00", "wall_half", -122.0, 2050.0, 1850.0, (1.35, 0.82, 1.18), -29.0),
        ("Vethara_BrokenWall_01", "wall_detail", -88.0, 2040.0, 1850.0, (1.10, 0.78, 1.42), 7.0),
        ("Vethara_BrokenWall_02", "wall", -54.0, 2050.0, 1850.0, (1.48, 0.86, 1.05), 31.0),
        ("Aurathos_BrokenWall_00", "wall", 45.0, 2080.0, 1850.0, (1.22, 0.92, 1.38), 139.0),
        ("Aurathos_BrokenWall_01", "wall_half", 83.0, 2070.0, 1900.0, (1.62, 0.88, 1.16), 176.0),
        ("Aurathos_BrokenWall_02", "wall_detail", 126.0, 2050.0, 1850.0, (1.05, 0.84, 1.55), 218.0),
    )
    for label, mesh_name, angle, radius_x, radius_y, scale, yaw in boundary_ruins:
        specs.append(
            make_mesh_spec(
                label,
                "Python",
                mesh_name,
                elliptical((cx, cy), radius_x, radius_y, angle, cz),
                scale,
                (0.0, yaw, 0.0),
                collision=True,
                material="stone",
                folder="TwinRuins/Boundary",
            )
        )

    # Four small ruined piers frame, but do not narrow, the existing openings.
    # Their uneven offsets avoid a prototype symmetry and leave the entrance
    # trigger's full 1040 x 1800 cm collision volume unobstructed.
    piers = (
        ("Entry_VetharaPier", (2050.0, cy - 1320.0, cz), (0.48, 0.48, 0.82), 12.0),
        ("Entry_AurathosPier", (2090.0, cy + 1360.0, cz), (0.44, 0.44, 0.94), -17.0),
        ("Exit_VetharaPier", (6200.0, cy - 1060.0, cz), (0.46, 0.46, 0.88), -14.0),
        ("Exit_AurathosPier", (6230.0, cy + 1100.0, cz), (0.50, 0.50, 0.78), 19.0),
    )
    for label, location, scale, yaw in piers:
        specs.append(
            make_mesh_spec(
                label,
                "Python",
                "room_corner",
                location,
                scale,
                (0.0, yaw, 0.0),
                collision=True,
                material="stone",
                folder="TwinRuins/Thresholds",
            )
        )

    # The fort scan is retained only as two dark, non-colliding background
    # fragments. This removes the bright shell that previously filled the
    # combat center while preserving a strong ruined skyline.
    identity_ruins = (
        ("Vethara_KeepFragment", "fort", (cx + 500.0, cy - 2100.0, cz - 30.0),
         (0.24, 0.15, 0.36), (0.0, 18.0, 0.0), "fort_wall"),
        ("Vethara_BackArch", "gate", (cx - 230.0, cy - 1740.0, cz + 180.0),
         (1.45, 1.45, 1.55), (0.0, 3.0, 0.0), "fort_trim"),
        ("Aurathos_KeepFragment", "fort", (cx - 550.0, cy + 2150.0, cz - 20.0),
         (0.18, 0.24, 0.30), (0.0, -23.0, 0.0), "fort_wall"),
        ("Aurathos_BackArch", "gate", (cx + 280.0, cy + 1760.0, cz + 190.0),
         (1.65, 1.65, 1.72), (0.0, -6.0, 0.0), "fort_trim"),
    )
    for label, mesh_name, location, scale, rotation, material in identity_ruins:
        specs.append(
            make_mesh_spec(
                label,
                "Python",
                mesh_name,
                location,
                scale,
                rotation,
                collision=False,
                material=material,
                folder="TwinRuins/Identity",
            )
        )

    specs.extend(
        [
            make_light_spec("Vethara_KeyLight", "Python", (cx - 360.0, cy - 1240.0, cz + 470.0),
                            (0.0, 0.40, 1.0), 340.0, 1650.0, False),
            make_light_spec("Aurathos_KeyLight", "Python", (cx + 330.0, cy + 1270.0, cz + 490.0),
                            (1.0, 0.55, 0.0), 320.0, 1620.0, False),
        ]
    )

    # Vethara's side uses sparse, sharp cyan data shards and thin ground traces.
    # Everything inside the combat bowl is non-colliding and below the camera's
    # foreground line; the boss anchor and the preserved west/east lane stay clear.
    vethara_crystals = (
        ((3600.0, 3650.0, cz + 70.0), (0.30, 0.30, 1.55), -14.0),
        ((4480.0, 3500.0, cz + 62.0), (0.24, 0.28, 1.25), 9.0),
        ((5260.0, 3840.0, cz + 68.0), (0.28, 0.25, 1.42), 18.0),
    )
    for index, (location, scale, yaw) in enumerate(vethara_crystals):
        specs.append(
            make_mesh_spec(
                f"Vethara_DataCrystal_{index:02d}", "Python", "wall_detail", location, scale,
                (0.0, yaw, (-5.0, 4.0, -3.0)[index]), collision=False,
                material="python_floor", folder="TwinRuins/Vethara/DataCrystals",
            )
        )

    for index, (location, scale, yaw) in enumerate((
        ((3650.0, 4370.0, cz + 13.0), (3.6, 0.035, 0.018), -8.0),
        ((4420.0, 4180.0, cz + 13.0), (3.0, 0.032, 0.018), 11.0),
        ((5110.0, 4440.0, cz + 13.0), (2.7, 0.030, 0.018), -15.0),
    )):
        specs.append(
            make_mesh_spec(
                f"Vethara_FrostTrace_{index:02d}", "Python", "cube", location, scale,
                (0.0, yaw, 0.0), collision=False, material="python_floor",
                folder="TwinRuins/Vethara/FrostTraces", cast_shadow=False,
            )
        )

    # Aurathos is heavier: broad gold heat plates sit over narrow crimson
    # runtime veins. They are low enough to preserve both boss silhouettes.
    aurathos_plates = (
        ((3570.0, 6250.0, cz + 11.0), (3.1, 0.70, 0.025), 11.0),
        ((4420.0, 6460.0, cz + 11.0), (3.8, 0.82, 0.025), -8.0),
        ((5270.0, 6190.0, cz + 11.0), (2.8, 0.75, 0.025), 16.0),
    )
    for index, (location, scale, yaw) in enumerate(aurathos_plates):
        specs.append(
            make_mesh_spec(
                f"Aurathos_HeatPlate_{index:02d}", "Python", "cube", location, scale,
                (0.0, yaw, 0.0), collision=False, material="vritra_floor",
                folder="TwinRuins/Aurathos/HeatPlates", cast_shadow=False,
            )
        )
        specs.append(
            make_mesh_spec(
                f"Aurathos_ErrorVein_{index:02d}", "Python", "cube",
                (location[0] + 35.0, location[1] - 18.0, location[2] + 3.0),
                (scale[0] * 0.76, 0.045, 0.014), (0.0, yaw + 7.0, 0.0), collision=False,
                material="cmd_floor", folder="TwinRuins/Aurathos/ErrorVeins", cast_shadow=False,
            )
        )

    # A jagged, low central fault is the only emissive focal point in the bowl.
    # Cyan and gold traces stop short of each other instead of filling the floor.
    center_scar = (
        ((3550.0, cy - 8.0, cz + 12.0), (4.7, 0.12, 0.026), -4.0),
        ((4300.0, cy + 18.0, cz + 12.0), (3.8, 0.14, 0.026), 6.0),
        ((5050.0, cy - 14.0, cz + 12.0), (4.8, 0.11, 0.026), -5.0),
    )
    for index, (location, scale, yaw) in enumerate(center_scar):
        specs.append(
            make_mesh_spec(
                f"CenterScar_{index:02d}", "Python", "cube", location, scale,
                (0.0, yaw, 0.0), collision=False, material="cmd_floor",
                folder="TwinRuins/CenterFault", cast_shadow=False,
            )
        )
    for side, material, y_offset, yaw_sign in (
        ("Cyan", "python_floor", -72.0, -1.0),
        ("Gold", "vritra_floor", 72.0, 1.0),
    ):
        for index, x in enumerate((3820.0, 4780.0)):
            specs.append(
                make_mesh_spec(
                    f"Center{side}Trace_{index:02d}", "Python", "cube",
                    (x, cy + y_offset + index * 10.0 * yaw_sign, cz + 14.0),
                    (3.1, 0.030, 0.015), (0.0, yaw_sign * (5.0 + index * 4.0), 0.0),
                    collision=False, material=material, folder="TwinRuins/CenterFault", cast_shadow=False,
                )
            )


def validate_python_specs(specs):
    python_specs = [spec for spec in specs if spec["arena"] == "Python"]
    labels = {spec["label"] for spec in python_specs}
    missing = sorted(PYTHON_REQUIRED_LABELS - labels)
    if missing:
        raise RuntimeError(f"Python arena specification is missing required labels: {missing}")
    unexpected = sorted(labels - PYTHON_REQUIRED_LABELS)
    if unexpected:
        raise RuntimeError(f"Python arena specification has unmanaged labels: {unexpected}")

    by_label = {spec["label"]: spec for spec in python_specs}
    thresholds = (
        ("entrance", PYTHON_ENTRANCE, "Entry_VetharaPier", "Entry_AurathosPier"),
        ("exit", PYTHON_EXIT, "Exit_VetharaPier", "Exit_AurathosPier"),
    )
    for name, anchor, south_suffix, north_suffix in thresholds:
        south = by_label[f"{PREFIX}Python_{south_suffix}"]["location"]
        north = by_label[f"{PREFIX}Python_{north_suffix}"]["location"]
        if abs(((south[1] + north[1]) * 0.5) - anchor[1]) > 50.0:
            raise RuntimeError(f"Python {name} piers are not centered on the preserved route")
        if north[1] - south[1] < 2000.0:
            raise RuntimeError(f"Python {name} pier centers leave too little opening width")

    colliding = [spec for spec in python_specs if spec["kind"] == "mesh" and spec["collision"]]
    route_blockers = [
        spec["label"] for spec in colliding
        if abs(spec["location"][1] - PYTHON_CENTER[1]) < PYTHON_ROUTE_HALF_WIDTH
    ]
    if route_blockers:
        raise RuntimeError(f"Python entrance-to-exit route is blocked by: {route_blockers}")

    for boss_name, anchor in PYTHON_BOSS_ANCHORS.items():
        blockers = [
            spec["label"] for spec in colliding
            if distance_2d(spec["location"], anchor) < PYTHON_BOSS_CLEARANCE
        ]
        if blockers:
            raise RuntimeError(f"{boss_name} spawn clearance is blocked by: {blockers}")

    player_blockers = [
        spec["label"] for spec in colliding
        if distance_2d(spec["location"], PYTHON_PLAYER_STAGING) < PYTHON_PLAYER_CLEARANCE
    ]
    if player_blockers:
        raise RuntimeError(f"Python player staging clearance is blocked by: {player_blockers}")


def add_cmd_keep(specs, data):
    cx, cy, cz = data["center"]
    specs.append(
        make_mesh_spec(
            "AncientKeep", "CMD", "fort", (cx + 220.0, cy, cz), (0.64, 0.64, 0.72),
            (0.0, 90.0, 0.0), collision=False, folder="Keep",
        )
    )

    # The fort is the visual shell; these colliding ruins enforce the outer edge
    # while leaving the large inner courtyard free for CMD's patterns.
    for index, (angle, radius) in enumerate(((-105.0, 2040.0), (-55.0, 2180.0), (5.0, 2150.0), (58.0, 2200.0), (112.0, 2050.0))):
        specs.append(
            make_mesh_spec(
                f"KeepButtress_{index:02d}", "CMD", "room_corner", radial((cx, cy), radius, angle, cz),
                (1.15, 1.15, 1.55), (0.0, angle + 45.0, 0.0), collision=True,
                material="fort_wall", folder="Keep",
            )
        )

    specs.append(make_light_spec("ThroneLight", "CMD", (cx + 1540.0, cy, cz + 510.0), data["light"], 850.0, 2800.0))


def build_specs():
    specs = []
    for arena, data in ARENAS.items():
        if arena == "Python":
            add_python_dual_arena(specs, data)
        else:
            add_common_arena(specs, arena, data)
    add_cmd_keep(specs, ARENAS["CMD"])

    labels = [spec["label"] for spec in specs]
    if len(labels) != len(set(labels)):
        raise RuntimeError("Duplicate desired actor labels.")
    validate_python_specs(specs)
    return specs


def mesh_component(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError(f"StaticMeshActor has no mesh component: {actor.get_actor_label()}")
    return component


def configure_mesh_actor(actor, spec, meshes, materials):
    actor.modify()
    actor.set_actor_label(spec["label"])
    actor.set_folder_path(unreal.Name(f"{FOLDER_ROOT}/{spec['arena']}/{spec['folder']}"))
    actor.set_actor_location(unreal.Vector(*spec["location"]), False, False)
    pitch, yaw, roll = spec["rotation"]
    actor.set_actor_rotation(make_rotator(pitch=pitch, yaw=yaw, roll=roll), False)
    actor.set_actor_scale3d(unreal.Vector(*spec["scale"]))

    component = mesh_component(actor)
    component.set_static_mesh(meshes[spec["mesh"]])
    component.set_mobility(unreal.ComponentMobility.STATIC)
    component.set_editor_property("cast_shadow", spec.get("cast_shadow", True))
    if spec["material"]:
        material = materials[spec["material"]]
        for index in range(max(1, component.get_num_materials())):
            component.set_material(index, material)

    if spec["collision"]:
        component.set_collision_profile_name(BLOCK_ALL)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        actor.set_actor_enable_collision(True)
    else:
        component.set_collision_profile_name(NO_COLLISION)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_enable_collision(False)
    actor.set_actor_hidden_in_game(False)


def configure_light_actor(actor, spec):
    actor.modify()
    actor.set_actor_label(spec["label"])
    actor.set_folder_path(unreal.Name(f"{FOLDER_ROOT}/{spec['arena']}/{spec['folder']}"))
    actor.set_actor_location(unreal.Vector(*spec["location"]), False, False)
    actor.set_actor_enable_collision(False)
    component = actor.get_component_by_class(unreal.PointLightComponent)
    if not component:
        raise RuntimeError(f"PointLight has no component: {spec['label']}")
    component.set_mobility(unreal.ComponentMobility.STATIONARY)
    component.set_editor_property("intensity", spec["intensity"])
    component.set_editor_property("attenuation_radius", spec["radius"])
    component.set_editor_property("light_color", make_color(
        spec["color"][0] * 255.0, spec["color"][1] * 255.0, spec["color"][2] * 255.0
    ))
    component.set_editor_property("cast_shadows", spec.get("cast_shadows", False))


def reconcile_specs(actor_subsystem, target_level, specs, meshes, materials):
    current = {}
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label().startswith(PREFIX):
            current[actor.get_actor_label()] = actor

    created = []
    updated = 0
    try:
        for spec in specs:
            actor = current.pop(spec["label"], None)
            expected_class = unreal.StaticMeshActor if spec["kind"] == "mesh" else unreal.PointLight
            if actor and not isinstance(actor, expected_class):
                actor_subsystem.destroy_actor(actor)
                actor = None
            if not actor:
                actor = actor_subsystem.spawn_actor_from_class(
                    expected_class.static_class(), unreal.Vector(*spec["location"]),
                    make_rotator()
                )
                if not actor:
                    raise RuntimeError(f"Could not spawn {spec['label']}")
                created.append(actor)
            else:
                updated += 1

            if spec["kind"] == "mesh":
                configure_mesh_actor(actor, spec, meshes, materials)
            else:
                configure_light_actor(actor, spec)
    except Exception:
        for actor in created:
            actor_subsystem.destroy_actor(actor)
        raise

    for actor in current.values():
        actor_subsystem.destroy_actor(actor)
    return len(created), updated, len(current)


def replace_exit_gates(actors, meshes, materials):
    changed = 0
    for arena, data in ARENAS.items():
        expected_fragment = {
            "Vritra": "1_VritraArena",
            "Python": "2_PythonArena",
            "Selvara": "3_SelvaraArena",
            "CMD": "4_CMDArena",
        }[arena]
        gate = next((actor for actor in actors if actor.get_actor_label() == f"Demo_Field_{expected_fragment}_ExitGate"), None)
        if not gate:
            log(f"Exit gate not found for {arena}; kept arena logic unchanged.")
            continue
        cx, cy, cz = data["center"]
        gate.set_actor_location(unreal.Vector(cx + 2160.0, cy, cz), False, False)
        gate.set_actor_rotation(make_rotator(yaw=90.0), False)
        gate.set_actor_scale3d(unreal.Vector(3.15, 3.15, 3.15))
        component = mesh_component(gate)
        component.set_static_mesh(meshes["gate_bars"])
        for index in range(max(1, component.get_num_materials())):
            component.set_material(index, materials["stone"])
        story_gate = next(
            (actor for actor in actors if actor.get_actor_label() == f"Explore_StoryGate_{arena}"),
            None,
        )
        if story_gate:
            # BuildExplorationWorld owns the replacement progression gate. Keep
            # its retired legacy mesh non-blocking on every rerun so it cannot
            # become an invisible wall when this dressing pass runs last.
            component.set_collision_profile_name(NO_COLLISION)
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            gate.set_actor_enable_collision(False)
        else:
            component.set_collision_profile_name(BLOCK_ALL)
            component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
            gate.set_actor_enable_collision(True)
        changed += 1
    return changed


def remove_prototype_arena_shapes(actor_subsystem, actors, target_level):
    exact_fragments = ("Arena_Wall_N", "Arena_Wall_S", "Arena_Wall_E", "Arena_Wall_W_L", "Arena_Wall_W_R")
    removed = 0
    for actor in actors:
        label = actor.get_actor_label()
        is_old_box_wall = label.startswith("Demo_Field_") and any(fragment in label for fragment in exact_fragments)
        is_old_arena_primitive = label.startswith("Demo_Env_Arena_")
        is_old_debug_sign = label.startswith("Demo_Field_") and label.endswith("_Sign") and "Arena" in label
        if is_old_box_wall or is_old_arena_primitive or is_old_debug_sign:
            if actor_subsystem.destroy_actor(actor):
                removed += 1
    return removed


def tune_field_lighting(actors):
    """Soften the prototype's clipped whites and pitch-black cast shadows."""
    changed = 0
    for actor in actors:
        if isinstance(actor, unreal.DirectionalLight):
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            component.set_editor_property("intensity", 1.05)
            changed += 1
        elif isinstance(actor, unreal.SkyLight):
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            component.set_editor_property("intensity", 0.70)
            changed += 1
    return changed


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        raise RuntimeError(f"Could not load map: {MAP_PATH}")

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem or not actor_subsystem:
        raise RuntimeError("Required editor subsystem is unavailable.")

    target_level = current_level()
    meshes = load_assets(MESH, unreal.StaticMesh)
    materials = load_assets(MATERIAL, unreal.MaterialInterface)
    specs = build_specs()
    created, updated, stale = reconcile_specs(actor_subsystem, target_level, specs, meshes, materials)

    actors = list(actor_subsystem.get_all_level_actors())
    gates = replace_exit_gates(actors, meshes, materials)
    removed = remove_prototype_arena_shapes(actor_subsystem, actors, target_level)
    tuned_lights = tune_field_lighting(actors)

    if not level_subsystem.save_current_level():
        raise RuntimeError(f"Failed to save {MAP_PATH}")
    log(
        f"Saved {MAP_PATH}: {len(specs)} external environment actors "
        f"({created} created, {updated} updated, {stale} stale removed), "
        f"{gates} exit gates upgraded, {removed} prototype arena shapes removed, "
        f"{tuned_lights} field lights tuned."
    )


main()
