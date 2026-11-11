#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
cmake -S . -B build
cmake --build build --parallel
mkdir -p examples/graphs

for package in A X P; do
    ./build/visualizer "examples/config_${package}.toml" \
        > "examples/graphs/${package}.txt"
    cp graph.puml "examples/graphs/${package}.puml"
    cp graph.svg "examples/graphs/${package}.svg"
done

printf 'Готовы файлы примеров в examples/graphs/.\n'
