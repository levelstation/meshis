#!/bin/sh
# Local PIN state-machine and portable digest tests, no device or network access.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/screen-pin
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -funsigned-char -I firmware/core -I firmware/ui \
  tests/test_screen_pin.cpp -o build/screen-pin/test-screen-pin
./build/screen-pin/test-screen-pin
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -funsigned-char -DMESHIS_HELTEC_PRG=1 \
  -I firmware/core -I firmware/ui tests/test_screen_pin.cpp \
  -o build/screen-pin/test-screen-pin-prg
./build/screen-pin/test-screen-pin-prg
