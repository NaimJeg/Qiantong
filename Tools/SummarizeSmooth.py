"""Summarize UE's CSV capture and fresh PIE evidence; no custom timing instrumentation."""
import csv
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/exec/evidence/smooth-performance.json"

def distribution(values):
    values = sorted(values)
    assert values
    return {"p50": values[math.ceil(len(values)*.5)-1],
            "p95": values[math.ceil(len(values)*.95)-1], "max": values[-1]}

report = {"source": "UE CSV profiler; timings in milliseconds; scope values are inclusive totals per render frame",
          "mode": "Editor floating PIE", "resolution": [720, 1280],
          "scenario": {"allies": 5, "preplacedEnemies": 21, "seed": 1, "protocol": 3},
          "warmupSeconds": 8, "runs": []}
device = json.loads((ROOT / "Saved/Automation/Demo/index.json").read_text(encoding="utf-8-sig"))["devices"][0]
report["hardware"] = {k: device[k] for k in ["cPUModel", "gPU", "rAMInGB", "oSVersion"]}
report["displays"] = json.loads((ROOT / "Saved/Logs/Smooth-Display.json").read_text(encoding="utf-8-sig"))
view_samples = {}
for path in sorted((ROOT / "Saved/SmoothPIE").glob("smooth-*.json")):
    evidence = json.loads(path.read_text())
    assert evidence["goldenMatch"]
    with (ROOT / "Saved/Profiling/CSV" / (path.stem + ".csv")).open() as stream:
        rows = list(csv.DictReader(stream))
    # Retain all active battle frames after two initial ticks. Do not exclude slow frames.
    rows = [r for r in rows if (r.get("Qiantong/SimTick") or "").replace(".", "", 1).isdigit()
            and 2 <= float(r["Qiantong/SimTick"]) < 277]
    metrics = {}
    for key in ["FrameTime", "GameThreadTime", "GPUTime"] + [
        "Qiantong/GameThread/" + s for s in
        ["QT_BattleStep", "QT_AdvanceCover", "QT_NextCell", "QT_ClearRay", "QT_SyncViews", "QT_DrawHUD"]]:
        metrics[key] = distribution([float(r.get(key) or 0) for r in rows])
    changes = [b[1] - a[1] for a, b in zip(evidence["samples"], evidence["samples"][1:])
               if .8 < a[0]*evidence["speed"] < 2.5]  # visible first-wave movement, before camera transitions
    moving = [abs(x) for x in changes if abs(x) > 1.e-5]
    run = {"label": path.stem, "frames": len(rows), "eventHash": evidence["eventHash"],
           "actorCount": len(evidence["actors"]), "actorIdentityPreserved": True,
           "timingMs": metrics, "simStepsPerFrameMax": max(float(r["Qiantong/SimSteps"]) for r in rows),
           "fps": distribution([1000/float(r["FrameTime"]) for r in rows]),
           "backlogSecondsMax": max(float(r["Qiantong/BacklogSeconds"]) for r in rows),
           "viewSyncsPerFrameMax": max(float(r["Qiantong/ViewSyncs"]) for r in rows),
           "firstWaveMovingFrameDeltas": distribution(moving) if moving else None,
           "sampleFile": str(path.relative_to(ROOT)).replace("\\", "/")}
    assert run["viewSyncsPerFrameMax"] == 1
    report["runs"].append(run)
    view_samples[path.stem] = {"columns": evidence["columns"], "speed": evidence["speed"],
                              "samples": [s for s in evidence["samples"] if .8 < s[0]*evidence["speed"] < 2.5]}
assert len(report["runs"]) == 4
OUT.write_text(json.dumps(report, indent=2), encoding="utf-8")
(OUT.parent / "smooth-view-samples.json").write_text(json.dumps(view_samples, indent=2), encoding="utf-8")
print(OUT)
