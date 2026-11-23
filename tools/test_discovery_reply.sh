#!/bin/sh
# Exercise the actual shared reply parser with a fake queue; no radio or network.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/discovery
c++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -funsigned-char -I firmware/core -I firmware/ui \
  tests/discovery_reply_test.cpp -o build/discovery/discovery-reply-test
./build/discovery/discovery-reply-test
