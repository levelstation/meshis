#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
c++ -std=c++17 -DARDUINO=10800 -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src -I firmware/core -I firmware/ui -I firmware/display tools/render_gallery.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp -o build/render-gallery
./build/render-gallery
