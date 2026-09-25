#!/usr/bin/env bash
# TinyArmSim Pure ISA Simulator Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${PROJECT_ROOT}/build/tinyarmsim"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <elf-file> [extra-args...]"
    echo "Example: $0 test.elf --log --coverage cov.csv"
    exit 1
fi

"${BIN}" "$@"
