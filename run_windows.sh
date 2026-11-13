#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# Russian messages are stored in UTF-8.
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
CONFIG_FILE="${1:-config.toml}"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/visualizer.exe "$CONFIG_FILE"
