#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
CONFIG_FILE="${1:-config.toml}"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/visualizer.exe "$CONFIG_FILE"
