#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/web
flags='-DMESHIS_BOARD=1 -DMESHIS_DEFAULT_PET=0'
if [ -f firmware/ui/LuAlert.h ];then flags='-DLU_ALERT_BRANDING=1';fi
c++ -std=c++17 -O2 -Wno-deprecated-declarations -funsigned-char -DARDUINO=10800 -DMESHIS_V2=1 -DMESHIS_LIVE_RADIO=1 -DBRAND_CLASSIC_UI=1 $flags \
 -I simulator/src -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I vendor/meshcore -I firmware/core -I firmware/ui -I firmware/display \
 simulator/tests/WebOracle.cpp simulator/src/Simulator.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp -o build/web/native-oracle
node tests/web_simulator_test.mjs
