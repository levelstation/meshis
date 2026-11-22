#!/bin/sh
# Headless cross-platform parity check; does not modify the published Wio WASM.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/heltec/wasm
flags='-DMESHIS_BOARD=1 -DMESHIS_DEFAULT_PET=0'
if [ -f firmware/ui/LuAlert.h ];then flags='-DLU_ALERT_BRANDING=1';fi
includes='-I simulator/src -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I vendor/meshcore -I firmware/core -I firmware/ui -I firmware/display'
common="-std=c++17 -O2 -funsigned-char -DARDUINO=10800 -DMESHIS_V2=1 -DBRAND_CLASSIC_UI=1 -DMESHIS_HELTEC_PRG=1 $flags $includes"
c++ $common -Wno-deprecated-declarations simulator/tests/WebOracle.cpp simulator/src/Simulator.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp -o build/heltec/native-oracle
em++ $common "-ffile-prefix-map=$PWD=." "-fdebug-prefix-map=$PWD=." simulator/web/Bridge.cpp simulator/src/Simulator.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp --no-entry -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sDYNAMIC_EXECUTION=0 -sEXPORTED_RUNTIME_METHODS=UTF8ToString,HEAPU8 -sINITIAL_MEMORY=16777216 -sSTACK_SIZE=262144 -sFILESYSTEM=0 -o build/heltec/wasm/core.mjs
node tests/heltec_web_test.mjs
