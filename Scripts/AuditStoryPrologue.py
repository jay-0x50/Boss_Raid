import math
import unreal


MAP_PATH = "/Game/Maps/L_Runtime_Field"
PREFIX = "Story_"


def fail(message):
    raise RuntimeError(f"[AuditStoryPrologue] {message}")


def get_prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as exc:
        fail(f"Could not read {name} from {obj}: {exc}")


if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
    fail(f"Could not load {MAP_PATH}")

actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
by_label = {actor.get_actor_label(): actor for actor in actors}

required = [
    "Demo_Field_PlayerStart",
    "Story_IntroDirector",
    "Story_IntroCamera_1",
    "Story_IntroCamera_2",
    "Story_IntroCamera_3",
    "Demo_Field_CheckpointBonfire",
    "Demo_SecondPass_PostProcess",
    "Story_Cave_Floor",
    "Story_Cave_BackWall",
    "Story_Cave_Roof",
    "Story_AwakeningPlatform_Core",
    "Story_AwakeningPlatform_Ring_00",
    "Story_AwakeningPlatform_Ring_06",
    "Story_Checkpoint_Platform",
    "Story_Checkpoint_Terminal_00",
    "Story_Checkpoint_Terminal_01",
    "Story_CaveLight_Checkpoint",
    "Story_Lore_Awakening",
    "Story_Nel_CaveExit",
    "Story_Nel_FirstRest",
    "Story_Nel_PythonTrace",
    "Story_Nel_PerlSigil",
    "Story_Nel_RuntimeShard",
    "Story_HiddenFragment_1",
    "Story_HiddenFragment_2",
    "Story_HiddenFragment_3",
]

missing = [label for label in required if label not in by_label]
if missing:
    fail(f"Missing actors: {missing}")

story_actors = [actor for actor in actors if actor.get_actor_label().startswith(PREFIX)]
if len(story_actors) < 50:
    fail(f"Expected at least 50 story actors, found {len(story_actors)}")

player_start = by_label["Demo_Field_PlayerStart"]
start = player_start.get_actor_location()
if abs(start.x - 1200.0) > 1.0 or abs(start.y) > 1.0 or abs(start.z - 150.0) > 1.0:
    fail(f"Unexpected player start: {start}")
same_spawn_starts = []
for actor in actors:
    if not isinstance(actor, unreal.PlayerStart):
        continue
    location = actor.get_actor_location()
    if math.dist((location.x, location.y, location.z), (1200.0, 0.0, 150.0)) <= 10.0:
        same_spawn_starts.append(actor.get_actor_label())
if same_spawn_starts != ["Demo_Field_PlayerStart"]:
    fail(f"Duplicate PlayerStarts remain at the awakening spawn: {same_spawn_starts}")

route_beat_locations = {
    "Story_Nel_FirstRest": (2140.0, 400.0, 150.0),
    "Demo_Field_CheckpointBonfire": (2180.0, 520.0, 150.0),
    "Story_Lore_MemoryLeak": (2910.0, 1880.0, 150.0),
}
for label, expected in route_beat_locations.items():
    actual = by_label[label].get_actor_location()
    if math.dist((actual.x, actual.y, actual.z), expected) > 2.0:
        fail(f"Story beat is off the Field 0 route: {label} at {actual}")

legacy_checkpoint = by_label.get("BRCheckpoint")
if legacy_checkpoint and legacy_checkpoint.get_actor_location().z > -4000.0:
    fail(f"Legacy duplicate checkpoint is still reachable: {legacy_checkpoint.get_actor_location()}")

director = by_label["Story_IntroDirector"]
cameras = get_prop(director, "shot_cameras")
times = get_prop(director, "shot_times")
if len(cameras) != 3 or len(times) != 3 or any(camera is None for camera in cameras):
    fail(f"Intro shot setup is invalid: cameras={len(cameras)}, times={len(times)}")

one_shot_ids = []
for actor in story_actors:
    label = actor.get_actor_label()
    property_name = None
    if label == "Story_IntroDirector" or label.startswith("Story_Lore_") or label.startswith("Story_Nel_"):
        property_name = "beat_id"
    elif label.startswith("Story_HiddenFragment_"):
        property_name = "fragment_id"
    if property_name:
        persistent_id = str(get_prop(actor, property_name))
        if not persistent_id or persistent_id == "None":
            fail(f"Persistent one-shot id is None: {label}.{property_name}")
        one_shot_ids.append((label, persistent_id))

id_values = [persistent_id for _label, persistent_id in one_shot_ids]
if len(id_values) != len(set(id_values)):
    fail(f"Duplicate persistent one-shot ids: {one_shot_ids}")

expected_fragment_ids = {
    "Story_HiddenFragment_1": "HiddenFragment_Field1",
    "Story_HiddenFragment_2": "HiddenFragment_Field2",
    "Story_HiddenFragment_3": "HiddenFragment_Field3",
}
for label, expected_id in expected_fragment_ids.items():
    if str(get_prop(by_label[label], "fragment_id")) != expected_id:
        fail(f"Wrong fragment id on {label}: {get_prop(by_label[label], 'fragment_id')}")

camera_targets = (
    (1200.0, 0.0, 130.0),
    (2060.0, 0.0, 230.0),
    (1200.0, 0.0, 135.0),
)
for camera, target in zip(cameras, camera_targets):
    location = camera.get_actor_location()
    forward = camera.get_actor_forward_vector()
    dx, dy, dz = target[0] - location.x, target[1] - location.y, target[2] - location.z
    distance = math.sqrt(dx * dx + dy * dy + dz * dz)
    look_dot = (forward.x * dx + forward.y * dy + forward.z * dz) / max(distance, 1.0)
    if look_dot < 0.995:
        fail(f"Intro camera does not face its subject: {camera.get_actor_label()} dot={look_dot:.3f}")

for actor in story_actors:
    label = actor.get_actor_label()
    if label.startswith("Story_Cave_") or label.startswith("Story_Hill_"):
        rotation = actor.get_actor_rotation()
        if abs(rotation.pitch) > 15.0 or abs(rotation.roll) > 15.0:
            fail(f"Story mesh has yaw written into pitch/roll: {label} rotation={rotation}")

post_process_settings = get_prop(by_label["Demo_SecondPass_PostProcess"], "settings")
exposure_bias = float(get_prop(post_process_settings, "auto_exposure_bias"))
if not -1.0 <= exposure_bias <= 0.0:
    fail(f"Runtime field exposure crushes shadows or clips highlights: bias={exposure_bias}")
if get_prop(post_process_settings, "bloom_intensity") > 0.18:
    fail("Runtime field bloom clips the blue/gold emissive accents")

cave_lights = [
    actor for actor in story_actors
    if actor.get_actor_label().startswith("Story_CaveLight_")
]
shadowed_cave_lights = []
for actor in cave_lights:
    component = actor.get_component_by_class(unreal.PointLightComponent)
    if not component:
        fail(f"Cave light has no PointLightComponent: {actor.get_actor_label()}")
    if component.get_editor_property("cast_shadows"):
        shadowed_cave_lights.append(actor.get_actor_label())
if shadowed_cave_lights != ["Story_CaveLight_Wake"]:
    fail(f"Field0 accent shadow budget differs: {shadowed_cave_lights}")

ring = [actor for actor in story_actors if actor.get_actor_label().startswith("Story_AwakeningPlatform_Ring_")]
if len(ring) != 7 or any(actor.get_actor_enable_collision() for actor in ring):
    fail(f"Awakening execution ring is incomplete or blocks movement: count={len(ring)}")

for label in ("Story_Lore_Awakening", "Story_Lore_MemoryLeak", "Story_Lore_CMDApproach"):
    if get_prop(by_label[label], "log_text").is_empty():
        fail(f"Lore text is empty: {label}")

request_checks = {
    "Story_Nel_PythonTrace": "Nel_FindPythonTrace",
    "Story_Nel_PerlSigil": "Nel_DecodePerlSigil",
    "Story_Nel_RuntimeShard": "Nel_RecoverRuntimeShard",
}
for label, expected_id in request_checks.items():
    actor = by_label[label]
    if str(get_prop(actor, "request_id")) != expected_id:
        fail(f"Wrong request id on {label}: {get_prop(actor, 'request_id')}")
    if not get_prop(actor, "completes_request"):
        fail(f"Request completion is disabled on {label}")

boss_labels = [
    "BossPlate_1_VritraArena",
    "BossPlate_2_PythonArena",
    "BossPlate_4_CMDArena",
]
for label in boss_labels:
    actor = by_label.get(label)
    if not actor:
        fail(f"Missing boss arena: {label}")
    if get_prop(actor, "boss_intro_line").is_empty() or get_prop(actor, "boss_defeat_log").is_empty():
        fail(f"Story text is empty on boss arena: {label}")

# Hills must stay outside the 1,200 cm-wide main route.
for actor in story_actors:
    label = actor.get_actor_label()
    if label.startswith("Story_Hill_") and abs(actor.get_actor_location().y) < 700.0:
        fail(f"Hill blocks the main route: {label}")

unreal.log(
    f"[AuditStoryPrologue] PASS: {len(story_actors)} story actors, "
    f"3 intro shots, 3 hidden requests, 3 fragments, 3 boss story hooks."
)
