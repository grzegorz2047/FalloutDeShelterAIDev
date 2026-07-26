#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH="$ROOT/scripts/apply_demolition_connectivity_guard.py"
STANDARD_RUNNER="$ROOT/scripts/.run_host_tests_connectivity_standard.sh"
CLEAN_BASE="26e2fad616d46fa2ef29dc9e54dc8d1e65f6aed8"
VERIFY_BRANCH="agent/demolition-connectivity-guard"
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
  cp include/gameplay/PlayableShelterSession.hpp /tmp/PlayableShelterSession.hpp
  cp source/PlayableShelterSession.cpp /tmp/PlayableShelterSession.cpp
  cp source/main.cpp /tmp/main.cpp
  cp tests/playable_shelter_session_tests.cpp /tmp/playable_shelter_session_tests.cpp
  git fetch origin "$VERIFY_BRANCH"
  git checkout -B "$VERIFY_BRANCH" "origin/$VERIFY_BRANCH"
  cp /tmp/PlayableShelterSession.hpp include/gameplay/PlayableShelterSession.hpp
  cp /tmp/PlayableShelterSession.cpp source/PlayableShelterSession.cpp
  cp /tmp/main.cpp source/main.cpp
  cp /tmp/playable_shelter_session_tests.cpp tests/playable_shelter_session_tests.cpp
  git show "$CLEAN_BASE:scripts/run_host_tests.sh" > scripts/run_host_tests.sh
  rm -f scripts/apply_demolition_connectivity_guard.py
  git config user.name github-actions[bot]
  git config user.email 41898282+github-actions[bot]@users.noreply.github.com
  git add include/gameplay/PlayableShelterSession.hpp source/PlayableShelterSession.cpp source/main.cpp tests/playable_shelter_session_tests.cpp scripts/run_host_tests.sh
  git add -u scripts/apply_demolition_connectivity_guard.py
  git commit -m "Block demolition that disconnects shelter"
  git push origin HEAD:"$VERIFY_BRANCH"
fi
