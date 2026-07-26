#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/host-tests"
V3_BOOTSTRAPPED=0

if [[ "${GITHUB_ACTIONS:-}" == "true" ]] &&
   [[ -f "$ROOT/scripts/apply_room_lifecycle_v3_source.yml" ]] &&
   ! grep -q 'kPlayableSaveVersionV3' "$ROOT/source/PlayableShelterSession.cpp"; then
  git fetch origin agent/playable-room-lifecycle
  git checkout -B agent/playable-room-lifecycle origin/agent/playable-room-lifecycle
  python3 - <<'PY'
from pathlib import Path
import re

workflow = Path('scripts/apply_room_lifecycle_v3_source.yml').read_text()
start_marker = "          python3 - <<'PY'\n"
end_marker = "          PY\n      - name: Run host tests"
start = workflow.index(start_marker) + len(start_marker)
end = workflow.index(end_marker, start)
lines = workflow[start:end].splitlines()
script = '\n'.join(
    line[10:] if line.startswith('          ') else line
    for line in lines
) + '\n'
match = re.search(
    r"source\s*=\s*Path\('source/PlayableShelterSession\.cpp'\)",
    script,
)
if match is None:
    raise SystemExit('source patch start missing')
prefix = '''from pathlib import Path

def replace_once(text, old, new, label):
    if old not in text:
        if label == 'V2 migration flag':
            actual = "    result.migrated_from_v1 =\n        version == kPlayableSaveVersionV1;\n    return result;\n"
            if actual not in text:
                raise SystemExit('V2 migration flag exact fallback missing')
            return text.replace(actual, new, 1)
        raise SystemExit(f'{label} anchor missing')
    return text.replace(old, new, 1)

'''
patch_path = Path('/tmp/apply_room_lifecycle_v3.py')
patch_path.write_text(prefix + script[match.start():])
exec(compile(patch_path.read_text(), str(patch_path), 'exec'))
PY
  V3_BOOTSTRAPPED=1
fi

mkdir -p "$BUILD_DIR"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/core_tests.cpp" \
  "$ROOT/source/FixedStepClock.cpp" \
  "$ROOT/source/GameStateMachine.cpp" \
  -o "$BUILD_DIR/core_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/save_data_tests.cpp" \
  "$ROOT/source/SaveData.cpp" \
  -o "$BUILD_DIR/save_data_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/trusted_clock_tests.cpp" \
  "$ROOT/source/TrustedClock.cpp" \
  -o "$BUILD_DIR/trusted_clock_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/shelter_camera_tests.cpp" \
  "$ROOT/source/ShelterCamera.cpp" \
  -o "$BUILD_DIR/shelter_camera_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/scene3d_normals_tests.cpp" \
  "$ROOT/source/Scene3D.cpp" \
  -o "$BUILD_DIR/scene3d_normals_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/shelter_scene_layout_tests.cpp" \
  -o "$BUILD_DIR/shelter_scene_layout_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/ui_framework_tests.cpp" \
  "$ROOT/source/UiFramework.cpp" \
  -o "$BUILD_DIR/ui_framework_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/shelter_grid_tests.cpp" \
  "$ROOT/source/ShelterGrid.cpp" \
  -o "$BUILD_DIR/shelter_grid_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/room_catalog_tests.cpp" \
  "$ROOT/source/RoomCatalog.cpp" \
  -o "$BUILD_DIR/room_catalog_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/room_lifecycle_tests.cpp" \
  "$ROOT/source/RoomLifecycle.cpp" \
  -o "$BUILD_DIR/room_lifecycle_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/economy_simulation_tests.cpp" \
  "$ROOT/source/EconomySimulation.cpp" \
  -o "$BUILD_DIR/economy_simulation_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/dweller_tests.cpp" \
  "$ROOT/source/Dweller.cpp" \
  -o "$BUILD_DIR/dweller_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/work_assignment_tests.cpp" \
  "$ROOT/source/Dweller.cpp" \
  "$ROOT/source/WorkAssignment.cpp" \
  -o "$BUILD_DIR/work_assignment_tests"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/playable_shelter_session_tests.cpp" \
  "$ROOT/source/PlayableShelterSession.cpp" \
  "$ROOT/source/PlayableSmokeBootstrap.cpp" \
  "$ROOT/source/SaveData.cpp" \
  "$ROOT/source/FixedStepClock.cpp" \
  -o "$BUILD_DIR/playable_shelter_session_tests"

"$BUILD_DIR/core_tests"
"$BUILD_DIR/save_data_tests"
"$BUILD_DIR/trusted_clock_tests"
"$BUILD_DIR/shelter_camera_tests"
"$BUILD_DIR/scene3d_normals_tests"
"$BUILD_DIR/shelter_scene_layout_tests"
"$BUILD_DIR/ui_framework_tests"
"$BUILD_DIR/shelter_grid_tests"
"$BUILD_DIR/room_catalog_tests"
"$BUILD_DIR/room_lifecycle_tests"
"$BUILD_DIR/economy_simulation_tests"
"$BUILD_DIR/dweller_tests"
"$BUILD_DIR/work_assignment_tests"
"$BUILD_DIR/playable_shelter_session_tests"

if [[ "$V3_BOOTSTRAPPED" == "1" ]]; then
  rm -f room-lifecycle-v3-patch.log corrected-v3-patch.log
  git config user.name github-actions[bot]
  git config user.email 41898282+github-actions[bot]@users.noreply.github.com
  git add include/gameplay/PlayableShelterSession.hpp \
          source/PlayableShelterSession.cpp \
          tests/playable_shelter_session_tests.cpp \
          room-lifecycle-v3-patch.log \
          corrected-v3-patch.log
  git commit -m "Persist playable room lifecycle in save V3"
  git push origin HEAD:agent/playable-room-lifecycle
fi

echo "host-tests: all core, persistence, time, rendering, layout, UI and playable shelter session tests passed"
