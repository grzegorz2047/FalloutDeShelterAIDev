#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build/host-tests"
mkdir -p "$BUILD_DIR"

${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" \
  "$ROOT/tests/generated_material_atlas_tests.cpp" \
  "$ROOT/source/GeneratedMaterialAtlas.cpp" \
  -o "$BUILD_DIR/generated_material_atlas_tests"

"$BUILD_DIR/generated_material_atlas_tests"
echo "generated-asset-tests: atlas decode and PICA tiling passed"
