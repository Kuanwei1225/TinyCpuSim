#!/usr/bin/env bash
# [Step 4] TinyCpuSim Full-System Simulation Runner
# Runs cycle-accurate Out-of-Order or pure ISA simulation on bare-metal ELF binaries,
# saving performance reports to reports/ directory.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BIN="${BUILD_DIR}/tinycpusim"
REPORTS_DIR="${PROJECT_ROOT}/reports"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

mkdir -p "${REPORTS_DIR}"

if [ $# -eq 0 ]; then
    echo "============================================================"
    echo " [Step 4/5] TinyCpuSim Full Simulation Runner               "
    echo "============================================================"
    echo "Usage: $0 <elf-file> [config-file] [options...]"
    echo ""
    echo "Options:"
    echo "  --all-perf, -v      Print full 40+ hardware performance counters"
    echo "  --isa-only          Run pure functional ISA simulation without uArch timing"
    echo "  --config <file>     Specify custom microarchitecture configuration file"
    echo ""
    echo "Available Built-in Fixtures:"
    for f in "${PROJECT_ROOT}"/tests/fixtures/*.elf; do
        [ -f "$f" ] && echo "  - ${f##*/}"
    done
    echo ""
    echo "Examples:"
    echo "  $0 tests/fixtures/test_fibonacci.elf"
    echo "  $0 tests/fixtures/test_fibonacci.elf --all-perf"
    echo "  $0 tests/fixtures/test_stress.elf configs/ooo_wide.cfg --all-perf"
    echo "  $0 tests/fixtures/test_arithmetic.elf --isa-only"
    echo "============================================================"
    exit 1
fi

ELF_FILE="$1"
shift || true

CFG_FILE="${PROJECT_ROOT}/configs/ooo_medium.cfg"
ALL_PERF=""
ISA_ONLY=0

while [ $# -gt 0 ]; do
    case "$1" in
        --all-perf|-v|--verbose-perf)
            ALL_PERF="--all-perf"
            shift
            ;;
        --isa-only)
            ISA_ONLY=1
            shift
            ;;
        --config|-u)
            CFG_FILE="$2"
            shift 2
            ;;
        *.cfg)
            CFG_FILE="$1"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

ELF_BASENAME=$(basename "${ELF_FILE}" .elf)
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="${REPORTS_DIR}/${ELF_BASENAME}_${TIMESTAMP}.txt"

echo "============================================================"
echo " Running Simulation: ${ELF_FILE}"
if [ ${ISA_ONLY} -eq 0 ]; then
    echo " Mode:     Cycle-Accurate Out-of-Order (OoO) uArch"
    echo " Config:   ${CFG_FILE}"
    echo " Report:   ${REPORT_FILE}"
    echo "============================================================"
    "${BIN}" --uarch --uarch-config "${CFG_FILE}" --perf-log "${REPORT_FILE}" ${ALL_PERF} "${ELF_FILE}"
    echo ""
    echo " Full performance log saved to: ${REPORT_FILE}"
else
    echo " Mode:     Pure Functional ISA Emulation"
    echo "============================================================"
    "${BIN}" "${ELF_FILE}"
fi

echo "============================================================"
