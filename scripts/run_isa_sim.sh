#!/usr/bin/env bash
# TinyCpuSim Pure ISA Functional Simulator Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${PROJECT_ROOT}/build/tinycpusim"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <elf-file> [options]"
    echo ""
    echo "Examples:"
    echo "  $0 tests/fixtures/test_arithmetic.elf"
    echo "  $0 --log tests/fixtures/test_fibonacci.elf"
    exit 1
fi

"${BIN}" "$@"
