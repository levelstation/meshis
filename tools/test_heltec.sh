#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
MESHIS_HELTEC_PRG=1 sh simulator/build.sh
./build/heltec/simulator --self-test build/heltec/screenshots
MESHIS_HELTEC_PRG=1 SIM_SANITIZE=1 sh simulator/build.sh
./build/heltec/simulator-asan --self-test build/heltec/asan
./build/heltec/simulator --layout-screenshots
sh tools/test_heltec_adapter.sh
