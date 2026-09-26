#!/usr/bin/env bash
# TinyCpuSim Microbenchmark (uBench) Runner Script
# Allows developers to run isolated component-level microbenchmarks
# when developing, replacing, or optimizing individual processor modules.

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

print_usage() {
    echo "============================================================"
    echo "      TinyCpuSim Isolated Microbenchmark (uBench) Runner   "
    echo "============================================================"
    echo "Usage: $0 [component] [gtest_options...]"
    echo ""
    echo "Available Components:"
    echo "  all      - Run all isolated microbenchmarks (Default)"
    echo "  bpu      - Branch Prediction & Frontend / PRF (bpu_frontend_ubench_test)"
    echo "  exec     - Execution Pipeline & LSU / LSQ Forwarding (exec_lsu_ubench_test)"
    echo "  rob      - Reorder Buffer & Top-Down Profiling (rob_topdown_ubench_test)"
    echo "  cache    - Cache Hierarchy, MSHR & MESI Coherence (cache_ubench_test)"
    echo ""
    echo "Examples:"
    echo "  $0 bpu                                  # Run all BPU/Frontend microbenchmarks"
    echo "  $0 exec --gtest_filter=*Forwarding*     # Run store-to-load forwarding test"
    echo "  $0 rob --gtest_filter=*Pareto*          # Run Top-Down Pareto ranking test"
    echo "  $0 cache --gtest_filter=*Mesi*          # Run MESI coherence transition test"
    echo "  $0 all                                  # Run all 35 microbenchmark cases"
    echo "============================================================"
}

COMPONENT="${1:-all}"
shift || true

run_target() {
    local target_bin="${BUILD_DIR}/$1"
    local desc="$2"
    if [ ! -f "${target_bin}" ]; then
        echo "Binary ${target_bin} not found. Compiling..."
        cmake --build "${BUILD_DIR}" --target "$1" -j4
    fi
    echo ""
    echo "============================================================"
    echo " >>> Running ${desc} (${target_bin##*/})"
    echo "============================================================"
    "${target_bin}" "$@"
}

case "${COMPONENT}" in
    -h|--help|help)
        print_usage
        exit 0
        ;;
    bpu|frontend)
        run_target "bpu_frontend_ubench_test" "Branch Prediction & Frontend uBench" "$@"
        ;;
    exec|lsu)
        run_target "exec_lsu_ubench_test" "Execution Engine & LSU uBench" "$@"
        ;;
    rob|topdown)
        run_target "rob_topdown_ubench_test" "ROB & Top-Down Profiler uBench" "$@"
        ;;
    cache|coherence)
        run_target "cache_ubench_test" "Cache Hierarchy & MESI Coherence uBench" "$@"
        ;;
    all)
        run_target "bpu_frontend_ubench_test" "Branch Prediction & Frontend uBench" "$@"
        run_target "exec_lsu_ubench_test" "Execution Engine & LSU uBench" "$@"
        run_target "rob_topdown_ubench_test" "ROB & Top-Down Profiler uBench" "$@"
        run_target "cache_ubench_test" "Cache Hierarchy & MESI Coherence uBench" "$@"
        echo ""
        echo "============================================================"
        echo " [SUCCESS] All isolated microbenchmarks completed successfully!"
        echo "============================================================"
        ;;
    *)
        echo "Unknown component: ${COMPONENT}"
        print_usage
        exit 1
        ;;
esac
