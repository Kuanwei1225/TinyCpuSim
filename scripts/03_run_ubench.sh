#!/usr/bin/env bash
# [Step 3] TinyCpuSim Isolated Microbenchmark (uBench) Runner
# Runs fine-grained component microbenchmarks with optional parameter sweep support.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

print_usage() {
    echo "============================================================"
    echo " [Step 3/5] TinyCpuSim Component Microbenchmarks (uBench)  "
    echo "============================================================"
    echo "Usage: $0 [component] [options...]"
    echo ""
    echo "Available Components:"
    echo "  all       - Run all 35 isolated microbenchmarks (Default)"
    echo "  bpu       - Branch Prediction & Frontend / PRF (12 tests)"
    echo "  exec      - Execution Pipeline & LSU / Forwarding (12 tests)"
    echo "  rob       - Reorder Buffer & Top-Down TMAM Profiler (11 tests)"
    echo "  cache     - Cache Hierarchy, MSHR & MESI Coherence (6 tests)"
    echo "  --sweep   - Run automated parameter sweep exploration"
    echo ""
    echo "Examples:"
    echo "  $0 all                                  # Run all microbenchmarks"
    echo "  $0 bpu                                  # Run BPU/Frontend tests"
    echo "  $0 exec --gtest_filter=*Forwarding*     # Test Store-to-Load Forwarding"
    echo "  $0 --sweep --param=rob_size             # Sweep ROB size"
    echo "  $0 --sweep --param=issue_width          # Sweep Issue Width"
    echo "============================================================"
}

COMPONENT="${1:-all}"

if [ "${COMPONENT}" = "--sweep" ] || [ "${COMPONENT}" = "-s" ] || [ "${COMPONENT}" = "sweep" ]; then
    shift || true
    python3 "${PROJECT_ROOT}/scripts/sweep_parameters.py" "$@"
    exit 0
fi

shift || true

run_target() {
    local target_bin="${BUILD_DIR}/$1"
    local desc="$2"
    if [ ! -f "${target_bin}" ]; then
        echo "Compiling ${1}..."
        cmake --build "${BUILD_DIR}" --target "$1" -j4
    fi
    echo ""
    echo "============================================================"
    echo " >>> Running ${desc}"
    echo "============================================================"
    "${target_bin}" "$@"
}

case "${COMPONENT}" in
    -h|--help|help)
        print_usage
        exit 0
        ;;
    bpu|frontend)
        run_target "bpu_frontend_ubench_test" "Branch Prediction & Frontend uBench (bpu_frontend_ubench_test)" "$@"
        ;;
    exec|lsu)
        run_target "exec_lsu_ubench_test" "Execution Engine & LSU uBench (exec_lsu_ubench_test)" "$@"
        ;;
    rob|topdown)
        run_target "rob_topdown_ubench_test" "ROB & Top-Down Profiler uBench (rob_topdown_ubench_test)" "$@"
        ;;
    cache|coherence)
        run_target "cache_ubench_test" "Cache Hierarchy & MESI Coherence uBench (cache_ubench_test)" "$@"
        ;;
    all)
        run_target "bpu_frontend_ubench_test" "Branch Prediction & Frontend uBench" "$@"
        run_target "exec_lsu_ubench_test" "Execution Engine & LSU uBench" "$@"
        run_target "rob_topdown_ubench_test" "ROB & Top-Down Profiler uBench" "$@"
        run_target "cache_ubench_test" "Cache Hierarchy & MESI Coherence uBench" "$@"
        echo ""
        echo "============================================================"
        echo " [SUCCESS] All microbenchmarks executed!"
        echo " Next step: Run full simulation with './scripts/04_run_simulation.sh'"
        echo "============================================================"
        ;;
    *)
        echo "Unknown component: ${COMPONENT}"
        print_usage
        exit 1
        ;;
esac
