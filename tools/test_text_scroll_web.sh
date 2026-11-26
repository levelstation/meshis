#!/bin/sh
# Wio-only build and real native/WASM parity. Heltec assets are untouched.
set -eu
cd "$(dirname "$0")/.."
sh tools/build_web_simulator.sh
sh tools/test_web_simulator.sh
node tests/screen_pin_web_test.mjs wio
node tests/text_scroll_web_test.mjs
