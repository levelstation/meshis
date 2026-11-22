#!/bin/sh
# Exercise the existing channel-send adapter against a fake MeshCore transport only.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/heltec
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -funsigned-char -DMESHIS_HELTEC_PRG=1 -I firmware/core -I firmware/ui tests/meshcore_adapter_test.cpp -o build/heltec/meshcore-adapter-test
./build/heltec/meshcore-adapter-test
