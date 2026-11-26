#!/bin/sh
# Host-only real-font framebuffer/animation tests. No devices or network.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/text-marquee
for profile in wio heltec; do
 flags='-DMESHIS_V2=1'
 if [ "$profile" = heltec ]; then flags="$flags -DMESHIS_HELTEC_PRG=1"; fi
 c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-unused-parameter \
  -funsigned-char -DARDUINO=10800 -fsanitize=address,undefined $flags \
  -I firmware/core -I firmware/display -I simulator/shims -I simulator/vendor/adafruit-gfx -I vendor/meshcore/src \
  tests/text_marquee_test.cpp simulator/vendor/adafruit-gfx/Adafruit_GFX.cpp \
  -o build/text-marquee/test-$profile
 ./build/text-marquee/test-$profile
done
