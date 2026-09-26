#!/usr/bin/env bash
# TinyCpuSim Microarchitecture Out-of-Order Simulator Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${PROJECT_ROOT}/build/tinycpusim"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <elf-file> [config-file] [perf-log-file]"
    echo ""
    echo "Examples:"
    echo "  $0 tests/fixtures/test_fibonacci.elf"
    echo "  $0 tests/fixtures/test_stress.elf configs/ooo_medium.cfg /tmp/stress_perf.log"
    exit 1
fi

ELF_FILE="$1"
CFG_FILE="${2:-${PROJECT_ROOT}/configs/ooo_medium.cfg}"
PERF_LOG="${3:-${PROJECT_ROOT}/uarch_stats.txt}"

echo "============================================================"
echo " Running Out-of-Order uArch Simulation: ${ELF_FILE}"
echo " Config: ${CFG_FILE}"
echo " Perf Log: ${PERF_LOG}"
echo "============================================================"

"${BIN}" --uarch --uarch-config "${CFG_FILE}" --perf-log "${PERF_LOG}" "${ELF_FILE}"
