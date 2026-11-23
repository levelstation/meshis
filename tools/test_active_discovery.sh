#!/bin/sh
# Compile and exercise only a fake transport; no hardware or network access.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/discovery
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -funsigned-char -I firmware/core tests/active_discovery_test.cpp \
  -o build/discovery/active-discovery-test
./build/discovery/active-discovery-test
