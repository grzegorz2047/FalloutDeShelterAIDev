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

"$BUILD_DIR/core_tests"
echo "host-tests: all core tests passed"
