#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/heltec
c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-parameter -fsanitize=address,undefined -funsigned-char -DARDUINO=10800 -DPIN_OLED_RESET=21 -I tests/heltec_shims -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I firmware/display tests/ssd1315_adapter_test.cpp firmware/display/SSD1315FramebufferDisplay.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp -o build/heltec/adapter-test
./build/heltec/adapter-test
