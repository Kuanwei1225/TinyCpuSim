#!/usr/bin/env bash
# [Step 4] TinyCpuSim Full-System Simulation Runner
# Runs cycle-accurate Out-of-Order or pure ISA simulation on bare-metal ELF binaries.
# By default, reads configs/current.cfg automatically so no CLI arguments are needed.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BIN="${BUILD_DIR}/tinycpusim"
REPORTS_DIR="${PROJECT_ROOT}/reports"
CURRENT_CFG="${PROJECT_ROOT}/configs/current.cfg"

if [ ! -f "${BIN}" ]; then
    echo "Simulator binary not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

mkdir -p "${REPORTS_DIR}"

# If current.cfg does not exist, initialize from default
if [ ! -f "${CURRENT_CFG}" ]; then
    python3 "${PROJECT_ROOT}/scripts/config.py" reset
fi

# Extract settings from configs/current.cfg using python
CONFIG_SETTINGS=$(python3 -c "
import configparser
cfg = configparser.ConfigParser()
cfg.read('${CURRENT_CFG}')
elf = cfg.get('simulation', 'elf_path', fallback='tests/fixtures/test_fibonacci.elf')
mode = cfg.get('simulation', 'mode', fallback='uarch')
all_perf = cfg.get('simulation', 'all_perf', fallback='true')
topdown = cfg.get('simulation', 'enable_topdown', fallback='true')
print(f'{elf}|{mode}|{all_perf}|{topdown}')
")

IFS='|' read -r DEF_ELF DEF_MODE DEF_ALL_PERF DEF_TOPDOWN <<< "${CONFIG_SETTINGS}"

ELF_FILE="${DEF_ELF}"
CFG_FILE="${CURRENT_CFG}"
ALL_PERF=""
[ "${DEF_ALL_PERF}" = "true" ] && ALL_PERF="--all-perf"
ISA_ONLY=0
[ "${DEF_MODE}" = "isa_only" ] && ISA_ONLY=1
ENABLE_TOPDOWN=""
[ "${DEF_TOPDOWN}" = "true" ] && ENABLE_TOPDOWN="--topdown"

# Handle CLI overrides if provided
if [ $# -gt 0 ]; then
    if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
        echo "============================================================"
        echo " [Step 4/5] TinyCpuSim Full Simulation Runner               "
        echo "============================================================"
        echo "Usage: $0 [elf-file] [config-file] [options...]"
        echo ""
        echo "Zero-Argument Mode:"
        echo "  $0                  # Automatically runs using configs/current.cfg"
        echo ""
        echo "Options:"
        echo "  --all-perf, -v      Print full 40+ hardware performance counters"
        echo "  --isa-only          Run pure functional ISA simulation without uArch timing"
        echo "  --config <file>     Specify custom microarchitecture configuration file"
        echo "============================================================"
        exit 0
    fi

    if [[ "$1" != -* ]]; then
        ELF_FILE="$1"
        shift
    fi

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
fi

# Resolve ELF path
if [[ "${ELF_FILE}" != /* ]]; then
    ELF_FILE="${PROJECT_ROOT}/${ELF_FILE}"
fi

if [ ! -f "${ELF_FILE}" ]; then
    echo "Error: Target ELF binary not found: ${ELF_FILE}"
    exit 1
fi

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
    "${BIN}" --uarch --uarch-config "${CFG_FILE}" --perf-log "${REPORT_FILE}" ${ALL_PERF} ${ENABLE_TOPDOWN} "${ELF_FILE}"
    echo ""
    echo " Full performance log saved to: ${REPORT_FILE}"
else
    echo " Mode:     Pure Functional ISA Emulation"
    echo "============================================================"
    "${BIN}" "${ELF_FILE}"
fi

echo "============================================================"
