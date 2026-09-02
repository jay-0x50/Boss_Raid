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

actors = unreal.EditorLevelLibrary.get_all_level_actors()
by_label = {actor.get_actor_label(): actor for actor in actors}

required = [
    "Demo_Field_PlayerStart",
    "Story_IntroDirector",
    "Story_IntroCamera_1",
    "Story_IntroCamera_2",
    "Story_IntroCamera_3",
    "Story_Cave_Floor",
    "Story_Cave_BackWall",
    "Story_Cave_Roof",
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

director = by_label["Story_IntroDirector"]
cameras = get_prop(director, "shot_cameras")
times = get_prop(director, "shot_times")
if len(cameras) != 3 or len(times) != 3 or any(camera is None for camera in cameras):
    fail(f"Intro shot setup is invalid: cameras={len(cameras)}, times={len(times)}")

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
