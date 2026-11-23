#!/bin/sh
# Compile the real adapter against fake native MeshCore APIs; no device access.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/discovery
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -funsigned-char -I firmware/core -I firmware/ui \
  tests/discovery_adapter_test.cpp -o build/discovery/discovery-adapter-test
./build/discovery/discovery-adapter-test
