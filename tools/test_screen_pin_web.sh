#!/bin/sh
# Existing Emscripten SDK must be activated. Local-only UI/WASM tests, no devices.
set -eu
cd "$(dirname "$0")/.."
sh tools/build_web_simulator.sh
sh tools/test_web_simulator.sh
sh tools/test_heltec_web.sh
node tests/screen_pin_web_test.mjs wio
node tests/screen_pin_web_test.mjs heltec
python3 tools/package_heltec_web.py
node tests/heltec_browser_input_test.mjs
