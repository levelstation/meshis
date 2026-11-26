#!/bin/sh
# Separate Heltec browser target; never overwrites Wio WebAssembly or downloads.
set -eu
cd "$(dirname "$0")/.."
sh tools/test_heltec_web.sh
node tests/screen_pin_web_test.mjs heltec
python3 tools/package_heltec_web.py
node tests/heltec_browser_input_test.mjs
