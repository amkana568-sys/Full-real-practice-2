#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/visualizer "$@"
