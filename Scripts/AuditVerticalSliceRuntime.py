"""Asynchronous, non-destructive PIE audit for the Runtime Necropolis slice.

Run this file in the Unreal Editor *before* starting an in-process PIE session
for ``L_Runtime_Field``.  The script registers a Slate post-tick callback and
waits for PIE, so the caller that starts PIE is not blocked by Python.

The audit deliberately does not award a PASS from asset presence alone.  It
observes the Python twins' runtime delegates, drives real Groggy/Execution and
player-death paths, performs three timer-based checkpoint respawns, and makes a
real save/open-level/load round trip.  A missing Python/reflection API is
reported as UNSUPPORTED, never silently treated as success.

Results are written incrementally to::

    Saved/QA/VerticalSliceRuntime_<timestamp>.json
    Saved/QA/VerticalSliceRuntime_<timestamp>.log
    Saved/QA/VerticalSliceRuntime_latest.json

The production ``SaveSlot_0.sav`` is backed up before PIE mutations and is
restored after PIE ends.  The round-trip uses a unique temporary QA slot which
is also removed during cleanup.  The audit never stops PIE from its Slate
callback.  Once runtime work is complete the report enters
``READY_FOR_EXTERNAL_STOP``; the operator must stop PIE externally, after
which the next Slate tick performs log auditing and save-file cleanup.
"""

from __future__ import annotations

import builtins
import hashlib
import json
import re
import shutil
import time
import traceback
from datetime import datetime, timezone
from pathlib import Path

import unreal


MAP_NAME = "L_Runtime_Field"
PYTHON_ARENA_LABEL = "BossPlate_2_PythonArena"
VETHARA_LABEL = "Python_Vethara"
AURATHOS_LABEL = "Python_Aurathos"
CHECKPOINT_LABEL = "Demo_Field_CheckpointBonfire"
DEFAULT_SAVE_SLOT = "SaveSlot_0"

WAIT_FOR_PIE_SECONDS = 150.0
MAX_ACTIVE_RUN_SECONDS = 180.0
STATE_TIMEOUT_SECONDS = 20.0
PATTERN_OBSERVE_SECONDS = 10.0
LOAD_TIMEOUT_SECONDS = 25.0

REQUIRED_RESULT_IDS = (
    "pie_runtime_map",
    "runtime_api_surface",
    "player_spawn_possession",
    "player_hud",
    "checkpoint_activation",
    "python_intro_camera_flow",
    "python_identity_configuration",
    "python_dual_boss_ai",
    "vethara_runtime_patterns",
    "aurathos_runtime_patterns",
    "groggy_execution",
    "boss_retry_reset",
    "checkpoint_respawn_three_cycles",
    "python_execution_victory",
    "boss_cleanup",
    "boss_autosave",
    "save_load_round_trip",
    "runtime_log_errors",
)


def _utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def _is_valid(obj):
    if obj is None:
        return False
    try:
        return bool(unreal.SystemLibrary.is_valid(obj))
    except Exception:
        try:
            return bool(obj)
        except Exception:
            return False


def _class_arg(class_or_type):
    if class_or_type is None:
        return None
    try:
        return class_or_type.static_class()
    except Exception:
        return class_or_type


def _load_exception_class(short_name):
    wrapper = getattr(unreal, short_name, None)
    if wrapper is not None:
        return wrapper
    try:
        return unreal.load_class(None, f"/Script/Exception.{short_name}")
    except Exception:
        return None


def _get_prop(obj, *names):
    last_error = None
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception as exc:
            last_error = exc
        try:
            return getattr(obj, name)
        except Exception as exc:
            last_error = exc
    raise AttributeError(f"Could not read any of {names!r} from {obj}: {last_error}")


def _try_prop(obj, *names, default=None):
    try:
        return _get_prop(obj, *names)
    except Exception:
        return default


def _actor_label(actor):
    if not _is_valid(actor):
        return "<invalid>"
    try:
        return str(actor.get_actor_label())
    except Exception:
        try:
            return str(actor.get_name())
        except Exception:
            return str(actor)


def _name_text(value):
    if value is None:
        return "None"
    # Python may stringify Unreal enums either as
    # ``BRBossAnimationStage.PATTERN_WINDUP`` or as the repr-like
    # ``<BRBossAnimationStage.PATTERN_WINDUP: 3>``.  Keep only the enum leaf;
    # the numeric suffix must not leak into tokens used by runtime checks.
    text = str(value).strip().strip("<>").strip()
    leaf = text.rsplit(".", 1)[-1]
    leaf = leaf.split(":", 1)[0]
    return leaf.strip().strip("<>").strip()


def _enum_token(value):
    return re.sub(r"[^A-Z0-9]", "", _name_text(value).upper())


def _bool_api(obj, python_name):
    """Read a reflected bool getter even when a b-prefixed property shadows it."""
    value = getattr(obj, python_name)
    return bool(value() if callable(value) else value)


def _object_path(obj):
    if not _is_valid(obj):
        return "<invalid>"
    for getter_name in ("get_path_name", "get_name"):
        try:
            return str(getattr(obj, getter_name)())
        except Exception:
            pass
    return str(obj)


def _game_paused(world):
    try:
        return bool(unreal.GameplayStatics.is_game_paused(world))
    except Exception:
        return None


def _vector_tuple(vector):
    return [round(float(vector.x), 3), round(float(vector.y), 3), round(float(vector.z), 3)]


def _distance(a, b):
    dx = float(a.x) - float(b.x)
    dy = float(a.y) - float(b.y)
    dz = float(a.z) - float(b.z)
    return (dx * dx + dy * dy + dz * dz) ** 0.5


def _transform_location(transform):
    for name in ("translation", "location"):
        try:
            value = getattr(transform, name)
            if value is not None:
                return value
        except Exception:
            pass
        try:
            value = transform.get_editor_property(name)
            if value is not None:
                return value
        except Exception:
            pass
    raise AttributeError(f"Could not read transform location from {transform}")


def _get_game_world():
    # GetPIEWorlds is preferred because GetGameWorld can briefly return the old
    # world during OpenLevel teardown.
    try:
        worlds = list(unreal.EditorLevelLibrary.get_pie_worlds(False))
        for world in worlds:
            if _is_valid(world):
                return world
    except Exception:
        pass

    try:
        subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        world = subsystem.get_game_world() if subsystem else None
        if _is_valid(world):
            return world
    except Exception:
        pass
    return None


def _all_actors(world, class_or_type):
    if not _is_valid(world) or class_or_type is None:
        return []
    try:
        return list(
            unreal.GameplayStatics.get_all_actors_of_class(
                world, _class_arg(class_or_type)
            )
        )
    except Exception:
        return []


def _find_actor(world, class_or_type, expected_label):
    actors = _all_actors(world, class_or_type)
    for actor in actors:
        if _actor_label(actor) == expected_label:
            return actor
    # PIE object names may have a generated suffix even when actor labels are
    # unavailable.  Only use this fallback for the exact expected token.
    for actor in actors:
        if expected_label in _actor_label(actor):
            return actor
    return None


class _SaveBlueprintLibraryAdapter:
    """World-bound facade for builds that omit SubsystemBlueprintLibrary.

    UE 5.8 can expose a project's GameInstanceSubsystem class and functions to
    Python while omitting ``unreal.SubsystemBlueprintLibrary`` itself.  The
    project's C++ Blueprint library reaches the same subsystem internally, so
    this adapter preserves real save/load behavior without pretending that a
    UObject subsystem pointer was obtained.
    """

    def __init__(self, world):
        self.world = world

    def save_current_game(self, slot_name, user_index):
        return unreal.BRSaveBlueprintLibrary.save_exception_game(
            self.world, slot_name, user_index
        )

    def load_game_from_slot_and_open_level(self, slot_name, user_index):
        return unreal.BRSaveBlueprintLibrary.load_exception_game_and_open_level(
            self.world, slot_name, user_index
        )

    def does_save_exist(self, slot_name, user_index):
        return unreal.BRSaveBlueprintLibrary.does_exception_save_exist(
            self.world, slot_name, user_index
        )

    def delete_save(self, slot_name, user_index):
        return unreal.BRSaveBlueprintLibrary.delete_exception_save(
            self.world, slot_name, user_index
        )


class VerticalSliceRuntimeAudit:
    def __init__(self):
        self.run_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.project_saved = Path(unreal.Paths.project_saved_dir())
        self.qa_dir = self.project_saved / "QA"
        self.qa_dir.mkdir(parents=True, exist_ok=True)
        self.json_path = self.qa_dir / f"VerticalSliceRuntime_{self.run_id}.json"
        self.log_path = self.qa_dir / f"VerticalSliceRuntime_{self.run_id}.log"
        self.latest_path = self.qa_dir / "VerticalSliceRuntime_latest.json"
        self.save_dir = self.project_saved / "SaveGames"
        self.default_save_path = self.save_dir / f"{DEFAULT_SAVE_SLOT}.sav"
        self.qa_slot = f"CodexVerticalSliceQA_{self.run_id}"
        self.qa_save_path = self.save_dir / f"{self.qa_slot}.sav"
        self.backup_path = self.qa_dir / "Backups" / f"{self.run_id}_{DEFAULT_SAVE_SLOT}.sav"

        self.created_at = _utc_now()
        self.arm_wall_time = time.monotonic()
        self.active_started_at = None
        self.state = "WAIT_FOR_PIE"
        self.state_started_at = self.arm_wall_time
        self.state_timeout = WAIT_FOR_PIE_SECONDS
        self.tick_handle = None
        self.finished = False
        self.end_requested = False
        self.results = {}
        self.timeline = []
        self.notes = []
        self.cleanup = {
            "default_save_arm_signature": self._save_signature(self.default_save_path),
            "default_save_originally_existed": None,
            "default_save_backup": str(self.backup_path),
            "default_save_backup_ready": False,
            "default_save_restore_needed": False,
            "default_save_restored": False,
            "temporary_save_removed": False,
            "external_stop_required": False,
            "external_stop_observed": False,
            "runtime_delegate_bindings_released": False,
            "runtime_references_released": False,
        }

        self.world = None
        self.player = None
        self.controller = None
        self.game_mode = None
        self.checkpoint = None
        self.arena = None
        self.vethara = None
        self.aurathos = None
        self.hidden_story = None
        self.save_subsystem = None
        self.checkpoint_location = None
        self.checkpoint_event_seen = False
        self.camera_samples = []
        self.delegate_callbacks = []
        self.delegate_bind_errors = []
        self.delegate_unbind_errors = []
        self.subsystem_lookup_errors = []
        self.pattern_events = {VETHARA_LABEL: [], AURATHOS_LABEL: []}
        self.pattern_hit_events = {VETHARA_LABEL: [], AURATHOS_LABEL: []}
        self.stage_events = {VETHARA_LABEL: [], AURATHOS_LABEL: []}
        self.cue_events = {VETHARA_LABEL: [], AURATHOS_LABEL: []}
        self.pattern_observation_baselines = {
            VETHARA_LABEL: {},
            AURATHOS_LABEL: {},
        }
        self.pattern_static = {}
        self.observe_label = None
        self.respawn_cycle = 0
        self.respawn_records = []
        self.death_started_at = None
        self.first_victory_record = None
        self.default_save_mtime_before_boss = None
        self.save_expected_hp = None
        self.save_expected_stamina = None
        self.load_old_player_path = None
        self.load_old_player_python_id = None
        self.load_old_world_path = None
        self.load_old_world_python_id = None
        self.load_requested_at = None
        self.load_seen_world_gap = False
        self.log_source_path = None
        self.log_start_offset = 0

        self.classes = {
            name: _load_exception_class(name)
            for name in (
                "BRBossArenaTrigger",
                "BRBossBase",
                "BRCheckpoint",
                "BRStoryIntroDirector",
                "BRSaveGameSubsystem",
                "BRHiddenStorySubsystem",
            )
        }

        self._capture_log_start()
        self._event("armed", "Waiting for in-process PIE on L_Runtime_Field")
        self._write_report()

    # ------------------------------------------------------------------
    # Reporting and cleanup
    # ------------------------------------------------------------------

    def _log(self, message):
        line = f"{_utc_now()} [{self.state}] {message}"
        try:
            with self.log_path.open("a", encoding="utf-8", newline="\n") as handle:
                handle.write(line + "\n")
        except Exception:
            pass
        unreal.log(f"[AuditVerticalSliceRuntime] {message}")

    def _event(self, event, detail):
        self.timeline.append({"time": _utc_now(), "event": event, "detail": str(detail)})
        self._log(f"{event}: {detail}")

    def _record(self, result_id, status, summary, evidence=None):
        if status not in ("PASS", "FAIL", "UNSUPPORTED"):
            raise ValueError(f"Invalid QA status: {status}")
        previous = self.results.get(result_id)
        # A later observation may strengthen UNSUPPORTED into PASS/FAIL, but a
        # real FAIL must never be overwritten by a convenient PASS.
        if previous and previous["status"] == "FAIL" and status != "FAIL":
            return
        self.results[result_id] = {
            "status": status,
            "summary": str(summary),
            "evidence": evidence if evidence is not None else {},
            "time": _utc_now(),
        }
        self._event(f"result.{result_id}.{status}", summary)
        self._write_report()

    def _set_state(self, state, timeout=STATE_TIMEOUT_SECONDS):
        self.state = state
        self.state_started_at = time.monotonic()
        self.state_timeout = float(timeout)
        self._event("state", state)

    def _state_elapsed(self):
        return time.monotonic() - self.state_started_at

    def _overall_status(self):
        # Watchdog/state-machine failures are deliberately not part of the
        # feature checklist: they describe the audit harness itself.  They
        # must nevertheless make the whole run fail rather than allowing a
        # misleading INCOMPLETE result from still-missing required checks.
        if any(result["status"] == "FAIL" for result in self.results.values()):
            return "FAIL"
        required = [self.results.get(result_id) for result_id in REQUIRED_RESULT_IDS]
        if any(result is None or result["status"] == "UNSUPPORTED" for result in required):
            return "INCOMPLETE"
        return "PASS"

    def _report_payload(self):
        return {
            "schema_version": 1,
            "audit": "Exception Runtime Necropolis vertical-slice PIE audit",
            "run_id": self.run_id,
            "created_at_utc": self.created_at,
            "updated_at_utc": _utc_now(),
            "state": self.state,
            "finished": self.finished,
            "overall_status": self._overall_status(),
            "map_required": MAP_NAME,
            "temporary_save_slot": self.qa_slot,
            "results": self.results,
            "respawn_records": self.respawn_records,
            "timeline": self.timeline,
            "delegate_bind_errors": self.delegate_bind_errors,
            "delegate_unbind_errors": self.delegate_unbind_errors,
            "subsystem_lookup_errors": self.subsystem_lookup_errors,
            "notes": self.notes,
            "cleanup": self.cleanup,
        }

    def _write_report(self):
        payload = self._report_payload()
        encoded = json.dumps(payload, ensure_ascii=False, indent=2)
        try:
            self.json_path.write_text(encoded, encoding="utf-8")
            self.latest_path.write_text(encoded, encoding="utf-8")
        except Exception as exc:
            unreal.log_warning(f"[AuditVerticalSliceRuntime] Could not write report: {exc}")

    @staticmethod
    def _save_signature(path):
        try:
            if not path.exists():
                return {"exists": False}
            stat = path.stat()
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            return {
                "exists": True,
                "size": stat.st_size,
                "mtime_ns": stat.st_mtime_ns,
                "sha256": digest,
            }
        except Exception as exc:
            return {"exists": None, "error": str(exc)}

    def _prepare_default_save_protection(self):
        """Snapshot the production save immediately before QA mutates PIE state."""
        try:
            before = self._save_signature(self.default_save_path)
            if before.get("exists") is None:
                raise RuntimeError(before.get("error", "could not inspect default save"))

            self.cleanup["default_save_originally_existed"] = bool(before["exists"])
            if before["exists"]:
                self.backup_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(self.default_save_path, self.backup_path)
                after = self._save_signature(self.default_save_path)
                if after != before:
                    self.backup_path.unlink(missing_ok=True)
                    raise RuntimeError(
                        f"{DEFAULT_SAVE_SLOT} changed while its QA backup was being created"
                    )
                backup_signature = self._save_signature(self.backup_path)
                if backup_signature.get("sha256") != before.get("sha256"):
                    self.backup_path.unlink(missing_ok=True)
                    raise RuntimeError("default save backup hash did not match the source")
            else:
                # Recheck the missing file as well.  If another process creates
                # it during this boundary, abort instead of later deleting it.
                after = self._save_signature(self.default_save_path)
                if after != before:
                    raise RuntimeError(
                        f"{DEFAULT_SAVE_SLOT} appeared while QA protection was being prepared"
                    )

            self.cleanup["default_save_snapshot_signature"] = before
            self.cleanup["default_save_backup_ready"] = True
            self.cleanup["default_save_restore_needed"] = True
        except Exception as exc:
            self.cleanup["default_save_backup_ready"] = False
            self.notes.append(f"Could not back up {DEFAULT_SAVE_SLOT}: {exc}")

    def _restore_save_files(self):
        try:
            if not self.cleanup.get("default_save_restore_needed", False):
                self.cleanup["default_save_restored"] = True
            else:
                originally_existed = self.cleanup["default_save_originally_existed"]
                if originally_existed:
                    if not self.backup_path.exists():
                        raise RuntimeError("original save existed but its QA backup is missing")
                    self.save_dir.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(self.backup_path, self.default_save_path)
                    self.cleanup["default_save_restored"] = True
                    self.backup_path.unlink(missing_ok=True)
                else:
                    # Exact, project-local filename only.  Remove the save produced
                    # by checkpoint/boss auto-save so an audit cannot create user
                    # progression where none existed before.
                    self.default_save_path.unlink(missing_ok=True)
                    self.cleanup["default_save_restored"] = not self.default_save_path.exists()
        except Exception as exc:
            self.cleanup["default_save_restored"] = False
            self.notes.append(f"Default save restoration failed: {exc}")

        try:
            self.qa_save_path.unlink(missing_ok=True)
            self.cleanup["temporary_save_removed"] = not self.qa_save_path.exists()
        except Exception as exc:
            self.cleanup["temporary_save_removed"] = False
            self.notes.append(f"Temporary QA save cleanup failed: {exc}")

    def _capture_log_start(self):
        try:
            log_dir = Path(unreal.Paths.project_log_dir())
            candidates = sorted(log_dir.glob("*.log"), key=lambda path: path.stat().st_mtime_ns)
            if candidates:
                self.log_source_path = candidates[-1]
                self.log_start_offset = self.log_source_path.stat().st_size
        except Exception as exc:
            self.notes.append(f"Could not establish runtime log offset: {exc}")

    def _audit_runtime_log(self):
        if not self.log_source_path:
            self._record(
                "runtime_log_errors",
                "UNSUPPORTED",
                "No active Unreal log file was discoverable when the audit was armed.",
            )
            return
        try:
            raw = self.log_source_path.read_bytes()
            tail = raw[min(self.log_start_offset, len(raw)) :].decode("utf-8", errors="replace")
            matches = []
            pattern = re.compile(
                r"(?:Log[^:\r\n]+:\s+(?:Error|Fatal):|Fatal error:|ensure condition failed)",
                re.IGNORECASE,
            )
            for line in tail.splitlines():
                if pattern.search(line) and "AuditVerticalSliceRuntime" not in line:
                    matches.append(line[-1200:])
            if matches:
                self._record(
                    "runtime_log_errors",
                    "FAIL",
                    f"{len(matches)} new Error/Fatal/ensure log lines appeared during the PIE audit.",
                    {"log": str(self.log_source_path), "lines": matches[:40]},
                )
            else:
                self._record(
                    "runtime_log_errors",
                    "PASS",
                    "No new Error/Fatal/ensure lines appeared in the Unreal log during this run.",
                    {"log": str(self.log_source_path), "bytes_scanned": len(tail.encode("utf-8"))},
                )
        except Exception as exc:
            self._record(
                "runtime_log_errors",
                "UNSUPPORTED",
                f"The runtime log could not be read after PIE: {exc}",
            )

    def _mark_unreached_results(self):
        for result_id in REQUIRED_RESULT_IDS:
            if result_id not in self.results:
                self._record(
                    result_id,
                    "UNSUPPORTED",
                    f"The state machine ended before reaching {result_id}.",
                )

    def _unbind_runtime_events(self, allow_native_unbind=True):
        """Detach Python callbacks while their PIE owners are still alive.

        Delegate proxies are intentionally not retained after this call.  The
        normal save/load path invokes this before OpenLevel, so neither the
        replacement world nor external PIE teardown can call a closure that
        still owns this audit runner.
        """
        bindings = list(self.delegate_callbacks)
        self.delegate_callbacks.clear()
        errors = []

        for owner, property_name, callback, label in reversed(bindings):
            if not allow_native_unbind:
                continue
            if not _is_valid(owner):
                # A destroyed owner no longer has a live multicast delegate.
                continue
            try:
                delegate = getattr(owner, property_name, None)
                if delegate is None:
                    delegate = owner.get_editor_property(property_name)
                if hasattr(delegate, "remove_callable"):
                    delegate.remove_callable(callback)
                elif hasattr(delegate, "remove_callable_unique"):
                    delegate.remove_callable_unique(callback)
                else:
                    raise AttributeError("delegate exposes no callable removal API")
            except Exception as exc:
                errors.append(f"{label}.{property_name}: {type(exc).__name__}: {exc}")

        for error in errors:
            if error not in self.delegate_unbind_errors:
                self.delegate_unbind_errors.append(error)
                self._log(f"Delegate unbind failed: {error}")
        self.cleanup["runtime_delegate_bindings_released"] = not errors

    def _release_runtime_references(self):
        """Release strong references to objects owned by the current PIE world."""
        for name in (
            "world",
            "player",
            "controller",
            "game_mode",
            "checkpoint",
            "arena",
            "vethara",
            "aurathos",
            "hidden_story",
            "save_subsystem",
        ):
            setattr(self, name, None)
        self.cleanup["runtime_references_released"] = True

    def _request_end(self, reason):
        if self.end_requested:
            return
        self.end_requested = True
        self.cleanup["external_stop_required"] = True
        self._event("ready_for_external_stop", reason)

        # Calling an editor stop function from this Slate callback after
        # OpenLevel can race Python proxy destruction against PIE teardown.
        # Detach while a live world is observable, release all world-owned
        # references, and let the editor/MCP operator stop PIE out-of-band.
        self._unbind_runtime_events(allow_native_unbind=_get_game_world() is not None)
        self._release_runtime_references()
        self._set_state("READY_FOR_EXTERNAL_STOP", 0.0)
        self._write_report()
        unreal.log_warning(
            "[AuditVerticalSliceRuntime] READY_FOR_EXTERNAL_STOP. "
            "Stop PIE externally; cleanup will run after the PIE world disappears."
        )

    def _finish(self):
        if self.finished:
            return
        was_ready_for_external_stop = self.state == "READY_FOR_EXTERNAL_STOP"
        self._unbind_runtime_events(allow_native_unbind=False)
        self._release_runtime_references()
        self._audit_runtime_log()
        self._mark_unreached_results()
        self._restore_save_files()
        if was_ready_for_external_stop:
            self.cleanup["external_stop_observed"] = True
        self.finished = True
        self.state = "FINISHED"
        self._event("finished", self._overall_status())
        self._write_report()
        if self.tick_handle is not None:
            try:
                unreal.unregister_slate_post_tick_callback(self.tick_handle)
            except Exception:
                pass
            self.tick_handle = None

    def stop(self, reason="stopped by a newer audit instance"):
        if self.finished:
            return True
        if _get_game_world() is not None:
            self.notes.append(
                f"{reason}; re-arm deferred until the active PIE session is stopped externally"
            )
            self._request_end(reason)
            return False
        self.notes.append(reason)
        if self.tick_handle is not None:
            try:
                unreal.unregister_slate_post_tick_callback(self.tick_handle)
            except Exception:
                pass
            self.tick_handle = None
        self._unbind_runtime_events(allow_native_unbind=False)
        self._release_runtime_references()
        self._restore_save_files()
        self.finished = True
        self.state = "STOPPED"
        self._write_report()
        return True

    # ------------------------------------------------------------------
    # Runtime object discovery and delegates
    # ------------------------------------------------------------------

    def _refresh_core_refs(self, world):
        self.world = world
        try:
            self.player = unreal.GameplayStatics.get_player_character(world, 0)
        except Exception:
            self.player = None
        try:
            self.controller = unreal.GameplayStatics.get_player_controller(world, 0)
        except Exception:
            self.controller = None
        try:
            self.game_mode = unreal.GameplayStatics.get_game_mode(world)
        except Exception:
            self.game_mode = None

        self.checkpoint = _find_actor(world, self.classes["BRCheckpoint"], CHECKPOINT_LABEL)
        self.arena = _find_actor(world, self.classes["BRBossArenaTrigger"], PYTHON_ARENA_LABEL)
        self.vethara = _find_actor(world, self.classes["BRBossBase"], VETHARA_LABEL)
        self.aurathos = _find_actor(world, self.classes["BRBossBase"], AURATHOS_LABEL)

        self.save_subsystem = self._get_game_instance_subsystem("BRSaveGameSubsystem")
        if not _is_valid(self.save_subsystem) and hasattr(
            unreal, "BRSaveBlueprintLibrary"
        ):
            self.save_subsystem = _SaveBlueprintLibraryAdapter(world)
        self.hidden_story = self._get_game_instance_subsystem("BRHiddenStorySubsystem")

    def _get_game_instance_subsystem(self, class_name):
        subsystem_class = self.classes.get(class_name)
        if not _is_valid(self.world) or subsystem_class is None:
            return None
        try:
            return unreal.SubsystemBlueprintLibrary.get_game_instance_subsystem(
                self.world, _class_arg(subsystem_class)
            )
        except Exception as exc:
            diagnostic = f"{class_name}: {type(exc).__name__}: {exc}"
            if diagnostic not in self.subsystem_lookup_errors:
                self.subsystem_lookup_errors.append(diagnostic)
                self._log(f"GameInstance subsystem lookup unavailable: {diagnostic}")
            return None

    def _saved_boss_ids(self, slot_name):
        try:
            save_game = unreal.GameplayStatics.load_game_from_slot(slot_name, 0)
            if not _is_valid(save_game):
                return [], "save object unavailable"
            ids = [_name_text(value) for value in list(_get_prop(save_game, "defeated_boss_ids"))]
            return ids, None
        except Exception as exc:
            return [], f"{type(exc).__name__}: {exc}"

    def _bind_delegate(self, owner, property_name, callback, label):
        try:
            delegate = getattr(owner, property_name, None)
            if delegate is None:
                delegate = owner.get_editor_property(property_name)
            delegate.add_callable_unique(callback)
            # Keep the owner/property pair rather than a delegate proxy.  This
            # lets cleanup reacquire and detach the multicast delegate while
            # its owner is valid without retaining a stale proxy over OpenLevel.
            self.delegate_callbacks.append((owner, property_name, callback, label))
            return True
        except Exception as exc:
            error = f"{label}.{property_name}: {exc}"
            self.delegate_bind_errors.append(error)
            self._log(f"Delegate binding unsupported: {error}")
            return False

    def _bind_runtime_events(self):
        if self.delegate_callbacks:
            return

        def checkpoint_callback(_player):
            self.checkpoint_event_seen = True

        self._bind_delegate(
            self.checkpoint,
            "on_checkpoint_activated",
            checkpoint_callback,
            CHECKPOINT_LABEL,
        )

        for boss, label in ((self.vethara, VETHARA_LABEL), (self.aurathos, AURATHOS_LABEL)):
            def make_pattern_callback(target_label):
                def callback(pattern_name):
                    self.pattern_events[target_label].append(
                        {"time": _utc_now(), "pattern": _name_text(pattern_name)}
                    )
                return callback

            def make_hit_callback(target_label):
                def callback(pattern_name):
                    player_hp_after = None
                    if _is_valid(self.player):
                        try:
                            player_hp_after = float(self.player.get_current_hp())
                        except Exception:
                            pass
                    self.pattern_hit_events[target_label].append(
                        {
                            "time": _utc_now(),
                            "pattern": _name_text(pattern_name),
                            "player_hp_after": player_hp_after,
                        }
                    )
                return callback

            def make_stage_callback(target_label):
                def callback(stage, action_name):
                    self.stage_events[target_label].append(
                        {
                            "time": _utc_now(),
                            "stage": _name_text(stage),
                            "stage_token": _enum_token(stage),
                            "action": _name_text(action_name),
                        }
                    )
                return callback

            def make_cue_callback(target_label):
                def callback(cue_name, location):
                    self.cue_events[target_label].append(
                        {
                            "time": _utc_now(),
                            "cue": _name_text(cue_name),
                            "location": _vector_tuple(location),
                        }
                    )
                return callback

            self._bind_delegate(
                boss, "on_pattern_started", make_pattern_callback(label), label
            )
            self._bind_delegate(boss, "on_pattern_hit", make_hit_callback(label), label)
            self._bind_delegate(
                boss, "on_animation_stage_changed", make_stage_callback(label), label
            )
            self._bind_delegate(
                boss, "on_boss_cue_requested", make_cue_callback(label), label
            )

    def _sample_intro_camera(self):
        if not _is_valid(self.controller):
            return
        try:
            view_target = self.controller.get_view_target()
        except Exception:
            return
        label = _actor_label(view_target)
        if label.startswith("Story_PythonIntroCamera_"):
            if not self.camera_samples or self.camera_samples[-1]["label"] != label:
                self.camera_samples.append({"time": _utc_now(), "label": label})

    # ------------------------------------------------------------------
    # Static/runtime evidence helpers
    # ------------------------------------------------------------------

    def _player_hud_widget(self):
        """Return the live HUD, preferring the explicit C++ accessor."""
        getter_error = None
        getter = getattr(self.controller, "get_player_hud_widget", None)
        if callable(getter):
            try:
                widget = getter()
                if _is_valid(widget):
                    return widget, "controller.get_player_hud_widget()", None
            except Exception as exc:
                getter_error = f"{type(exc).__name__}: {exc}"

        widget = _try_prop(self.controller, "player_hud_widget", default=None)
        if _is_valid(widget):
            return widget, "controller.player_hud_widget property", getter_error
        return None, "unavailable", getter_error

    def _method_surface(self):
        required = {
            "player": (
                self.player,
                (
                    "get_current_hp",
                    "get_max_hp",
                    "get_current_stamina",
                    "restore_hp_and_stamina",
                    "apply_saved_stats",
                    "respawn_at_checkpoint",
                ),
            ),
            "controller": (
                self.controller,
                (
                    "show_player_hud_widget",
                    "hide_pause_menu_widget",
                    "get_boss_status_widget",
                ),
            ),
            "arena": (self.arena, ("activate_arena", "reset_arena_for_retry")),
            "vethara": (
                self.vethara,
                (
                    "get_current_hp",
                    "get_max_hp",
                    "get_max_groggy",
                    "is_combat_ai_enabled",
                    "apply_groggy_damage",
                    "can_be_executed",
                    "begin_execution",
                    "complete_execution",
                ),
            ),
            "aurathos": (
                self.aurathos,
                (
                    "get_current_hp",
                    "get_max_hp",
                    "get_max_groggy",
                    "is_combat_ai_enabled",
                    "apply_groggy_damage",
                    "can_be_executed",
                    "begin_execution",
                    "complete_execution",
                ),
            ),
            "save_subsystem": (
                self.save_subsystem,
                (
                    "save_current_game",
                    "load_game_from_slot_and_open_level",
                    "does_save_exist",
                ),
            ),
        }
        missing = []
        for owner_name, (owner, methods) in required.items():
            if not _is_valid(owner):
                missing.append(f"{owner_name}=invalid")
                continue
            for method in methods:
                if not hasattr(owner, method):
                    missing.append(f"{owner_name}.{method}")

        # The new boss pass is expected to expose these reflected delegates and
        # stage/action data.  Their absence usually means the editor still has
        # the old DLL loaded, which is a hard integration failure.
        for owner_name, owner in (("vethara", self.vethara), ("aurathos", self.aurathos)):
            for prop_name in (
                "on_pattern_started",
                "on_animation_stage_changed",
                "on_boss_cue_requested",
                "attack_patterns",
            ):
                try:
                    _get_prop(owner, prop_name)
                except Exception:
                    missing.append(f"{owner_name}.{prop_name}")

        return missing

    def _inspect_pattern_configuration(self, boss, label, expected_identity):
        evidence = {"label": label}
        identity = _try_prop(boss, "python_boss_identity", default=None)
        evidence["identity"] = _name_text(identity)
        patterns = list(_get_prop(boss, "attack_patterns"))
        evidence["patterns"] = []
        for pattern in patterns:
            pattern_name = _name_text(_get_prop(pattern, "pattern_name"))
            action_names = []
            for prop_name in (
                "animation_action_name",
                "windup_animation_action_name",
                "impact_animation_action_name",
                "recovery_animation_action_name",
                "follow_up_animation_action_name",
            ):
                value = _try_prop(pattern, prop_name, default=None)
                text = _name_text(value)
                if text not in ("None", ""):
                    action_names.append(text)
            evidence["patterns"].append(
                {"pattern": pattern_name, "actions": sorted(set(action_names))}
            )

        identity_ok = expected_identity.lower() in evidence["identity"].lower()
        prefix_ok = bool(evidence["patterns"]) and all(
            item["pattern"].startswith(expected_identity + "_")
            for item in evidence["patterns"]
        )
        action_ok = bool(evidence["patterns"]) and all(
            item["actions"] for item in evidence["patterns"]
        )
        return identity_ok and prefix_ok and action_ok, evidence

    def _mark_pattern_observation_baseline(self, label):
        player_hp = None
        if _is_valid(self.player):
            try:
                player_hp = float(self.player.get_current_hp())
            except Exception:
                pass
        baseline = {
            "patterns_started": len(self.pattern_events[label]),
            "pattern_hits": len(self.pattern_hit_events[label]),
            "animation_stages": len(self.stage_events[label]),
            "cues": len(self.cue_events[label]),
            "player_hp": player_hp,
        }
        self.pattern_observation_baselines[label] = baseline
        self._event(
            "pattern_observation_baseline",
            f"{label}: {baseline}",
        )

    def _runtime_pattern_evidence(self, label):
        baseline = self.pattern_observation_baselines.get(label, {})
        pattern_start = int(baseline.get("patterns_started", 0))
        hit_start = int(baseline.get("pattern_hits", 0))
        stage_start = int(baseline.get("animation_stages", 0))
        cue_start = int(baseline.get("cues", 0))
        events = self.pattern_events[label][pattern_start:]
        hits = self.pattern_hit_events[label][hit_start:]
        stages = self.stage_events[label][stage_start:]
        cues = self.cue_events[label][cue_start:]
        stage_tokens = {item["stage_token"] for item in stages}
        prefix = "Vethara_" if label == VETHARA_LABEL else "Aurathos_"
        cue_prefix = prefix
        player_hp_before = baseline.get("player_hp")
        hit_hp_samples = [
            item["player_hp_after"]
            for item in hits
            if item.get("player_hp_after") is not None
        ]
        minimum_player_hp_after_hit = min(hit_hp_samples) if hit_hp_samples else None
        has_player_hp_loss = (
            player_hp_before is not None
            and minimum_player_hp_after_hit is not None
            and minimum_player_hp_after_hit < float(player_hp_before) - 0.1
        )
        return {
            "observation_baseline": baseline,
            "patterns_started": events,
            "pattern_hits": hits,
            "animation_stages": stages,
            "cues": cues,
            "player_hp_before": player_hp_before,
            "minimum_player_hp_after_hit": minimum_player_hp_after_hit,
            "has_identity_pattern": any(
                item["pattern"].startswith(prefix) for item in events
            ),
            "has_pattern_hit": bool(hits),
            "has_player_hp_loss": has_player_hp_loss,
            "has_windup": "PATTERNWINDUP" in stage_tokens or "WINDUP" in stage_tokens,
            "has_impact": "PATTERNIMPACT" in stage_tokens or "IMPACT" in stage_tokens,
            "has_recovery": "PATTERNRECOVERY" in stage_tokens or "RECOVERY" in stage_tokens,
            "has_identity_cue": any(item["cue"].startswith(cue_prefix) for item in cues),
        }

    def _runtime_pattern_complete(self, label):
        evidence = self._runtime_pattern_evidence(label)
        return all(
            evidence[key]
            for key in (
                "has_identity_pattern",
                "has_pattern_hit",
                "has_player_hp_loss",
                "has_windup",
                "has_impact",
                "has_recovery",
                "has_identity_cue",
            )
        )

    def _arena_bool(self, property_name):
        return bool(_get_prop(self.arena, property_name))

    def _move_player(self, location):
        try:
            self.player.set_actor_location(location, False, True)
            return True
        except Exception as exc:
            self.notes.append(f"Could not teleport player for QA setup: {exc}")
            return False

    def _set_boss_observation(self, observed, other, label, distance):
        self.player.restore_hp_and_stamina()
        other.set_combat_ai_enabled(False)
        observed.reset_boss()
        # Ignore all delegate traffic produced by the preceding dual-boss
        # intro/fight (and by ResetBoss itself).  Only events emitted after
        # this identity is isolated may satisfy its runtime-pattern check.
        self._mark_pattern_observation_baseline(label)
        observed.set_combat_ai_enabled(True)
        location = observed.get_actor_location()
        target = unreal.Vector(location.x + distance, location.y, location.z)
        self._move_player(target)
        self.observe_label = label

    def _boss_reset_snapshot(self):
        entries = []
        okay = not self._arena_bool("arena_started")
        for boss in (self.vethara, self.aurathos):
            entry = {
                "label": _actor_label(boss),
                "dead": _bool_api(boss, "is_dead"),
                "groggy": _bool_api(boss, "is_groggy"),
                "ai": bool(boss.is_combat_ai_enabled()),
                "hp": float(boss.get_current_hp()),
                "max_hp": float(boss.get_max_hp()),
            }
            entries.append(entry)
            okay = okay and not entry["dead"] and not entry["groggy"] and not entry["ai"]
            okay = okay and abs(entry["hp"] - entry["max_hp"]) <= 0.1
        return okay, entries

    def _execute_boss(self, boss, lethal):
        before_hp = float(boss.get_current_hp())
        groggy_amount = float(boss.get_max_groggy()) + 10.0
        groggy_applied = bool(boss.apply_groggy_damage(groggy_amount, self.player))
        is_groggy = _bool_api(boss, "is_groggy")
        executable = bool(boss.can_be_executed())
        began = bool(boss.begin_execution(self.player)) if executable else False
        damage = float(boss.get_max_hp()) + 100.0 if lethal else 1.0
        completed = bool(boss.complete_execution(damage, self.player)) if began else False
        return {
            "label": _actor_label(boss),
            "groggy_applied": groggy_applied,
            "is_groggy_before_execution": is_groggy,
            "can_be_executed": executable,
            "begin_execution": began,
            "complete_execution": completed,
            "damage": damage,
            "hp_before": before_hp,
            "hp_after": float(boss.get_current_hp()),
            "dead_after": _bool_api(boss, "is_dead"),
        }

    # ------------------------------------------------------------------
    # State machine
    # ------------------------------------------------------------------

    def start(self):
        self.tick_handle = unreal.register_slate_post_tick_callback(self.tick)
        self._log(f"ARMED. Start PIE on {MAP_NAME}; report={self.json_path}")

    def tick(self, _delta_seconds):
        if self.finished:
            return
        try:
            now = time.monotonic()
            world = _get_game_world()

            if self.state == "WAIT_FOR_PIE":
                if world is None:
                    if self._state_elapsed() > self.state_timeout:
                        self._record(
                            "pie_runtime_map",
                            "FAIL",
                            f"No in-process PIE world appeared within {WAIT_FOR_PIE_SECONDS:.0f}s.",
                        )
                        self._finish()
                    return
                self.active_started_at = now
                # Exclude editor/MCP map-duplication traffic that happened
                # before a PIE world existed; audit runtime errors from the
                # first observable game world onward.
                self._capture_log_start()
                self._set_state("DISCOVER_RUNTIME", 15.0)

            if self.state == "READY_FOR_EXTERNAL_STOP":
                # Do not run a watchdog, touch released PIE UObjects, restore
                # saves, or stop the editor from this callback.  An external
                # StopPIE makes the world disappear; only then is cleanup safe.
                if world is None:
                    self._finish()
                return

            if self.active_started_at and now - self.active_started_at > MAX_ACTIVE_RUN_SECONDS:
                self._record(
                    "runtime_watchdog",
                    "FAIL",
                    f"Active PIE audit exceeded {MAX_ACTIVE_RUN_SECONDS:.0f}s watchdog.",
                    {"state": self.state},
                )
                self._request_end("active-run watchdog")
                return

            if self.state == "WAIT_FOR_LOAD":
                if world is None:
                    self.load_seen_world_gap = True
                    if self._state_elapsed() > self.state_timeout:
                        self._record(
                            "save_load_round_trip",
                            "FAIL",
                            "PIE world did not return after OpenLevel during load.",
                        )
                        self._request_end("load world timeout")
                    return
                self._state_wait_for_load(world)
                return

            if world is None:
                self._record(
                    "pie_runtime_map",
                    "FAIL",
                    f"The PIE world disappeared unexpectedly in state {self.state}.",
                )
                self._request_end("unexpected PIE termination")
                return

            self.world = world
            if self.state == "WAIT_FOR_INTRO_AI":
                self._sample_intro_camera()

            handler = getattr(self, f"_state_{self.state.lower()}", None)
            if handler is None:
                raise RuntimeError(f"No handler for state {self.state}")
            handler(world)

            if self.state not in ("WAIT_FOR_PIE", "READY_FOR_EXTERNAL_STOP"):
                if self._state_elapsed() > self.state_timeout:
                    timed_out_state = self.state
                    self._record(
                        "state_machine_timeout",
                        "FAIL",
                        f"State {timed_out_state} exceeded its {self.state_timeout:.1f}s timeout.",
                    )
                    self._request_end(f"state timeout: {timed_out_state}")
        except Exception as exc:
            self.notes.append(traceback.format_exc())
            self._record(
                "runtime_state_machine",
                "FAIL",
                f"Unhandled QA exception in {self.state}: {exc}",
                {"traceback": traceback.format_exc()},
            )
            self._request_end("unhandled QA exception")

    def _state_discover_runtime(self, world):
        self._refresh_core_refs(world)
        level_name = str(unreal.GameplayStatics.get_current_level_name(world, True))
        if level_name != MAP_NAME:
            self._record(
                "pie_runtime_map",
                "FAIL",
                f"PIE loaded {level_name}, expected {MAP_NAME}.",
            )
            self._request_end("wrong runtime map")
            return

        refs = {
            "player": self.player,
            "controller": self.controller,
            "game_mode": self.game_mode,
            "checkpoint": self.checkpoint,
            "arena": self.arena,
            "vethara": self.vethara,
            "aurathos": self.aurathos,
            "save_subsystem": self.save_subsystem,
        }
        missing_refs = [name for name, value in refs.items() if not _is_valid(value)]
        if missing_refs:
            if self._state_elapsed() < 8.0:
                return
            self._record(
                "runtime_api_surface",
                "FAIL",
                f"Required PIE objects are missing: {missing_refs}",
            )
            self._request_end("missing required runtime objects")
            return

        # GameplayStatics' indexed player-character lookup is the primary
        # possession evidence for this single-player PIE harness.  Direct
        # controller/pawn getters are retained as diagnostics because Unreal's
        # Python wrappers have intermittently returned None for those links
        # even while the indexed controller owns and drives the character.
        indexed_player = None
        indexed_player_error = None
        try:
            indexed_player = unreal.GameplayStatics.get_player_character(world, 0)
        except Exception as exc:
            indexed_player_error = f"{type(exc).__name__}: {exc}"
        indexed_player_path = _object_path(indexed_player)
        discovered_player_path = _object_path(self.player)
        indexed_player_matches = (
            indexed_player_path != "<invalid>"
            and indexed_player_path == discovered_player_path
        )

        possessed_pawn = None
        player_controller = None
        direct_pawn_error = None
        direct_controller_error = None
        pawn_matches = False
        controller_matches = False
        try:
            possessed_pawn = self.controller.get_pawn()
            pawn_matches = (
                _object_path(possessed_pawn) != "<invalid>"
                and _object_path(possessed_pawn) == discovered_player_path
            )
        except Exception as exc:
            direct_pawn_error = f"{type(exc).__name__}: {exc}"
        try:
            player_controller = self.player.get_controller()
            controller_matches = (
                _object_path(player_controller) != "<invalid>"
                and _object_path(player_controller) == _object_path(self.controller)
            )
        except Exception as exc:
            direct_controller_error = f"{type(exc).__name__}: {exc}"
        direct_link_matches = pawn_matches or controller_matches

        hud, hud_source, hud_getter_error = self._player_hud_widget()
        hud_in_viewport = False
        if _is_valid(hud):
            try:
                hud_in_viewport = bool(hud.is_in_viewport())
            except Exception:
                hud_in_viewport = False

        # Do not judge a viewport PIE session while indexed player discovery
        # and SetPawn's HUD retry are still settling.  Poll both until the
        # discovery deadline; only then emit a real FAIL and try the diagnostic
        # manual HUD fallback.
        startup_ready = indexed_player_matches and hud_in_viewport
        if not startup_ready and self._state_elapsed() < 8.0:
            return

        self._record(
            "pie_runtime_map",
            "PASS",
            f"In-process PIE is running {MAP_NAME} with the authored Python arena.",
            {"level": level_name},
        )

        missing_methods = self._method_surface()
        if missing_methods:
            self._record(
                "runtime_api_surface",
                "FAIL",
                "The restarted editor does not expose the required runtime QA surface.",
                {"missing": missing_methods},
            )
            self._request_end("stale or incomplete runtime API")
            return
        self._record(
            "runtime_api_surface",
            "PASS",
            "Player, boss, arena, event, checkpoint, and save/load APIs are reflected in the running DLL.",
        )

        self._prepare_default_save_protection()
        if not self.cleanup["default_save_backup_ready"]:
            self._record(
                "checkpoint_activation",
                "UNSUPPORTED",
                "The existing default save could not be backed up, so mutating checkpoint/combat QA was not started.",
            )
            self._request_end("default save backup protection unavailable")
            return

        location = self.player.get_actor_location()
        spawn_distance = _distance(location, unreal.Vector(1200.0, 0.0, 150.0))
        spawn_ok = indexed_player_matches and spawn_distance <= 550.0
        self._record(
            "player_spawn_possession",
            "PASS" if spawn_ok else "FAIL",
            "Hendel spawned near the awakening start and is possessed."
            if spawn_ok
            else "The player is not possessed or spawned outside the awakening start tolerance.",
            {
                "location": _vector_tuple(location),
                "distance_from_authored_start_cm": round(spawn_distance, 2),
                "possessed": indexed_player_matches,
                "possession_primary": "GameplayStatics.get_player_character(world, 0) path equality",
                "indexed_player_matches": indexed_player_matches,
                "indexed_player_path": indexed_player_path,
                "indexed_player_error": indexed_player_error,
                "direct_link_matches": direct_link_matches,
                "direct_pawn_matches": pawn_matches,
                "direct_controller_matches": controller_matches,
                "controller_path": _object_path(self.controller),
                "controller_pawn_path": _object_path(possessed_pawn),
                "player_controller_path": _object_path(player_controller),
                "controller_pawn_raw": str(possessed_pawn),
                "player_controller_raw": str(player_controller),
                "direct_pawn_error": direct_pawn_error,
                "direct_controller_error": direct_controller_error,
                "player_path": discovered_player_path,
                "class": str(self.player.get_class().get_name()),
            },
        )

        if not hud_in_viewport:
            # Diagnostic fallback only.  Creating the HUD here cannot turn a
            # failed automatic BeginPlay check into a PASS.
            try:
                fallback_return = self.controller.show_player_hud_widget()
                fallback_hud, fallback_source, fallback_getter_error = (
                    self._player_hud_widget()
                )
                fallback_works = _is_valid(fallback_hud) and bool(
                    fallback_hud.is_in_viewport()
                )
            except Exception as exc:
                fallback_return = None
                fallback_hud = None
                fallback_source = "unavailable"
                fallback_getter_error = f"{type(exc).__name__}: {exc}"
                fallback_works = False
            self._record(
                "player_hud",
                "FAIL",
                "The player HUD was not already in the viewport after controller BeginPlay.",
                {
                    "automatic_lookup_source": hud_source,
                    "automatic_getter_error": hud_getter_error,
                    "manual_show_fallback_works": fallback_works,
                    "manual_show_return_path": _object_path(fallback_return),
                    "manual_lookup_source": fallback_source,
                    "manual_getter_error": fallback_getter_error,
                    "manual_widget_path": _object_path(fallback_hud),
                },
            )
        else:
            self._record(
                "player_hud",
                "PASS",
                "The controller-created player HUD is present in the PIE viewport.",
                {
                    "widget_class": str(hud.get_class().get_name()),
                    "widget_path": _object_path(hud),
                    "lookup_source": hud_source,
                    "getter_error": hud_getter_error,
                },
            )

        self._bind_runtime_events()

        # The Field 0 camera sequence would otherwise own the controller for
        # roughly ten seconds and contaminate Python intro camera evidence.
        directors = _all_actors(world, self.classes["BRStoryIntroDirector"])
        for director in directors:
            if hasattr(director, "skip_intro"):
                director.skip_intro()
        self._set_state("WAIT_AFTER_PROLOGUE_SKIP", 4.0)

    def _state_wait_after_prologue_skip(self, _world):
        if self._state_elapsed() < 1.25:
            return
        self._set_state("MOVE_TO_CHECKPOINT", 4.0)

    def _state_move_to_checkpoint(self, _world):
        checkpoint_location = self.checkpoint.get_actor_location()
        target = unreal.Vector(
            checkpoint_location.x,
            checkpoint_location.y,
            checkpoint_location.z + 100.0,
        )
        if not self._move_player(target):
            self._record(
                "checkpoint_activation",
                "FAIL",
                "Could not move the player into the authored checkpoint overlap.",
            )
            self._set_state("BOSS_PREPARE")
            return
        self.checkpoint_location = target
        self._set_state("WAIT_FOR_CHECKPOINT", 5.0)

    def _state_wait_for_checkpoint(self, _world):
        transform_ok = False
        actual_location = None
        try:
            if _bool_api(self.game_mode, "has_checkpoint"):
                actual_location = _transform_location(self.game_mode.get_checkpoint_transform())
                transform_ok = _distance(actual_location, self.checkpoint_location) <= 45.0
        except Exception:
            transform_ok = False

        try:
            if self.controller.is_pause_menu_open():
                self.controller.hide_pause_menu_widget()
        except Exception:
            # Calling hide is safe even if IsPauseMenuOpen is unavailable.
            try:
                self.controller.hide_pause_menu_widget()
            except Exception:
                pass

        if transform_ok:
            self._record(
                "checkpoint_activation",
                "PASS",
                "The authored checkpoint overlap updated the GameMode respawn transform and emitted its activation path.",
                {
                    "expected": _vector_tuple(self.checkpoint_location),
                    "actual": _vector_tuple(actual_location),
                    "activation_delegate_seen": self.checkpoint_event_seen,
                },
            )
            self._set_state("BOSS_PREPARE")
            return

        if self._state_elapsed() >= 4.0:
            self._record(
                "checkpoint_activation",
                "FAIL",
                "The checkpoint overlap did not replace the initial spawn checkpoint transform.",
                {
                    "expected": _vector_tuple(self.checkpoint_location),
                    "actual": _vector_tuple(actual_location) if actual_location else None,
                    "activation_delegate_seen": self.checkpoint_event_seen,
                },
            )
            # Preserve downstream testability without claiming the overlap
            # worked.  This invokes the same public GameMode checkpoint API.
            fallback = self.player.get_actor_transform()
            self.game_mode.set_checkpoint_transform(fallback)
            self.checkpoint_location = _transform_location(fallback)
            self._set_state("BOSS_PREPARE")

    def _state_boss_prepare(self, _world):
        if self._arena_bool("arena_cleared"):
            self._record(
                "python_dual_boss_ai",
                "FAIL",
                "Python arena entered PIE already cleared; a fresh encounter could not be exercised.",
            )
            self._request_end("Python encounter was already cleared")
            return

        identity_evidence = {}
        configuration_ok = True
        try:
            for boss, label, identity in (
                (self.vethara, VETHARA_LABEL, "Vethara"),
                (self.aurathos, AURATHOS_LABEL, "Aurathos"),
            ):
                okay, evidence = self._inspect_pattern_configuration(boss, label, identity)
                identity_evidence[label] = evidence
                configuration_ok = configuration_ok and okay
        except Exception as exc:
            configuration_ok = False
            identity_evidence["inspection_error"] = str(exc)

        v_names = {
            item["pattern"] for item in identity_evidence.get(VETHARA_LABEL, {}).get("patterns", [])
        }
        a_names = {
            item["pattern"] for item in identity_evidence.get(AURATHOS_LABEL, {}).get("patterns", [])
        }
        configuration_ok = configuration_ok and bool(v_names) and bool(a_names) and v_names.isdisjoint(a_names)
        self.pattern_static = identity_evidence
        self._record(
            "python_identity_configuration",
            "PASS" if configuration_ok else "FAIL",
            "Vethara and Aurathos expose distinct identity-prefixed patterns with explicit action names."
            if configuration_ok
            else "Python twin identity, pattern prefix, or action-name configuration is incomplete.",
            identity_evidence,
        )

        self.arena.reset_arena_for_retry()
        self.player.restore_hp_and_stamina()
        self._move_player(unreal.Vector(3000.0, 5200.0, 250.0))
        self.camera_samples = []
        self.arena.activate_arena()
        self._set_state("WAIT_FOR_INTRO_AI", 14.0)

    def _state_wait_for_intro_ai(self, _world):
        started = self._arena_bool("arena_started")
        both_ai = bool(self.vethara.is_combat_ai_enabled()) and bool(
            self.aurathos.is_combat_ai_enabled()
        )
        if not both_ai and self._state_elapsed() < 12.0:
            return

        cameras = list(_get_prop(self.arena, "intro_cameras"))
        configured_labels = [_actor_label(camera) for camera in cameras if _is_valid(camera)]
        sampled_labels = [sample["label"] for sample in self.camera_samples]
        view_api_supported = hasattr(self.controller, "get_view_target")
        camera_ok = (
            len(configured_labels) == 3
            and view_api_supported
            and all(label in sampled_labels for label in configured_labels)
        )
        if not view_api_supported:
            camera_status = "UNSUPPORTED"
            camera_summary = "PlayerController.GetViewTarget is not exposed to Python; live camera switching could not be observed."
        else:
            camera_status = "PASS" if camera_ok else "FAIL"
            camera_summary = (
                "All three authored Python intro cameras became the live PIE view target before AI activation."
                if camera_ok
                else "The three-shot Python intro was not fully observed as live PIE view targets."
            )
        self._record(
            "python_intro_camera_flow",
            camera_status,
            camera_summary,
            {
                "configured": configured_labels,
                "sampled": self.camera_samples,
                "ai_enabled_after_intro": both_ai,
            },
        )

        controllers = []
        for boss in (self.vethara, self.aurathos):
            try:
                controllers.append(_is_valid(boss.get_boss_ai_controller()))
            except Exception:
                controllers.append(False)
        same_team = False
        try:
            same_team = (
                _is_valid(self.vethara.get_team_coordinator())
                and self.vethara.get_team_coordinator() == self.aurathos.get_team_coordinator()
            )
        except Exception:
            same_team = False
        dual_ok = started and both_ai and all(controllers) and same_team
        self._record(
            "python_dual_boss_ai",
            "PASS" if dual_ok else "FAIL",
            "Both Python identities activated after the reveal with live AI controllers and a shared coordinator."
            if dual_ok
            else "The Python encounter did not produce two coordinated, AI-enabled bosses after its intro.",
            {
                "arena_started": started,
                "vethara_ai": bool(self.vethara.is_combat_ai_enabled()),
                "aurathos_ai": bool(self.aurathos.is_combat_ai_enabled()),
                "ai_controllers": controllers,
                "shared_team_coordinator": same_team,
            },
        )

        # Keep testing after a failed intro/AI observation, but make the setup
        # intervention explicit in the existing FAIL evidence.
        self.vethara.set_combat_ai_enabled(True)
        self.aurathos.set_combat_ai_enabled(True)
        self._set_boss_observation(
            self.vethara, self.aurathos, VETHARA_LABEL, 700.0
        )
        self._set_state("OBSERVE_VETHARA", PATTERN_OBSERVE_SECONDS + 1.0)

    def _state_observe_vethara(self, _world):
        if self.player.get_current_hp() < self.player.get_max_hp() * 0.3:
            self.player.restore_hp_and_stamina()
        if self._runtime_pattern_complete(VETHARA_LABEL):
            evidence = self._runtime_pattern_evidence(VETHARA_LABEL)
            self._record(
                "vethara_runtime_patterns",
                "PASS",
                "Vethara emitted an identity pattern, dealt observed player HP damage, and completed Windup, Impact, Recovery, and identity audio cue events in PIE.",
                evidence,
            )
            self._set_boss_observation(
                self.aurathos, self.vethara, AURATHOS_LABEL, 250.0
            )
            self._set_state("OBSERVE_AURATHOS", PATTERN_OBSERVE_SECONDS + 1.0)
            return
        if self._state_elapsed() >= PATTERN_OBSERVE_SECONDS:
            delegate_missing = any(
                VETHARA_LABEL in error for error in self.delegate_bind_errors
            )
            self._record(
                "vethara_runtime_patterns",
                "UNSUPPORTED" if delegate_missing else "FAIL",
                "Vethara runtime delegates could not be bound."
                if delegate_missing
                else "Vethara did not complete an observable identity action cycle within the timeout.",
                self._runtime_pattern_evidence(VETHARA_LABEL),
            )
            self._set_boss_observation(
                self.aurathos, self.vethara, AURATHOS_LABEL, 250.0
            )
            self._set_state("OBSERVE_AURATHOS", PATTERN_OBSERVE_SECONDS + 1.0)

    def _state_observe_aurathos(self, _world):
        if self.player.get_current_hp() < self.player.get_max_hp() * 0.3:
            self.player.restore_hp_and_stamina()
        if self._runtime_pattern_complete(AURATHOS_LABEL):
            evidence = self._runtime_pattern_evidence(AURATHOS_LABEL)
            self._record(
                "aurathos_runtime_patterns",
                "PASS",
                "Aurathos emitted an identity pattern, dealt observed player HP damage, and completed Windup, Impact, Recovery, and identity audio cue events in PIE.",
                evidence,
            )
            self._set_state("TEST_GROGGY_EXECUTION")
            return
        if self._state_elapsed() >= PATTERN_OBSERVE_SECONDS:
            delegate_missing = any(
                AURATHOS_LABEL in error for error in self.delegate_bind_errors
            )
            self._record(
                "aurathos_runtime_patterns",
                "UNSUPPORTED" if delegate_missing else "FAIL",
                "Aurathos runtime delegates could not be bound."
                if delegate_missing
                else "Aurathos did not complete an observable identity action cycle within the timeout.",
                self._runtime_pattern_evidence(AURATHOS_LABEL),
            )
            self._set_state("TEST_GROGGY_EXECUTION")

    def _state_test_groggy_execution(self, _world):
        self.vethara.set_combat_ai_enabled(False)
        self.aurathos.set_combat_ai_enabled(False)
        self.player.restore_hp_and_stamina()
        self.vethara.reset_boss()
        result = self._execute_boss(self.vethara, lethal=False)
        stage_tokens = {
            item["stage_token"] for item in self.stage_events[VETHARA_LABEL]
        }
        result["groggy_stage_seen"] = "GROGGY" in stage_tokens
        result["execution_reaction_stage_seen"] = "EXECUTIONREACTION" in stage_tokens
        functional_ok = all(
            result[key]
            for key in (
                "groggy_applied",
                "is_groggy_before_execution",
                "can_be_executed",
                "begin_execution",
                "complete_execution",
            )
        )
        functional_ok = functional_ok and not result["dead_after"]
        functional_ok = functional_ok and abs(
            (result["hp_before"] - result["hp_after"]) - 1.0
        ) <= 0.1
        if not any(VETHARA_LABEL in error for error in self.delegate_bind_errors):
            functional_ok = functional_ok and result["groggy_stage_seen"]
            functional_ok = functional_ok and result["execution_reaction_stage_seen"]
        self._record(
            "groggy_execution",
            "PASS" if functional_ok else "FAIL",
            "Groggy opened a valid execution window and a nonlethal execution completed through the boss API."
            if functional_ok
            else "Groggy or the execution begin/impact/recovery path failed.",
            result,
        )
        self.arena.reset_arena_for_retry()
        self._set_state("VERIFY_BOSS_RESET", 4.0)

    def _state_verify_boss_reset(self, _world):
        if self._state_elapsed() < 0.3:
            return
        okay, entries = self._boss_reset_snapshot()
        self._record(
            "boss_retry_reset",
            "PASS" if okay else "FAIL",
            "Arena retry reset restored both bosses to full, non-Groggy, inactive state."
            if okay
            else "Arena retry reset left a boss damaged, active, dead, or Groggy.",
            {"arena_started": self._arena_bool("arena_started"), "bosses": entries},
        )
        self.respawn_cycle = 0
        self._set_state("RESPAWN_START_CYCLE")

    def _state_respawn_start_cycle(self, _world):
        if self.respawn_cycle >= 3:
            all_ok = len(self.respawn_records) == 3 and all(
                record.get("passed", False) for record in self.respawn_records
            )
            self._record(
                "checkpoint_respawn_three_cycles",
                "PASS" if all_ok else "FAIL",
                "Three lethal damage events respawned the player at the checkpoint and reset the active Python encounter."
                if all_ok
                else "One or more of the three checkpoint respawn cycles failed.",
                {"cycles": self.respawn_records},
            )
            self._set_state("FINAL_VICTORY_PREPARE")
            return

        self.player.restore_hp_and_stamina()
        self._move_player(unreal.Vector(3600.0, 5200.0, 250.0))
        self.arena.activate_arena()
        self._set_state("RESPAWN_WAIT_FOR_AI", 11.0)

    def _state_respawn_wait_for_ai(self, _world):
        vethara_ai = bool(self.vethara.is_combat_ai_enabled())
        aurathos_ai = bool(self.aurathos.is_combat_ai_enabled())
        both_ai = vethara_ai and aurathos_ai
        # A slow/replayed intro is itself evidence, but it must not prevent the
        # remaining real death/respawn cycles from being exercised.  Continue
        # after eight seconds and make this cycle fail explicitly if AI never
        # became ready.
        if not both_ai and self._state_elapsed() < 8.0:
            return
        self.player.restore_hp_and_stamina()
        applied = float(
            unreal.GameplayStatics.apply_damage(
                self.player,
                float(self.player.get_max_hp()) + 100.0,
                None,
                None,
                None,
            )
        )
        dead_hp = float(self.player.get_current_hp())
        self.death_started_at = time.monotonic()
        self.respawn_records.append(
            {
                "cycle": self.respawn_cycle + 1,
                "damage_applied": applied,
                "hp_immediately_after": dead_hp,
                "death_triggered": applied > 0.0 and dead_hp <= 0.0,
                "paused_immediately_after_death": _game_paused(_world),
                "arena_started_before_death": self._arena_bool("arena_started"),
                "vethara_ai_before_death": vethara_ai,
                "aurathos_ai_before_death": aurathos_ai,
                "ai_ready_before_death": both_ai,
            }
        )
        self._set_state("RESPAWN_WAIT_FOR_RETURN", 8.0)

    def _state_respawn_wait_for_return(self, _world):
        record = self.respawn_records[-1]
        if time.monotonic() - self.death_started_at < 1.0:
            return
        player_hp = float(self.player.get_current_hp())
        if player_hp <= 0.0:
            if time.monotonic() - self.death_started_at < 5.5:
                return
            # Preserve downstream coverage after recording a real natural
            # respawn timeout.  This public recovery call cannot make the
            # cycle pass because natural_respawn remains false.
            record["natural_respawn"] = False
            record["natural_respawn_timed_out"] = True
            try:
                self.player.respawn_at_checkpoint()
                record["manual_recovery_called"] = True
            except Exception as exc:
                record["manual_recovery_called"] = False
                record["manual_recovery_error"] = str(exc)
            player_hp = float(self.player.get_current_hp())
        else:
            record["natural_respawn"] = True

        paused_after_respawn = _game_paused(_world)
        player_location = self.player.get_actor_location()
        reset_ok, reset_entries = self._boss_reset_snapshot()
        location_error = _distance(player_location, self.checkpoint_location)
        full_hp = abs(player_hp - float(self.player.get_max_hp())) <= 0.1
        passed = (
            record["death_triggered"]
            and record["ai_ready_before_death"]
            and record["natural_respawn"]
            and paused_after_respawn is False
            and full_hp
            and location_error <= 120.0
            and reset_ok
        )
        record.update(
            {
                "respawn_location": _vector_tuple(player_location),
                "checkpoint_location": _vector_tuple(self.checkpoint_location),
                "location_error_cm": round(location_error, 2),
                "full_hp": full_hp,
                "paused_immediately_after_respawn": paused_after_respawn,
                "arena_and_bosses_reset": reset_ok,
                "bosses": reset_entries,
                "elapsed_seconds": round(time.monotonic() - self.death_started_at, 3),
                "passed": passed,
            }
        )
        self._write_report()
        self.respawn_cycle += 1
        self._set_state("RESPAWN_START_CYCLE")

    def _state_final_victory_prepare(self, _world):
        self.player.restore_hp_and_stamina()
        self._move_player(unreal.Vector(3600.0, 5200.0, 250.0))
        self.arena.activate_arena()
        self._set_state("FINAL_VICTORY_WAIT_ACTIVE", 5.0)

    def _state_final_victory_wait_active(self, _world):
        if not self._arena_bool("arena_started"):
            return
        if self._state_elapsed() < 0.5:
            return
        self.vethara.set_combat_ai_enabled(False)
        self.aurathos.set_combat_ai_enabled(False)
        self.first_victory_record = self._execute_boss(self.vethara, lethal=True)
        self.first_victory_record["arena_cleared_after_first"] = self._arena_bool(
            "arena_cleared"
        )
        self._set_state("WAIT_FOR_SURVIVOR_TRANSITION", 5.0)

    def _state_wait_for_survivor_transition(self, _world):
        if _bool_api(self.aurathos, "is_phase_transitioning"):
            return
        if self._state_elapsed() < 0.15:
            return
        try:
            self.default_save_mtime_before_boss = (
                self.default_save_path.stat().st_mtime_ns
                if self.default_save_path.exists()
                else None
            )
        except Exception:
            self.default_save_mtime_before_boss = None
        second = self._execute_boss(self.aurathos, lethal=True)
        first = self.first_victory_record
        victory_ok = (
            first["dead_after"]
            and not first["arena_cleared_after_first"]
            and second["dead_after"]
            and first["begin_execution"]
            and first["complete_execution"]
            and second["begin_execution"]
            and second["complete_execution"]
        )
        self._record(
            "python_execution_victory",
            "PASS" if victory_ok else "FAIL",
            "Each Python twin was defeated through a lethal Groggy execution, and the arena waited for both."
            if victory_ok
            else "The lethal dual-execution victory sequence did not complete correctly.",
            {"first": first, "second": second},
        )
        self._set_state("WAIT_FOR_ARENA_CLEAR", 5.0)

    def _state_wait_for_arena_clear(self, _world):
        if self._state_elapsed() < 0.75:
            return
        arena_cleared = self._arena_bool("arena_cleared")
        boss_states = []
        cleanup_ok = arena_cleared
        for boss in (self.vethara, self.aurathos):
            state = {
                "label": _actor_label(boss),
                "dead": _bool_api(boss, "is_dead"),
                "ai": bool(boss.is_combat_ai_enabled()),
                "collision": bool(boss.get_actor_enable_collision()),
            }
            try:
                state["hidden"] = bool(boss.is_hidden())
            except Exception:
                state["hidden"] = None
            boss_states.append(state)
            cleanup_ok = cleanup_ok and state["dead"] and not state["ai"] and not state["collision"]

        story_defeated = None
        story_evidence = {"source": None, "saved_boss_ids": [], "error": None}
        if _is_valid(self.hidden_story) and hasattr(
            self.hidden_story, "is_main_boss_defeated"
        ):
            story_defeated = bool(
                self.hidden_story.is_main_boss_defeated(unreal.Name("SerpentPython"))
            )
            story_evidence["source"] = "BRHiddenStorySubsystem"
        else:
            saved_ids, save_error = self._saved_boss_ids(DEFAULT_SAVE_SLOT)
            story_defeated = "SerpentPython" in saved_ids
            story_evidence = {
                "source": "SaveSlot_0.DefeatedBossIds",
                "saved_boss_ids": saved_ids,
                "error": save_error,
            }
        cleanup_ok = cleanup_ok and story_defeated
        self._record(
            "boss_cleanup",
            "PASS" if cleanup_ok else "FAIL",
            "The cleared Python arena disabled collision/AI for both dead bosses and recorded story progress."
            if cleanup_ok
            else "Python victory left encounter state, collision, AI, or story progress uncleared.",
            {
                "arena_cleared": arena_cleared,
                "story_defeated": story_defeated,
                "story_evidence": story_evidence,
                "bosses": boss_states,
            },
        )

        try:
            after_mtime = (
                self.default_save_path.stat().st_mtime_ns
                if self.default_save_path.exists()
                else None
            )
        except Exception:
            after_mtime = None
        autosave_ok = after_mtime is not None and (
            self.default_save_mtime_before_boss is None
            or after_mtime > self.default_save_mtime_before_boss
        )
        self._record(
            "boss_autosave",
            "PASS" if autosave_ok else "FAIL",
            "Python victory advanced the default save file after the final boss death."
            if autosave_ok
            else "No new default save write was observed after Python victory.",
            {
                "before_mtime_ns": self.default_save_mtime_before_boss,
                "after_mtime_ns": after_mtime,
                "save_path": str(self.default_save_path),
            },
        )
        self._set_state("SAVE_ROUND_TRIP", 8.0)

    def _state_save_round_trip(self, _world):
        if not _is_valid(self.save_subsystem):
            self._record(
                "save_load_round_trip",
                "UNSUPPORTED",
                "BRSaveGameSubsystem is not available in the PIE GameInstance.",
            )
            self._request_end("save subsystem unavailable")
            return

        self.player.restore_hp_and_stamina()
        self.save_expected_hp = min(321.0, float(self.player.get_max_hp()) - 1.0)
        self.save_expected_stamina = min(
            44.0, float(self.player.get_max_stamina()) - 1.0
        )
        self.player.apply_saved_stats(
            self.save_expected_hp, self.save_expected_stamina
        )
        saved = bool(self.save_subsystem.save_current_game(self.qa_slot, 0))
        exists = bool(self.save_subsystem.does_save_exist(self.qa_slot, 0))
        if not saved or not exists:
            self._record(
                "save_load_round_trip",
                "FAIL",
                "The isolated QA save slot could not be written or validated.",
                {"save_return": saved, "does_save_exist": exists, "slot": self.qa_slot},
            )
            self._request_end("temporary QA save failed")
            return

        mutation = unreal.Vector(
            self.checkpoint_location.x + 1400.0,
            self.checkpoint_location.y + 900.0,
            self.checkpoint_location.z + 200.0,
        )
        self._move_player(mutation)
        self.player.restore_hp_and_stamina()
        # Preserve only primitive identity evidence across OpenLevel.  Keeping
        # PyUObject wrappers from the outgoing world alive until editor
        # teardown is both unnecessary and unsafe.
        self.load_old_player_path = _object_path(self.player)
        self.load_old_player_python_id = id(self.player)
        self.load_old_world_path = _object_path(self.world)
        self.load_old_world_python_id = id(self.world)
        self.load_requested_at = time.monotonic()
        self._unbind_runtime_events(allow_native_unbind=True)
        load_api = self.save_subsystem
        loaded = bool(
            load_api.load_game_from_slot_and_open_level(self.qa_slot, 0)
        )
        del load_api
        if not loaded:
            self._record(
                "save_load_round_trip",
                "FAIL",
                "LoadGameFromSlotAndOpenLevel rejected the isolated QA save.",
                {"slot": self.qa_slot},
            )
            self._request_end("temporary QA load failed")
            return
        self._release_runtime_references()
        self._set_state("WAIT_FOR_LOAD", LOAD_TIMEOUT_SECONDS)

    def _state_wait_for_load(self, world):
        if self._state_elapsed() > self.state_timeout:
            self._record(
                "save_load_round_trip",
                "FAIL",
                "OpenLevel did not produce a restored player before the load timeout.",
            )
            self._request_end("load timeout")
            return

        level_name = str(unreal.GameplayStatics.get_current_level_name(world, True))
        if level_name != MAP_NAME:
            return
        try:
            new_player = unreal.GameplayStatics.get_player_character(world, 0)
        except Exception:
            return
        if not _is_valid(new_player):
            return

        world_wrapper_changed = id(world) != self.load_old_world_python_id
        player_wrapper_changed = id(new_player) != self.load_old_player_python_id
        if not (
            self.load_seen_world_gap
            or world_wrapper_changed
            or player_wrapper_changed
        ):
            return

        self._refresh_core_refs(world)
        if not all(
            _is_valid(item)
            for item in (self.player, self.controller, self.arena, self.vethara, self.aurathos)
        ):
            return

        hp = float(self.player.get_current_hp())
        stamina = float(self.player.get_current_stamina())
        # The replacement pawn can exist one frame before GameMode applies the
        # pending save.  Wait for the saved HP sentinel rather than incorrectly
        # judging that transient default state as a failed load.
        if abs(hp - self.save_expected_hp) > 0.5 and self._state_elapsed() < 8.0:
            return
        location = self.player.get_actor_location()
        location_error = _distance(location, self.checkpoint_location)
        arena_cleared = self._arena_bool("arena_cleared")
        story_defeated = None
        story_evidence = {"source": None, "saved_boss_ids": [], "error": None}
        if _is_valid(self.hidden_story) and hasattr(
            self.hidden_story, "is_main_boss_defeated"
        ):
            story_defeated = bool(
                self.hidden_story.is_main_boss_defeated(unreal.Name("SerpentPython"))
            )
            story_evidence["source"] = "BRHiddenStorySubsystem"
        else:
            saved_ids, save_error = self._saved_boss_ids(self.qa_slot)
            story_defeated = "SerpentPython" in saved_ids
            story_evidence = {
                "source": f"{self.qa_slot}.DefeatedBossIds",
                "saved_boss_ids": saved_ids,
                "error": save_error,
            }
        pending_is_clear = None
        pending_state_observable = False
        if _is_valid(self.save_subsystem) and hasattr(
            self.save_subsystem, "get_pending_save_game"
        ):
            pending_is_clear = not _is_valid(self.save_subsystem.get_pending_save_game())
            pending_state_observable = True

        hp_ok = abs(hp - self.save_expected_hp) <= 0.5
        # Stamina can regenerate between GameMode's next-tick apply and the
        # Slate callback.  It must still be close to the saved value, not full.
        stamina_ok = (
            stamina >= self.save_expected_stamina - 0.5
            and stamina <= self.save_expected_stamina + 8.0
        )
        # The Blueprint-library fallback cannot expose the subsystem's private
        # pending-save pointer.  In that environment, require externally
        # observable application instead: a replacement pawn with saved stats,
        # checkpoint transform, cleared arena, and serialized boss progress.
        pending_ok = pending_is_clear is True if pending_state_observable else True
        restored_ok = (
            hp_ok
            and stamina_ok
            and location_error <= 140.0
            and arena_cleared
            and story_defeated is True
            and pending_ok
        )
        self._record(
            "save_load_round_trip",
            "PASS" if restored_ok else "FAIL",
            "The isolated save reopened L_Runtime_Field and restored player stats, checkpoint transform, and Python clear state."
            if restored_ok
            else "The reopened level did not restore all saved player/checkpoint/Python progression state.",
            {
                "slot": self.qa_slot,
                "world_gap_seen": self.load_seen_world_gap,
                "old_runtime_refs_released": True,
                "old_world_path": self.load_old_world_path,
                "new_world_path": _object_path(world),
                "world_wrapper_changed": world_wrapper_changed,
                "old_player_path": self.load_old_player_path,
                "new_player_path": _object_path(new_player),
                "python_wrapper_changed": player_wrapper_changed,
                "hp_expected": self.save_expected_hp,
                "hp_actual": hp,
                "stamina_expected": self.save_expected_stamina,
                "stamina_actual": stamina,
                "checkpoint_expected": _vector_tuple(self.checkpoint_location),
                "player_location": _vector_tuple(location),
                "location_error_cm": round(location_error, 2),
                "arena_cleared": arena_cleared,
                "story_defeated": story_defeated,
                "story_evidence": story_evidence,
                "pending_save_applied": pending_is_clear,
                "pending_state_observable": pending_state_observable,
            },
        )
        try:
            unreal.GameplayStatics.delete_game_in_slot(self.qa_slot, 0)
        except Exception as exc:
            self.notes.append(f"GameplayStatics could not delete temporary QA slot: {exc}")
        self._request_end("runtime audit complete")


def _start_audit():
    key = "_exception_vertical_slice_runtime_audit"
    previous = getattr(builtins, key, None)
    if previous is not None:
        if not getattr(previous, "finished", False) and _get_game_world() is not None:
            unreal.log_error(
                "[AuditVerticalSliceRuntime] Refusing to re-arm while a previous "
                "audit still owns an active PIE run. Stop PIE externally first."
            )
            return previous
        try:
            stopped = previous.stop("superseded by a new AuditVerticalSliceRuntime run")
            if stopped is False:
                return previous
        except Exception:
            pass
    runner = VerticalSliceRuntimeAudit()
    setattr(builtins, key, runner)
    runner.start()
    return runner


AUDIT = _start_audit()
