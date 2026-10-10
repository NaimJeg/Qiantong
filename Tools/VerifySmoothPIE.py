"""Editor-only DEMO-04 acceptance. Launch with -DemoOption=3 and -ExecutePythonScript, then MCP StartPIE.

Uses UE CSV/Insights for timing; samples actual Actor transforms only for view evidence.
No assets or user settings are saved. Output: Saved/SmoothPIE and Saved/Profiling/CSV.
"""
import json
import pathlib
import time
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/SmoothPIE"
OUT.mkdir(parents=True, exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
exec(compile((ROOT / "Tools/VerifyDemoAssets.py").read_text(encoding="utf-8"), "VerifyDemoAssets.py", "exec"))
unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings")).set_editor_property("bThrottleCPUWhenNotForeground", False)
settings = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.LevelEditorPlaySettings"))
settings.set_editor_property("NewWindowWidth", 720)
settings.set_editor_property("NewWindowHeight", 1280)

scenarios = [(60, 1, True), (120, 1, True), (60, 10, True), (60, 1, False)]
state = {"phase": "wait", "index": 0, "samples": [], "last": time.perf_counter()}

def command(world, text):
    unreal.SystemLibrary.execute_console_command(world, text)

def actors(world):
    return unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/QiantongUE.DemoUnitView"))

def identities(world):
    return {str(a.tags[0]): a.get_path_name() for a in actors(world)}

def tick(delta):
    try:
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        if not worlds:
            return
        world = worlds[0]
        now = time.perf_counter()
        if state["phase"] == "wait":
            command(world, "t.IdleWhenNotForeground 0")
            command(world, "r.VSync 0")
            command(world, "r.GPUCsvStatsEnabled 1")
            command(world, "qt.demo squad")
            state.update(phase="warmup", last=now)
        elif state["phase"] == "warmup" and now-state["last"] > 8:
            state["phase"] = "start"
        elif state["phase"] == "start":
            fps, speed, interpolate = scenarios[state["index"]]
            label = f"smooth-{fps}fps-{speed}x-{'on' if interpolate else 'off'}"
            command(world, f"t.MaxFPS {fps}")
            command(world, "qt.demo restart")
            command(world, f"qt.demo speed {speed}")
            command(world, f"qt.demo interpolation {int(interpolate)}")
            state.update(phase="run", last=now, label=label, samples=[], chosen=False,
                         started=time.time(), before=identities(world), units=actors(world))
            command(world, f"CsvProfile STARTFILE={label}.csv")
            command(world, "CsvProfile START")
            (OUT / "current.json").write_text(json.dumps({"label": label, "started": time.time()}))
        elif state["phase"] == "run":
            elapsed = now-state["last"]
            fps, speed, interpolate = scenarios[state["index"]]
            # The first ally enters, turns, and crosses both camera transitions.
            unit = state["units"][0]
            p, r = unit.get_actor_location(), unit.get_actor_rotation()
            state["samples"].append([elapsed, p.x, p.y, r.yaw])
            result_path = ROOT / "Saved/DemoResults/continuous-5-seed1-option3.json"
            if elapsed > 16/speed + 1:
                command(world, "CsvProfile STOP")
                command(world, "qt.demo export")
                assert result_path.stat().st_mtime >= state["started"], "No fresh result for this scenario"
                result = json.loads(result_path.read_text(encoding="utf-8-sig"))
                golden = json.loads((ROOT / "Tests/Golden/continuous-5-option3.json").read_text())
                assert result == golden, f"PIE Golden mismatch: {state['label']}"
                assert identities(world) == state["before"], "Actor identity changed between waves"
                evidence = {"label": state["label"], "fpsCap": fps, "speed": speed,
                            "interpolation": interpolate, "startSimTick": 0, "goldenMatch": True,
                            "eventHash": result["eventHash"], "actors": state["before"],
                            "columns": ["elapsedSeconds", "actorWorldX", "actorWorldY", "yawDegrees"],
                            "samples": state["samples"]}
                (OUT / (state["label"] + ".json")).write_text(json.dumps(evidence))
                unreal.log("QIANTONG_SMOOTH_PIE_PASS: " + state["label"])
                state.update(phase="gap", last=now)
        elif state["phase"] == "gap" and now-state["last"] > 2:
            state["index"] += 1
            if state["index"] == len(scenarios):
                state["phase"] = "done"
                (OUT / "complete.json").write_text(json.dumps({"passed": len(scenarios)}))
                unreal.unregister_slate_post_tick_callback(handle)
            else:
                state["phase"] = "start"
    except Exception as error:
        (OUT / "error.txt").write_text(repr(error))
        unreal.log_error("QIANTONG_SMOOTH_PIE_FAILED: " + repr(error))
        unreal.unregister_slate_post_tick_callback(handle)

handle = unreal.register_slate_post_tick_callback(tick)
unreal.log("QIANTONG_SMOOTH_PIE_READY: waiting for MCP StartPIE")
