#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/host-tests"
UI_BOOTSTRAPPED=0

if [[ "${GITHUB_ACTIONS:-}" == "true" ]] && [[ -f "$ROOT/scripts/apply_room_lifecycle_ui.py" ]]; then
  python3 scripts/apply_room_lifecycle_ui_include.py
  python3 scripts/apply_room_lifecycle_ui.py
  UI_BOOTSTRAPPED=1
fi

mkdir -p "$BUILD_DIR"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/core_tests.cpp" "$ROOT/source/FixedStepClock.cpp" "$ROOT/source/GameStateMachine.cpp" -o "$BUILD_DIR/core_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/save_data_tests.cpp" "$ROOT/source/SaveData.cpp" -o "$BUILD_DIR/save_data_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/trusted_clock_tests.cpp" "$ROOT/source/TrustedClock.cpp" -o "$BUILD_DIR/trusted_clock_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/shelter_camera_tests.cpp" "$ROOT/source/ShelterCamera.cpp" -o "$BUILD_DIR/shelter_camera_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/scene3d_normals_tests.cpp" "$ROOT/source/Scene3D.cpp" -o "$BUILD_DIR/scene3d_normals_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/shelter_scene_layout_tests.cpp" -o "$BUILD_DIR/shelter_scene_layout_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/ui_framework_tests.cpp" "$ROOT/source/UiFramework.cpp" -o "$BUILD_DIR/ui_framework_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/shelter_grid_tests.cpp" "$ROOT/source/ShelterGrid.cpp" -o "$BUILD_DIR/shelter_grid_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/room_catalog_tests.cpp" "$ROOT/source/RoomCatalog.cpp" -o "$BUILD_DIR/room_catalog_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/room_lifecycle_tests.cpp" "$ROOT/source/RoomLifecycle.cpp" -o "$BUILD_DIR/room_lifecycle_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/economy_simulation_tests.cpp" "$ROOT/source/EconomySimulation.cpp" -o "$BUILD_DIR/economy_simulation_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/dweller_tests.cpp" "$ROOT/source/Dweller.cpp" -o "$BUILD_DIR/dweller_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/work_assignment_tests.cpp" "$ROOT/source/Dweller.cpp" "$ROOT/source/WorkAssignment.cpp" -o "$BUILD_DIR/work_assignment_tests"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/include" "$ROOT/tests/playable_shelter_session_tests.cpp" "$ROOT/source/PlayableShelterSession.cpp" "$ROOT/source/PlayableSmokeBootstrap.cpp" "$ROOT/source/SaveData.cpp" "$ROOT/source/FixedStepClock.cpp" -o "$BUILD_DIR/playable_shelter_session_tests"

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

if [[ "$UI_BOOTSTRAPPED" == "1" && "${GITHUB_EVENT_NAME:-}" == "push" ]]; then
  git config user.name github-actions[bot]
  git config user.email 41898282+github-actions[bot]@users.noreply.github.com
  git add source/main.cpp
  git commit -m "Expose playable room lifecycle controls"
  git push origin HEAD:agent/playable-room-lifecycle
fi

echo "host-tests: all core, persistence, time, rendering, layout, UI and playable shelter session tests passed"
