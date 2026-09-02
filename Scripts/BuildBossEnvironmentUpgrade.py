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


def log(message):
    unreal.log(f"[BuildBossEnvironmentUpgrade] {message}")


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


def make_mesh_spec(label, arena, mesh, location, scale, rotation=(0.0, 0.0, 0.0), collision=True, material=None, folder="Ruins"):
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
    }


def make_light_spec(label, arena, location, color, intensity, radius):
    return {
        "kind": "light",
        "label": f"{PREFIX}{arena}_{label}",
        "arena": arena,
        "location": location,
        "color": color,
        "intensity": intensity,
        "radius": radius,
        "folder": "Lighting",
    }


def radial(center, radius, angle_degrees, z):
    angle = math.radians(angle_degrees)
    return (
        center[0] + math.cos(angle) * radius,
        center[1] + math.sin(angle) * radius,
        z,
    )


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
            make_light_spec("KeyLight", arena, (cx + 780.0, cy - 960.0, cz + 430.0), color, 1450.0, 3000.0),
            make_light_spec("RimLight", arena, (cx + 930.0, cy + 1050.0, cz + 360.0), color, 850.0, 2400.0),
        ]
    )


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

    specs.append(make_light_spec("ThroneLight", "CMD", (cx + 1540.0, cy, cz + 510.0), data["light"], 1900.0, 3300.0))


def build_specs():
    specs = []
    for arena, data in ARENAS.items():
        add_common_arena(specs, arena, data)
    add_cmd_keep(specs, ARENAS["CMD"])

    labels = [spec["label"] for spec in specs]
    if len(labels) != len(set(labels)):
        raise RuntimeError("Duplicate desired actor labels.")
    return specs


def mesh_component(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError(f"StaticMeshActor has no mesh component: {actor.get_actor_label()}")
    return component


def configure_mesh_actor(actor, spec, meshes, materials):
    actor.set_actor_label(spec["label"])
    actor.set_folder_path(unreal.Name(f"{FOLDER_ROOT}/{spec['arena']}/{spec['folder']}"))
    actor.set_actor_location(unreal.Vector(*spec["location"]), False, False)
    pitch, yaw, roll = spec["rotation"]
    actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)
    actor.set_actor_scale3d(unreal.Vector(*spec["scale"]))

    component = mesh_component(actor)
    component.set_static_mesh(meshes[spec["mesh"]])
    component.set_mobility(unreal.ComponentMobility.STATIC)
    component.set_editor_property("cast_shadow", True)
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
    component.set_editor_property("light_color", unreal.Color(
        int(spec["color"][0] * 255.0), int(spec["color"][1] * 255.0), int(spec["color"][2] * 255.0), 255
    ))
    component.set_editor_property("cast_shadows", True)


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
                    expected_class.static_class(), unreal.Vector(*spec["location"]), unreal.Rotator()
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
        gate.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), False)
        gate.set_actor_scale3d(unreal.Vector(3.15, 3.15, 3.15))
        component = mesh_component(gate)
        component.set_static_mesh(meshes["gate_bars"])
        for index in range(max(1, component.get_num_materials())):
            component.set_material(index, materials["stone"])
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
            component.set_editor_property("intensity", 1.6)
            changed += 1
        elif isinstance(actor, unreal.SkyLight):
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            component.set_editor_property("intensity", 0.8)
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
