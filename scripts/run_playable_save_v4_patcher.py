from pathlib import Path

patcher = Path("scripts/apply_playable_save_v4.py")
source = patcher.read_text()
redundant = '''replace_once(
    "source/PlayableShelterSession.cpp",
    """bool read_v3_payload(const std::vector<std::uint8_t>& body,\n                      PlayableShelterState& state) noexcept {\n""",
    """bool read_v3_payload(const std::vector<std::uint8_t>& body,\n                      PlayableShelterState& state) noexcept {\n""",
)
'''
if redundant not in source:
    raise SystemExit("redundant V3 assertion block was not found")
source = source.replace(redundant, "", 1)
exec(compile(source, str(patcher), "exec"))
