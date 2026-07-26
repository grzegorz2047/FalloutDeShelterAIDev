#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH="$ROOT/scripts/apply_room_lifecycle_v3_verified.py"
STANDARD_RUNNER="/tmp/deep-shelter-standard-host-tests.sh"
CLEAN_BASE="eaa1b18ce955ac66c97122fc1d3c5312aed4cb0d"

git show "$CLEAN_BASE:scripts/run_host_tests.sh" > "$STANDARD_RUNNER"
chmod +x "$STANDARD_RUNNER"

if [[ "${GITHUB_ACTIONS:-}" == "true" && -f "$PATCH" ]]; then
  python3 "$PATCH"
  bash "$STANDARD_RUNNER"

  if [[ "${GITHUB_EVENT_NAME:-}" == "push" ]]; then
    cp "$STANDARD_RUNNER" scripts/run_host_tests.sh
    rm -f scripts/apply_room_lifecycle_v3_verified.py
    git config user.name github-actions[bot]
    git config user.email 41898282+github-actions[bot]@users.noreply.github.com
    git add source/PlayableShelterSession.cpp tests/playable_shelter_session_tests.cpp scripts/run_host_tests.sh
    git add -u scripts/apply_room_lifecycle_v3_verified.py
    git commit -m "Persist playable room lifecycle in save V3"
    git push origin HEAD:agent/room-lifecycle-v3-verified
  fi
  exit 0
fi

bash "$STANDARD_RUNNER"
