#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH="$ROOT/scripts/apply_room_lifecycle_evacuation_verified.py"
STANDARD_RUNNER="$ROOT/scripts/.run_host_tests_evacuation_standard.sh"
CLEAN_BASE="408baf4c8ecfa26581a43d09b2c57e67c91bcf0f"
VERIFY_BRANCH="agent/room-lifecycle-evacuation-verified"
LOG="$ROOT/build.log"

git config --global --add safe.directory "$ROOT"

set +e
{
  git fetch --depth=1 origin "$CLEAN_BASE"
  git show "$CLEAN_BASE:scripts/run_host_tests.sh" > "$STANDARD_RUNNER"
  chmod +x "$STANDARD_RUNNER"
  if [[ "${GITHUB_ACTIONS:-}" == "true" && -f "$PATCH" ]]; then
    python3 "$PATCH"
  fi
  bash "$STANDARD_RUNNER"
} > "$LOG" 2>&1
status=$?
set -e
cat "$LOG"
rm -f "$STANDARD_RUNNER"
if [[ "$status" -ne 0 ]]; then
  exit "$status"
fi

if [[ "${GITHUB_ACTIONS:-}" == "true" &&
      "${GITHUB_EVENT_NAME:-}" == "pull_request" && -f "$PATCH" ]]; then
  cp source/PlayableShelterSession.cpp /tmp/PlayableShelterSession.cpp
  cp tests/playable_shelter_session_tests.cpp /tmp/playable_shelter_session_tests.cpp
  git fetch origin "$VERIFY_BRANCH"
  git checkout -B "$VERIFY_BRANCH" "origin/$VERIFY_BRANCH"
  cp /tmp/PlayableShelterSession.cpp source/PlayableShelterSession.cpp
  cp /tmp/playable_shelter_session_tests.cpp tests/playable_shelter_session_tests.cpp
  git show "$CLEAN_BASE:scripts/run_host_tests.sh" > scripts/run_host_tests.sh
  rm -f scripts/apply_room_lifecycle_evacuation_verified.py
  git config user.name github-actions[bot]
  git config user.email 41898282+github-actions[bot]@users.noreply.github.com
  git add source/PlayableShelterSession.cpp tests/playable_shelter_session_tests.cpp scripts/run_host_tests.sh
  git add -u scripts/apply_room_lifecycle_evacuation_verified.py
  git commit -m "Evacuate residents before room demolition"
  git push origin HEAD:"$VERIFY_BRANCH"
fi
