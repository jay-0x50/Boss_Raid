import math

import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
CLEAN_BOULDER = "/Game/ThirdParty/BossEnvironment/PolyHavenLOD/Boulder01/boulder_01_LOD1"
TERRAIN_ASSETS = {
    "Field1": "/Game/World/Environment/Terrain/SM_OpenField_Field1",
    "Field2": "/Game/World/Environment/Terrain/SM_OpenField_Field2",
    "Field3": "/Game/World/Environment/Terrain/SM_OpenField_Field3",
}
ROUTE_POINTS = {
    "Field1": [
        (1200.0, 0.0), (2050.0, 250.0), (2520.0, 1050.0), (2920.0, 1880.0),
        (2600.0, 2750.0), (3040.0, 3650.0), (2420.0, 4480.0), (1820.0, 5200.0),
    ],
    "Field2": [
        (6460.0, 5200.0), (7140.0, 4240.0), (7300.0, 2750.0), (6620.0, 1120.0),
        (5480.0, -720.0), (4300.0, -2500.0), (3000.0, -4140.0), (1820.0, -5200.0),
    ],
    "Field3": [
        (6460.0, -5200.0), (7800.0, -4380.0), (9020.0, -3120.0),
        (10100.0, -1820.0), (11220.0, -760.0), (12320.0, 0.0),
    ],
}


def fail(message):
    raise RuntimeError(f"[AuditExplorationPass] {message}")


def log(message):
    unreal.log(f"[AuditExplorationPass] {message}")


def object_path(value):
    return value.get_path_name() if value else "None"


def distance_2d(a, b):
    la, lb = a.get_actor_location(), b.get_actor_location()
    return math.hypot(la.x - lb.x, la.y - lb.y)


def distance_to_route(route, location):
    nearest = float("inf")
    for start, end in zip(ROUTE_POINTS[route], ROUTE_POINTS[route][1:]):
        dx, dy = end[0] - start[0], end[1] - start[1]
        length_squared = dx * dx + dy * dy
        alpha = 0.0 if length_squared <= 1.0 else max(
            0.0,
            min(1.0, ((location.x - start[0]) * dx + (location.y - start[1]) * dy) / length_squared),
        )
        nearest_x, nearest_y = start[0] + dx * alpha, start[1] + dy * alpha
        nearest = min(nearest, math.hypot(location.x - nearest_x, location.y - nearest_y))
    return nearest


def main():
    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")

    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(subsystem.get_all_level_actors())
    by_label = {actor.get_actor_label(): actor for actor in actors}
    explore = [actor for actor in actors if actor.get_actor_label().startswith("Explore_")]
    terrain = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Terrain_")]
    rocks = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Rock_")]
    landmarks = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Landmark_")]
    seals = [actor for actor in explore if actor.get_actor_label().startswith("Explore_SQLSeal_")]
    lore = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Lore_")]
    encounters = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Encounter_")]
    spawners = [actor for actor in encounters if actor.get_actor_label().endswith("_Spawner")]
    gates = [actor for actor in explore if actor.get_class().get_name() == "BRStoryPathGate"]
    checkpoints = [actor for actor in explore if actor.get_actor_label().startswith("Explore_Checkpoint_")]
    map_fragments = [actor for actor in explore if actor.get_class().get_name() == "BRMapFragmentPickup"]

    expected = {
        "explore": 131, "terrain": 3, "rocks": 29, "landmarks": 25,
        "seals": 3, "lore": 5, "encounters": 49, "spawners": 7,
        "gates": 2, "checkpoints": 2, "map_fragments": 3,
    }
    actual = {
        "explore": len(explore), "terrain": len(terrain), "rocks": len(rocks), "landmarks": len(landmarks),
        "seals": len(seals), "lore": len(lore), "encounters": len(encounters), "spawners": len(spawners),
        "gates": len(gates), "checkpoints": len(checkpoints), "map_fragments": len(map_fragments),
    }
    if actual != expected:
        fail(f"Managed actor groups differ: actual={actual}, expected={expected}")

    lore_ids = []
    for actor in lore:
        expected_id = actor.get_actor_label()
        actual_id = str(actor.get_editor_property("beat_id"))
        if actual_id != expected_id:
            fail(f"Exploration lore has unstable beat id: {expected_id} -> {actual_id}")
        lore_ids.append(actual_id)
    if len(lore_ids) != len(set(lore_ids)):
        fail(f"Exploration lore beat ids are duplicated: {lore_ids}")

    terrain_counts = {route: sum(a.get_actor_label() == f"Explore_Terrain_{route}" for a in terrain)
                      for route in ("Field1", "Field2", "Field3")}
    rock_counts = {route: sum(a.get_actor_label().startswith(f"Explore_Rock_{route}_") for a in rocks)
                   for route in ("Field1", "Field2", "Field3")}
    if terrain_counts != {"Field1": 1, "Field2": 1, "Field3": 1}:
        fail(f"Unexpected open terrain actor counts: {terrain_counts}")
    if rock_counts != {"Field1": 9, "Field2": 11, "Field3": 9}:
        fail(f"Unexpected sparse outcrop counts: {rock_counts}")

    for label in (
        "Explore_Landmark_PythonRuin_Arch", "Explore_Landmark_PerlDescent_Arch",
        "Explore_Landmark_PerlArchive_Arch", "Explore_Landmark_RuntimeCollapse_Arch",
        "Explore_Landmark_CMDThreshold_Arch", "Explore_StoryGate_Python", "Explore_StoryGate_Vritra",
        "Explore_Checkpoint_Vritra", "Explore_Checkpoint_CMD",
        "Explore_MapFragment_Field1", "Explore_MapFragment_Field2", "Explore_MapFragment_Field3",
    ):
        if label not in by_label:
            fail(f"Missing route landmark: {label}")

    expected_fragments = {
        "Explore_MapFragment_Field1": ("Field1", (2360.0, 720.0)),
        "Explore_MapFragment_Field2": ("Field2", (6900.0, 4600.0)),
        "Explore_MapFragment_Field3": ("Field3", (7050.0, -4840.0)),
    }
    for label, (region_id, expected_xy) in expected_fragments.items():
        actor = by_label[label]
        if str(actor.get_editor_property("region_id")) != region_id:
            fail(f"Wrong map region id on {label}")
        loc = actor.get_actor_location()
        if math.hypot(loc.x - expected_xy[0], loc.y - expected_xy[1]) > 5.0:
            fail(f"Map fragment moved away from its intended route: {label}")

    arena_story = {
        "BossPlate_2_PythonArena": ("SerpentPython", [], False),
        "BossPlate_1_VritraArena": ("VritraPerl", ["SerpentPython"], True),
        "BossPlate_4_CMDArena": ("CMDFinal", ["SerpentPython", "VritraPerl"], True),
    }
    for label, (story_id, required, blocked) in arena_story.items():
        actor = by_label.get(label)
        if not actor:
            fail(f"Missing story arena: {label}")
        actual_id = str(actor.get_editor_property("boss_story_id"))
        actual_required = [str(value) for value in actor.get_editor_property("required_boss_story_ids")]
        actual_blocked = bool(actor.get_editor_property("block_arena_until_story_ready"))
        if (actual_id, actual_required, actual_blocked) != (story_id, required, blocked):
            fail(f"Wrong prerequisites on {label}: id={actual_id}, required={actual_required}, blocked={actual_blocked}")
        if label != "BossPlate_4_CMDArena" and actor.get_editor_property("gate_actor_to_hide_on_defeat"):
            fail(f"{label} still has an instant-hide legacy gate")

    python_arena = by_label["BossPlate_2_PythonArena"]
    vethara = by_label.get("Python_Vethara")
    aurathos = by_label.get("Python_Aurathos")
    coordinator = by_label.get("Python_TeamCoordinator")
    if not vethara or vethara.get_class().get_name() != "BP_VetharaBoss_C":
        fail("Python arena is missing the authored Vethara boss instance")
    if not aurathos or aurathos.get_class().get_name() != "BP_AurathosBoss_C":
        fail("Python arena is missing the authored Aurathos boss instance")
    if not coordinator or coordinator.get_class().get_name() != "BRBossTeamCoordinator":
        fail("Python arena is missing its team coordinator")
    team_members = list(coordinator.get_editor_property("team_members"))
    managed_bosses = list(python_arena.get_editor_property("boss_actors"))
    if team_members != [vethara, aurathos] or managed_bosses != [vethara, aurathos]:
        fail("Python bosses are not wired in stable Vethara/Aurathos order")
    if python_arena.get_editor_property("boss_class_to_spawn") or bool(
            python_arena.get_editor_property("spawn_boss_on_arena_start")):
        fail("Python arena still has the legacy single-boss spawn path enabled")
    if bool(coordinator.get_editor_property("allow_simultaneous_attacks")):
        fail("Python team coordinator allows simultaneous committed attacks")

    gate_setup = {
        "Explore_StoryGate_Python": "SerpentPython",
        "Explore_StoryGate_Vritra": "VritraPerl",
    }
    for label, required_boss in gate_setup.items():
        actor = by_label[label]
        pieces = list(actor.get_editor_property("gate_pieces"))
        if str(actor.get_editor_property("required_boss_id")) != required_boss:
            fail(f"Wrong required boss on {label}")
        if len(pieces) != 3 or not actor.get_editor_property("reveal_camera"):
            fail(f"{label} is missing gate pieces or reveal camera")
        if any(not piece.get_actor_enable_collision() for piece in pieces):
            fail(f"{label} has a non-blocking closed gate piece")
        for piece in pieces:
            component = piece.get_component_by_class(unreal.StaticMeshComponent)
            if not component or component.get_editor_property("mobility") != unreal.ComponentMobility.MOVABLE:
                fail(f"{label} has a gate piece that cannot animate without PIE mobility warnings")
        if actor.get_editor_property("reveal_camera").get_actor_location().z < 1200.0:
            fail(f"{label} reveal camera is low enough to clip arena rocks")
        timing = (float(actor.get_editor_property("open_duration")),
                  float(actor.get_editor_property("sink_distance")),
                  float(actor.get_editor_property("collision_release_alpha")))
        if any(abs(a - b) > 0.01 for a, b in zip(timing, (2.8, 450.0, 0.55))):
            fail(f"Wrong gate timing on {label}: {timing}")
        if not str(actor.get_editor_property("gate_open_line")):
            fail(f"Missing Nel line on {label}")

    selvara = by_label.get("BossPlate_3_SelvaraArena")
    if not selvara or selvara.get_editor_property("start_on_player_overlap"):
        fail("The unfinished Selvara/SQL arena is not sealed")

    expected_spawns = {
        "Explore_Encounter_Field1_A_Spawner": (3, 2), "Explore_Encounter_Field1_B_Spawner": (4, 2),
        "Explore_Encounter_Field2_A_Spawner": (4, 2), "Explore_Encounter_Field2_B_Spawner": (5, 3),
        "Explore_Encounter_Field2_C_Spawner": (4, 2), "Explore_Encounter_Field3_A_Spawner": (4, 2),
        "Explore_Encounter_Field3_B_Spawner": (5, 3),
    }
    for label, (count, alive) in expected_spawns.items():
        actor = by_label[label]
        values = (bool(actor.get_editor_property("should_spawn_enemies_immediately")),
                  bool(actor.get_editor_property("wait_for_player_when_auto_spawning")),
                  int(actor.get_editor_property("spawn_count")), int(actor.get_editor_property("max_alive_enemies")))
        if values != (True, True, count, alive):
            fail(f"Wrong finite proximity encounter setup on {label}: {values}")
        if float(actor.get_editor_property("auto_activation_distance")) > 1750.0:
            fail(f"{label} activates too far from the player")

    for label in ("Demo_Field_EnemySpawner_A", "Demo_Field_EnemySpawner_B", "Demo_Field_EnemySpawner_C", "Demo_Field_EnemySpawner_D"):
        actor = by_label.get(label)
        if actor and bool(actor.get_editor_property("should_spawn_enemies_immediately")):
            fail(f"Legacy spawner is still active: {label}")
    for label in ("Demo_Field_2_PythonArena_ExitGate", "Demo_Field_1_VritraArena_ExitGate"):
        actor = by_label.get(label)
        if actor and actor.get_actor_enable_collision():
            fail(f"Legacy instant gate still has collision: {label}")
    for actor in actors:
        label = actor.get_actor_label()
        if (label.startswith("Demo_Env_") or label.startswith("Demo_Field_MainPath_") or label.startswith("Demo_Field_Cover_")) \
                and actor.get_actor_enable_collision():
            fail(f"Prototype blockout actor still blocks the S route: {label}")

    legacy_corridors = [actor.get_actor_label() for actor in explore if actor.get_actor_label().startswith(
        ("Explore_Path_", "Explore_Cliff_"))]
    if legacy_corridors:
        fail(f"Maze-like path or cliff corridor actors remain: {legacy_corridors[:8]}")

    for route, expected_path in TERRAIN_ASSETS.items():
        actor = by_label[f"Explore_Terrain_{route}"]
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        terrain_mesh = component.get_editor_property("static_mesh") if component else None
        if object_path(terrain_mesh).split(".", 1)[0] != expected_path:
            fail(f"{route} is not using its rolling terrain mesh: {object_path(terrain_mesh)}")
        if not actor.get_actor_enable_collision():
            fail(f"{route} rolling terrain has no collision")
        tags = {str(tag) for tag in actor.get_editor_property("tags")}
        if not {"OpenWorldTerrain", "RollingSurface"}.issubset(tags):
            fail(f"{route} terrain is missing open-world surface tags: {tags}")
        bounds = terrain_mesh.get_bounding_box()
        spans = (bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z)
        if min(spans[0], spans[1]) < 4000.0 or spans[2] < 1100.0:
            fail(f"{route} terrain is too small or flat: spans={spans}")
        if route == "Field1" and bounds.max.y < 6000.0:
            fail(f"Field1 terrain was mirrored away from the Python approach: bounds={bounds}")
        if route == "Field3" and (bounds.min.y > -6500.0 or bounds.max.y > 3500.0):
            fail(f"Field3 terrain was mirrored away from the CMD approach: bounds={bounds}")
        body_setup = terrain_mesh.get_editor_property("body_setup")
        if body_setup.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
            fail(f"{route} terrain is not using complex surface collision")

    clean_mesh = unreal.load_asset(CLEAN_BOULDER)
    if not clean_mesh:
        fail(f"Missing clean boulder mesh: {CLEAN_BOULDER}")
    grounded_boulders = []
    for actor in explore:
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        used_mesh = component.get_editor_property("static_mesh") if component else None
        if used_mesh and "Mountainside" in used_mesh.get_path_name():
            fail(f"Ragged Mountainside mesh remains in managed world: {actor.get_actor_label()}")
        if used_mesh == clean_mesh:
            grounded_boulders.append(actor)

    if len(grounded_boulders) != 53:
        fail(f"Unexpected grounded boulder count: {len(grounded_boulders)}")
    for actor in grounded_boulders:
        tags = [str(tag) for tag in actor.get_editor_property("tags")]
        ground_tag = next((tag for tag in tags if tag.startswith("GroundZ_")), None)
        if "GroundedRock" not in tags or not ground_tag:
            fail(f"Boulder lacks grounding metadata: {actor.get_actor_label()}")
        ground_z = float(ground_tag.removeprefix("GroundZ_"))
        bounds_origin, bounds_extent = actor.get_actor_bounds(False)
        bottom_delta = bounds_origin.z - bounds_extent.z - ground_z
        if not -30.5 <= bottom_delta <= -12.0:
            fail(f"Floating or over-buried boulder: {actor.get_actor_label()} delta={bottom_delta:.1f}")
        if not actor.get_actor_enable_collision():
            fail(f"Grounded boulder has no collision: {actor.get_actor_label()}")

    nearest_outcrop = (float("inf"), "")
    for actor in rocks:
        route = actor.get_actor_label().split("_")[2]
        distance = distance_to_route(route, actor.get_actor_location())
        nearest_outcrop = min(nearest_outcrop, (distance, actor.get_actor_label()))
    if nearest_outcrop[0] < 800.0:
        fail(f"A sparse outcrop blocks the central traversal area: {nearest_outcrop}")

    hidden_positions = {
        "Story_HiddenFragment_2": (5230.0, -1620.0),
        "Story_HiddenFragment_3": (10050.0, -2580.0),
        "Demo_Field_HiddenWeaponAltar": (11750.0, -820.0),
    }
    for label, expected_xy in hidden_positions.items():
        actor = by_label.get(label)
        if not actor:
            fail(f"Missing hidden content: {label}")
        loc = actor.get_actor_location()
        if math.hypot(loc.x - expected_xy[0], loc.y - expected_xy[1]) > 5.0:
            fail(f"Hidden content was not moved onto its intended field: {label}")

    gate_class = unreal.load_class(None, "/Script/Exception.BRStoryPathGate")
    ending_class = unreal.load_class(None, "/Script/Exception.BREndingWidget")
    map_subsystem_class = unreal.load_class(None, "/Script/Exception.BRWorldMapSubsystem")
    map_widget_class = unreal.load_class(None, "/Script/Exception.BRWorldMapWidget")
    map_fragment_class = unreal.load_class(None, "/Script/Exception.BRMapFragmentPickup")
    save_game_class = unreal.load_class(None, "/Script/Exception.BRSaveGame")
    player_controller_class = unreal.load_class(None, "/Script/Exception.ExceptionPlayerController")
    gameplay_controller_class = unreal.load_class(None, "/Game/Blueprints/Core/BP_ExceptionPlayerController.BP_ExceptionPlayerController_C")
    if not all((gate_class, ending_class, map_subsystem_class, map_widget_class, map_fragment_class,
                save_game_class, player_controller_class, gameplay_controller_class)):
        fail("Story, ending, or world-map runtime class is unavailable")
    save_cdo = unreal.get_default_object(save_game_class)
    controller_cdo = unreal.get_default_object(player_controller_class)
    map_widget_cdo = unreal.get_default_object(map_widget_class)
    if int(save_cdo.get_editor_property("save_version")) != 4:
        fail("Save-game version was not advanced for map discovery and one-shot story data")
    if controller_cdo.get_editor_property("world_map_widget_class") != map_widget_class:
        fail("Player controller is not configured to create the world-map widget")
    mini_size = map_widget_cdo.get_editor_property("mini_map_frame_size")
    full_size = map_widget_cdo.get_editor_property("full_map_frame_size")
    world_span = map_widget_cdo.get_editor_property("mini_map_world_span")
    if (mini_size.x, mini_size.y) != (340.0, 260.0):
        fail(f"Unexpected top-right minimap size: {mini_size}")
    if (full_size.x, full_size.y) != (1180.0, 740.0):
        fail(f"Unexpected full-map size: {full_size}")
    if world_span.x < 4500.0 or world_span.y < 3500.0:
        fail(f"Minimap world span is too narrow for route reading: {world_span}")
    gameplay_controller_cdo = unreal.get_default_object(gameplay_controller_class)
    if gameplay_controller_cdo.get_editor_property("world_map_widget_class") != map_widget_class:
        fail("Gameplay controller Blueprint did not inherit the world-map widget class")

    log(f"PASS actors={len(actors)} managed={len(explore)} terrain={terrain_counts} rocks={rock_counts} "
        f"encounters=2/3/2 map_fragments=3 nearest_outcrop={nearest_outcrop[0]:.0f}cm "
        f"order=Python->Vritra->CMD")


main()
