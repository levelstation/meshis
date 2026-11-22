#!/bin/sh
# Compile only. This script never opens a device or invokes an upload target.
set -eu
cd "$(dirname "$0")/.."
variant=${1:-r2}
case "$variant" in r2|r8);; *) echo 'Choose r2 or r8';exit 2;;esac
python3 tools/setup_heltec.py
python3 -m platformio run -d upstream -e meshis_heltec_v43_${variant}_oled
python3 tools/check_heltec_image.py meshis_heltec_v43_${variant}_oled
