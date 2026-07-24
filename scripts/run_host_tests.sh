#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/host-tests"
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
  "$ROOT/tests/dweller_tests.cpp" \
  "$ROOT/source/Dweller.cpp" \
  -o "$BUILD_DIR/dweller_tests"

"$BUILD_DIR/core_tests"
"$BUILD_DIR/save_data_tests"
"$BUILD_DIR/trusted_clock_tests"
"$BUILD_DIR/shelter_camera_tests"
"$BUILD_DIR/ui_framework_tests"
"$BUILD_DIR/shelter_grid_tests"
"$BUILD_DIR/room_catalog_tests"
"$BUILD_DIR/room_lifecycle_tests"
"$BUILD_DIR/dweller_tests"
echo "host-tests: all core, persistence, time, rendering, UI, shelter, room and dweller tests passed"
