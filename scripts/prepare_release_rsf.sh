#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <source.rsf> <output.rsf>" >&2
  exit 2
fi

mkdir -p "$(dirname "$2")"
sed \
  -e '/^[[:space:]]*SystemModeExt[[:space:]]*:/d' \
  -e '/^[[:space:]]*CpuSpeed[[:space:]]*:/d' \
  -e '/^[[:space:]]*EnableL2Cache[[:space:]]*:/d' \
  -e '/^[[:space:]]*CanAccessCore2[[:space:]]*:/d' \
  "$1" > "$2"
