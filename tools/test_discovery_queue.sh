#!/bin/sh
# Test the production ownership guard using a fake packet manager; no device.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/discovery
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -funsigned-char -I firmware/core \
  tests/discovery_queue_test.cpp -o build/discovery/discovery-queue-test
./build/discovery/discovery-queue-test
