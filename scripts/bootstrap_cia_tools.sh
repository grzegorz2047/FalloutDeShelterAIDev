#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$ROOT/.tools/src"
BIN_DIR="$ROOT/.tools/bin"
MAKEROM_TAG="makerom-v0.18.4"
BANNERTOOL_COMMIT="16d8c5a0ce02a5e06e64ab42275132fca57c04a2"
HOST_CC="${HOST_CC:-gcc}"
HOST_CXX="${HOST_CXX:-g++}"

mkdir -p "$SOURCE_DIR" "$BIN_DIR"

if [[ ! -x "$BIN_DIR/makerom" ]]; then
  rm -rf "$SOURCE_DIR/Project_CTR"
  git clone --depth 1 --branch "$MAKEROM_TAG" https://github.com/3DSGuy/Project_CTR.git "$SOURCE_DIR/Project_CTR"
  make -C "$SOURCE_DIR/Project_CTR/makerom" -j2 deps CC="$HOST_CC" CXX="$HOST_CXX"
  make -C "$SOURCE_DIR/Project_CTR/makerom" -j2 CC="$HOST_CC" CXX="$HOST_CXX"
  makerom_path="$(find "$SOURCE_DIR/Project_CTR/makerom" -type f -name makerom -perm -111 | head -n 1)"
  test -n "$makerom_path"
  install -m 0755 "$makerom_path" "$BIN_DIR/makerom"
fi

if [[ ! -x "$BIN_DIR/bannertool" ]]; then
  rm -rf "$SOURCE_DIR/bannertool"
  git clone --recurse-submodules https://github.com/diasurgical/bannertool.git "$SOURCE_DIR/bannertool"
  git -C "$SOURCE_DIR/bannertool" checkout --detach "$BANNERTOOL_COMMIT"
  git -C "$SOURCE_DIR/bannertool" submodule update --init --recursive
  make -C "$SOURCE_DIR/bannertool" -j2 CC="$HOST_CC" CXX="$HOST_CXX"
  bannertool_path="$(find "$SOURCE_DIR/bannertool" -type f -name bannertool -perm -111 | head -n 1)"
  test -n "$bannertool_path"
  install -m 0755 "$bannertool_path" "$BIN_DIR/bannertool"
fi

"$BIN_DIR/makerom" -help >/dev/null 2>&1 || true
"$BIN_DIR/bannertool" >/dev/null 2>&1 || true
printf 'CIA tools ready in %s\n' "$BIN_DIR"
