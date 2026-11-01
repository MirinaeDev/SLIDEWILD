#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
  echo "This script requires Apple Silicon. Open a native Terminal (without Rosetta)." >&2
  exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
  echo "Install Apple tools: xcode-select --install" >&2; exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "Install CMake >= 3.24, then retry (for example: brew install cmake)." >&2; exit 1
fi
BUILD="$ROOT/build-macos"
mkdir -p "$BUILD"
exec > >(tee "$BUILD/build.log") 2>&1
cmake -S "$ROOT" -B "$BUILD" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DDH_FETCH_SDL=ON -DBUILD_TESTING=ON
# Keep memory usage bounded on entry-level 8 GB Macs.
cmake --build "$BUILD" --parallel "${BUILD_JOBS:-4}"
ctest --test-dir "$BUILD" --output-on-failure
APP="$BUILD/Slidewild.app"
lipo "$APP/Contents/MacOS/Slidewild" -verify_arch arm64
otool -L "$APP/Contents/MacOS/Slidewild"
if otool -L "$APP/Contents/MacOS/Slidewild" | grep -Eq '/opt/homebrew|/usr/local|@rpath/.*SDL'; then
  echo "Unexpected external library: the application is not self-contained." >&2; exit 1
fi
# Local ad-hoc signature only; this is not Developer ID signing/notarization.
codesign --force --sign - "$APP"
codesign --verify --strict --verbose=2 "$APP"
printf '\nBuild complete: %s\n' "$APP"
printf 'Next step: bash "%s/test-macos.sh"\n' "$ROOT"
