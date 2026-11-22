#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build simulator/dist
MESHIS_CXX=$(xcrun --find clang++)
MESHIS_SDK=$(xcrun --show-sdk-path)
MESHIS_FLAGS='-O2'
MESHIS_FEATURE_FLAGS='-DMESHIS_V2=1'
if [ "${MESHIS_LEGACY_UI:-0}" = 1 ]; then MESHIS_FEATURE_FLAGS='';fi
MESHIS_OUTPUT='meshis-simulator'
if [ "${SIM_SANITIZE:-0}" = 1 ]; then MESHIS_FLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer';MESHIS_OUTPUT='build/meshis-simulator-asan';fi
"$MESHIS_CXX" -isysroot "$MESHIS_SDK" -mmacosx-version-min=14.0 -std=c++17 $MESHIS_FLAGS $MESHIS_FEATURE_FLAGS -funsigned-char -fobjc-arc -DARDUINO=10800 -Wno-deprecated-declarations -DBRAND_CLASSIC_UI=1 -DMESHIS_BOARD=1 -DMESHIS_DEFAULT_PET=0 \
  "-ffile-prefix-map=$PWD=." "-fdebug-prefix-map=$PWD=." \
  -I simulator/src -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I vendor/meshcore -I firmware/core -I firmware/ui -I firmware/display \
  simulator/src/main.mm simulator/src/ImageExport.mm simulator/src/Simulator.cpp simulator/tests/SimulatorTests.cpp simulator/tests/Meshis2Tests.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp -framework Cocoa -o "$MESHIS_OUTPUT"
if [ "${SIM_SANITIZE:-0}" = 1 ]; then exit 0;fi
MESHIS_APP='simulator/dist/Meshis Simulator.app/Contents'
mkdir -p "$MESHIS_APP/MacOS" "$MESHIS_APP/Resources"
cp "$MESHIS_OUTPUT" "$MESHIS_APP/MacOS/meshis-simulator"
cp LICENSE "$MESHIS_APP/Resources/MeshCore-LICENSE.txt"
cp simulator/vendor/adafruit-gfx/license.txt "$MESHIS_APP/Resources/Adafruit-GFX-LICENSE.txt"
python3 - <<'PLIST'
import plistlib
from pathlib import Path
path=Path('simulator/dist/Meshis Simulator.app/Contents/Info.plist')
with path.open('wb') as f:plistlib.dump({'CFBundleExecutable':'meshis-simulator','CFBundleIdentifier':'org.meshis.simulator','CFBundleName':'Meshis Simulator','CFBundlePackageType':'APPL','CFBundleShortVersionString':'0.2.0','CFBundleVersion':'20261122','LSMinimumSystemVersion':'14.0','NSHighResolutionCapable':True},f)
PLIST
codesign --force --sign - 'simulator/dist/Meshis Simulator.app' >/dev/null 2>&1
codesign --verify --deep --strict 'simulator/dist/Meshis Simulator.app'
