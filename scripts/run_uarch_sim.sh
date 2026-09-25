#!/usr/bin/env bash
# TinyArmSim Microarchitecture Simulator Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${PROJECT_ROOT}/build/tinyarmsim"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <elf-file> [config-file] [stats-output-file]"
    echo "Example: $0 test.elf configs/ooo_medium.cfg uarch_stats.txt"
    exit 1
fi

ELF_FILE="$1"
CFG_FILE="${2:-${PROJECT_ROOT}/configs/ooo_medium.cfg}"
STATS_FILE="${3:-${PROJECT_ROOT}/uarch_stats.txt}"

echo "Running uArch simulation with config: ${CFG_FILE}"
"${BIN}" --uarch --uarch-config "${CFG_FILE}" --uarch-stats "${STATS_FILE}" "${ELF_FILE}"
