#!/usr/bin/env bash
set -euo pipefail

display_number=99
while [ -e "/tmp/.X${display_number}-lock" ] || [ -S "/tmp/.X11-unix/X${display_number}" ]; do
  display_number=$((display_number + 1))
done
export DISPLAY=":${display_number}"
echo "Using Xvfb display $DISPLAY"

Xvfb "$DISPLAY" -screen 0 1280x720x24 -nolisten tcp > azahar-xvfb.log 2>&1 &
xvfb_pid=$!
azahar_pid=""

stop_azahar() {
  local pid="${azahar_pid:-}"
  if [ -z "$pid" ]; then
    return 0
  fi

  kill -- -"$pid" 2>/dev/null || true
  for attempt in $(seq 1 40); do
    if ! kill -0 "$pid" 2>/dev/null; then
      wait "$pid" 2>/dev/null || true
      azahar_pid=""
      return 0
    fi
    if ps -o stat= -p "$pid" 2>/dev/null | grep -q '^[[:space:]]*Z'; then
      wait "$pid" 2>/dev/null || true
      azahar_pid=""
      return 0
    fi
    sleep 0.25
  done

  kill -KILL -- -"$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
  azahar_pid=""
}

cleanup() {
  stop_azahar || true
  kill "$xvfb_pid" 2>/dev/null || true
  wait "$xvfb_pid" 2>/dev/null || true
}
trap cleanup EXIT

for attempt in $(seq 1 20); do
  if xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
    break
  fi
  if ! kill -0 "$xvfb_pid" 2>/dev/null; then
    cat azahar-xvfb.log >&2
    exit 1
  fi
  sleep 0.25
done
xdpyinfo -display "$DISPLAY" >/dev/null

HOME="$PWD/azahar-home" setsid ./squashfs-root/AppRun -w "$PWD/dist/DeepShelter3D.3dsx" > azahar-3dsx-launch.log 2>&1 &
azahar_pid=$!
perf_path=""
phase_one_path=""
for second in $(seq 1 30); do
  sleep 1
  if ! kill -0 "$azahar_pid" 2>/dev/null; then
    wait "$azahar_pid" || true
    azahar_pid=""
    echo "Azahar exited before producing the build/transit state." >&2
    cat azahar-3dsx-launch.log >&2
    exit 1
  fi
  perf_path=$(find azahar-home -type f -name 'DeepShelter3D_perf.log' -print -quit)
  phase_one_path=$(find azahar-home -type f -name 'DeepShelter3D_playable_smoke.log' -print -quit)
  if [ -n "$perf_path" ] && [ -n "$phase_one_path" ]; then
    break
  fi
done

if [ -z "$perf_path" ] || [ -z "$phase_one_path" ]; then
  echo "Azahar did not produce smoke readiness markers within 30 seconds." >&2
  timeout 10s xwininfo -display "$DISPLAY" -root -tree > azahar-window-tree.log || true
  cat azahar-window-tree.log >&2 || true
  cat azahar-3dsx-launch.log >&2
  exit 1
fi
cp "$perf_path" azahar-performance.log
grep -q 'DEEP_SHELTER_PERF mode=mono' azahar-performance.log
grep -q 'DEEP_SHELTER_PERF mode=stereo' azahar-performance.log
cat azahar-performance.log

cp "$phase_one_path" azahar-playable-smoke.log
cat azahar-playable-smoke.log
grep -q 'phase=build status=ok' azahar-playable-smoke.log
grep -q 'rooms=5' azahar-playable-smoke.log
grep -q 'credits=180' azahar-playable-smoke.log
grep -q 'elevators=2' azahar-playable-smoke.log
grep -q 'power=2' azahar-playable-smoke.log
grep -q 'food=1' azahar-playable-smoke.log
grep -q 'selected=4' azahar-playable-smoke.log
grep -q 'assigned=4' azahar-playable-smoke.log
grep -q 'resident_state=transit' azahar-playable-smoke.log
grep -q 'movement_ticks=7' azahar-playable-smoke.log
grep -q 'invalid_result=invalid-placement' azahar-playable-smoke.log
grep -q 'invalid_unchanged=1' azahar-playable-smoke.log
grep -q 'first_saved=1' azahar-playable-smoke.log
grep -q 'backup_absent_after_first=1' azahar-playable-smoke.log
grep -q 'backup_rotated=1' azahar-playable-smoke.log
grep -q 'saved=1' azahar-playable-smoke.log
grep -q 'first_save_status=0' azahar-playable-smoke.log
grep -q 'save_status=0' azahar-playable-smoke.log

stop_azahar

HOME="$PWD/azahar-home" setsid ./squashfs-root/AppRun -w "$PWD/dist/DeepShelter3D.3dsx" > azahar-3dsx-resume.log 2>&1 &
azahar_pid=$!
phase_two_path=""
for second in $(seq 1 15); do
  sleep 1
  if ! kill -0 "$azahar_pid" 2>/dev/null; then
    wait "$azahar_pid" || true
    azahar_pid=""
    echo "Azahar exited before validating the resumed state." >&2
    cat azahar-3dsx-resume.log >&2
    exit 1
  fi
  phase_two_path=$(find azahar-home -type f -name 'DeepShelter3D_playable_smoke_resume.log' -print -quit)
  if [ -n "$phase_two_path" ]; then
    break
  fi
done

if [ -z "$phase_two_path" ]; then
  echo "Azahar did not produce the resume marker within 15 seconds." >&2
  timeout 10s xwininfo -display "$DISPLAY" -root -tree > azahar-window-tree.log || true
  cat azahar-window-tree.log >&2 || true
  cat azahar-3dsx-resume.log >&2
  exit 1
fi
cp "$phase_two_path" azahar-playable-smoke-resume.log
cat azahar-playable-smoke-resume.log
grep -q 'phase=resume status=ok' azahar-playable-smoke-resume.log
grep -q 'restored=1' azahar-playable-smoke-resume.log
grep -q 'rooms=5' azahar-playable-smoke-resume.log
grep -q 'credits=180' azahar-playable-smoke-resume.log
grep -q 'selected=1' azahar-playable-smoke-resume.log
grep -q 'assigned=4' azahar-playable-smoke-resume.log
grep -q 'worker_state=working' azahar-playable-smoke-resume.log
grep -q 'worker_column=3' azahar-playable-smoke-resume.log
grep -q 'worker_floor=1' azahar-playable-smoke-resume.log
grep -q 'elevator_lower=1' azahar-playable-smoke-resume.log
grep -q 'elevator_vertical=1' azahar-playable-smoke-resume.log
grep -q 'elevator_upper=1' azahar-playable-smoke-resume.log
grep -q 'idle_moved=1' azahar-playable-smoke-resume.log
grep -q 'idle_assigned=-1' azahar-playable-smoke-resume.log
grep -q 'idle_state=roaming' azahar-playable-smoke-resume.log

timeout 10s xwininfo -display "$DISPLAY" -root -tree > azahar-window-tree.log
if grep -Eq 'An exception occurred|ExceptionRaised|NoExecuteFault' azahar-window-tree.log azahar-3dsx-launch.log azahar-3dsx-resume.log; then
  echo "Azahar reported an emulated application exception." >&2
  cat azahar-window-tree.log >&2
  cat azahar-3dsx-launch.log >&2
  cat azahar-3dsx-resume.log >&2
  exit 1
fi

timeout 10s import -display "$DISPLAY" -window root azahar-first-frame.png
test -s azahar-first-frame.png
convert azahar-first-frame.png -crop 420x250+430+185 +repage \
  -filter point -resize 1260x750 azahar-top-screen-3x.png
convert azahar-first-frame.png -crop 340x250+470+410 +repage \
  -filter point -resize 1020x750 azahar-bottom-screen-3x.png
test "$(identify -format '%wx%h' azahar-top-screen-3x.png)" = "1260x750"
test "$(identify -format '%wx%h' azahar-bottom-screen-3x.png)" = "1020x750"
top_colors=$(identify -format '%k' azahar-top-screen-3x.png)
bottom_colors=$(identify -format '%k' azahar-bottom-screen-3x.png)
echo "Captured resumed free-form shelter with $top_colors colors"
echo "Captured readable lower HUD with $bottom_colors colors"
test "$top_colors" -gt 16
test "$bottom_colors" -gt 16

if grep -Eq 'could not connect to display|X11 connection broke|terminate called|Fatal|Unhandled exception' azahar-3dsx-launch.log azahar-3dsx-resume.log; then
  cat azahar-3dsx-launch.log >&2
  cat azahar-3dsx-resume.log >&2
  exit 1
fi

stop_azahar
