from pathlib import Path
import re
import subprocess
import traceback


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        if label == "V2 migration flag":
            actual = (
                "    result.migrated_from_v1 =\n"
                "        version == kPlayableSaveVersionV1;\n"
                "    return result;\n"
            )
            if actual not in text:
                raise RuntimeError("V2 migration flag exact fallback missing")
            return text.replace(actual, new, 1)
        raise RuntimeError(f"{label} anchor missing")
    return text.replace(old, new, 1)


def commit_failure(message: str) -> None:
    Path("v3-bootstrap-error.txt").write_text(message)
    subprocess.run(
        ["git", "config", "user.name", "github-actions[bot]"], check=True
    )
    subprocess.run(
        [
            "git",
            "config",
            "user.email",
            "41898282+github-actions[bot]@users.noreply.github.com",
        ],
        check=True,
    )
    subprocess.run(["git", "add", "v3-bootstrap-error.txt"], check=True)
    subprocess.run(
        ["git", "commit", "-m", "Capture exact V3 bootstrap failure"],
        check=True,
    )
    subprocess.run(
        ["git", "push", "origin", "HEAD:agent/playable-room-lifecycle"],
        check=True,
    )


try:
    workflow = Path("scripts/apply_room_lifecycle_v3_source.yml").read_text()
    start_marker = "          python3 - <<'PY'\n"
    end_marker = "          PY\n      - name: Run host tests"
    start = workflow.index(start_marker) + len(start_marker)
    end = workflow.index(end_marker, start)
    lines = workflow[start:end].splitlines()
    script = "\n".join(
        line[10:] if line.startswith("          ") else line for line in lines
    ) + "\n"
    match = re.search(
        r"source\s*=\s*Path\('source/PlayableShelterSession\.cpp'\)", script
    )
    if match is None:
        raise RuntimeError("source patch start missing")
    namespace = {"Path": Path, "replace_once": replace_once}
    exec(compile(script[match.start():], "v3_ci_patch", "exec"), namespace)
except Exception:
    failure = traceback.format_exc()
    commit_failure(failure)
    raise
