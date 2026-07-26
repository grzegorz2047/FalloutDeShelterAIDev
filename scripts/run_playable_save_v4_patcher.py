from pathlib import Path
import re

patcher = Path("scripts/apply_playable_save_v4.py")
lines = patcher.read_text().splitlines(keepends=True)


def remove_replace_block(marker: str) -> None:
    for index in range(len(lines) - 4):
        block = "".join(lines[index:index + 5])
        if lines[index].strip() == "replace_once(" and marker in block:
            del lines[index:index + 5]
            return
    raise SystemExit(f"redundant patch block was not found: {marker}")


remove_replace_block("bool read_v3_payload")
remove_replace_block("? read_v1_payload(body, state)")
source = "".join(lines)
exec(compile(source, str(patcher), "exec"))

cpp_path = Path("source/PlayableShelterSession.cpp")
cpp = cpp_path.read_text()
pattern = re.compile(
    r"    const bool parsed =\n"
    r"\s*version == kPlayableSaveVersionV1\n"
    r"\s*\? read_v1_payload\(body, state\)\n"
    r"\s*: \(version == kPlayableSaveVersionV2\n"
    r"\s*\? read_v2_payload\(body, state\)\n"
    r"\s*: read_v3_payload\(body, state\)\);"
)
replacement = """    const bool parsed =
        version == kPlayableSaveVersionV1
            ? read_v1_payload(body, state)
            : (version == kPlayableSaveVersionV2
                   ? read_v2_payload(body, state)
                   : (version == kPlayableSaveVersionV3
                          ? read_v3_payload(body, state)
                          : read_v4_payload(body, state)));
"""
cpp, count = pattern.subn(replacement.rstrip("\n"), cpp, count=1)
if count != 1:
    raise SystemExit("save decoder selection block was not found")
cpp_path.write_text(cpp)
