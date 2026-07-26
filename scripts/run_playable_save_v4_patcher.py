from pathlib import Path

patcher = Path("scripts/apply_playable_save_v4.py")
lines = patcher.read_text().splitlines(keepends=True)
removed = False
for index in range(len(lines) - 4):
    if (lines[index].strip() == "replace_once(" and
            "source/PlayableShelterSession.cpp" in lines[index + 1] and
            "bool read_v3_payload" in lines[index + 2] and
            "bool read_v3_payload" in lines[index + 3] and
            lines[index + 4].strip() == ")"):
        del lines[index:index + 5]
        removed = True
        break
if not removed:
    raise SystemExit("redundant V3 assertion block was not found")
source = "".join(lines)
exec(compile(source, str(patcher), "exec"))
