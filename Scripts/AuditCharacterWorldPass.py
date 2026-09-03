import math
import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
SPAWNER_LABELS = [f"Demo_Field_EnemySpawner_{key}" for key in "ABCD"]
EXPECTED_SPAWNER_LOCATIONS = {
    "A": (2400.0, 485.0, 120.0),
    "B": (3600.0, -485.0, 120.0),
    "C": (5350.0, 495.0, 120.0),
    "D": (9000.0, -495.0, 120.0),
}
EXPECTED_NEL_LOCATIONS = {
    "Story_NelCompanion_CaveExit": (1940.0, 420.0, 95.0),
    "Story_NelCompanion_FirstRest": (2280.0, 560.0, 95.0),
}


def fail(message):
    raise RuntimeError(f"[AuditCharacterWorldPass] {message}")


def main():
    required_assets = [
        "/Game/Characters/Exception/Materials/M_Hendel_Armor01",
        "/Game/Characters/Exception/Materials/M_Hendel_Armor02",
        "/Game/Characters/Exception/Materials/M_Nel_Ghost01",
        "/Game/Characters/Exception/Materials/M_Nel_Ghost02",
        "/Game/Characters/Exception/Materials/M_Nel_Robe",
    ]
    for path in required_assets:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            fail(f"Missing appearance asset: {path}")

    player_class = unreal.load_class(
        None,
        "/Game/Blueprints/Core/BP_ExceptionCharacter.BP_ExceptionCharacter_C",
    )
    if not player_class:
        fail("Could not load BP_ExceptionCharacter_C")
    player_cdo = unreal.get_default_object(player_class)
    combo_anims = player_cdo.get_editor_property("light_combo_anims")
    if len(combo_anims) != 3 or any(anim is None for anim in combo_anims):
        fail(f"Expected 3 light combo animations, found {len(combo_anims)}")
    if player_cdo.get_editor_property("heavy_alt_anim") is None:
        fail("Heavy alternate animation is not assigned")

    if not unreal.load_class(None, "/Script/Exception.BRNelCompanion"):
        fail("BRNelCompanion class is not available")

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(subsystem.get_all_level_actors())
    by_label = {actor.get_actor_label(): actor for actor in actors}

    nel = [actor for actor in actors if actor.get_actor_label().startswith("Story_NelCompanion_")]
    buildings = [actor for actor in actors if actor.get_actor_label().startswith("EncounterBuild_")]
    open_exploration = "Explore_Terrain_Field1" in by_label
    if len(nel) != 6:
        fail(f"Expected 6 Nel appearances, found {len(nel)}")
    if open_exploration and buildings:
        fail(f"Legacy x-axis EncounterBuild actors overlap open exploration: {len(buildings)}")
    if not open_exploration and len(buildings) != 28:
        fail(f"Expected 28 encounter building actors in the prototype layout, found {len(buildings)}")

    for label, expected in EXPECTED_NEL_LOCATIONS.items():
        companion = by_label.get(label)
        if not companion:
            fail(f"Missing Nel appearance: {label}")
        actual = companion.get_actor_location()
        if math.dist((actual.x, actual.y, actual.z), expected) > 2.0:
            fail(f"Nel appearance is off its story beat: {label} at {actual}")

    for label in SPAWNER_LABELS:
        spawner = by_label.get(label)
        if not spawner:
            fail(f"Missing spawner: {label}")
        if open_exploration:
            if bool(spawner.get_editor_property("should_spawn_enemies_immediately")):
                fail(f"Legacy spawner remains active after open exploration rebuild: {label}")
            continue
        key = label[-1]
        expected = EXPECTED_SPAWNER_LOCATIONS[key]
        actual = spawner.get_actor_location()
        if math.dist((actual.x, actual.y, actual.z), expected) > 2.0:
            fail(f"Spawner {key} is not inside its building doorway: {actual}")
        related = [actor for actor in buildings if actor.get_actor_label().startswith(f"EncounterBuild_{key}_")]
        if len(related) != 7:
            fail(f"Spawner {key} has {len(related)} building actors")
        closest = min(
            math.hypot(
                actor.get_actor_location().x - spawner.get_actor_location().x,
                actor.get_actor_location().y - spawner.get_actor_location().y,
            )
            for actor in related
        )
        if closest > 420.0:
            fail(f"Spawner {key} is not connected to its building")

    unreal.log(
        "[AuditCharacterWorldPass] PASS: 3-step light combo assets, 2 heavy variants, "
        f"{len(nel)} Nel appearances, legacy buildings={len(buildings)}, open_exploration={open_exploration}."
    )


main()
