#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
APP="$ROOT/build-macos/Slidewild.app"
EXE="$APP/Contents/MacOS/Slidewild"
if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 || ! -x "$EXE" ]]; then
  echo "First run bash build-macos.sh on Apple Silicon." >&2; exit 1
fi
RESULTS="$ROOT/build-macos/native-check"
mkdir -p "$RESULTS"
# Work outside the bundle: proves that SDL resolves Contents/Resources.
cd "$RESULTS"
for course in 0 1 2 3 4 5 6 7; do
  "$EXE" --smoke --course "$course" --capture --frames 2
  cp run.log "course-$course.log"
  cp captures/latest.bmp "course-$course.bmp"
  grep -q 'GPU metal' run.log
  grep -q 'Exit clean' run.log
done
"$EXE" --session-test --vertex-test --course 4 --rain --frames 180
cp run.log session-and-capacity.log
grep -q 'Dynamic capacity 800000' run.log
"$EXE" --tour-test --course 6 --race --difficulty 2 --capture --frames 10000
cp run.log full-race.log
cp captures/latest.bmp full-race.bmp
grep -q 'Finish runout 1' run.log
grep -q 'Menu page 5' run.log
echo "Native checks complete. Captures and logs: $RESULTS"
echo "Also check graphics, audio, controllers, fullscreen and Retina manually."
