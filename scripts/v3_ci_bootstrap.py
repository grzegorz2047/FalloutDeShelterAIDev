from pathlib import Path
import re


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
