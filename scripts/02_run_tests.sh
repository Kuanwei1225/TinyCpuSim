#!/usr/bin/env bash
# [Step 2] TinyCpuSim Regression Test Runner
# Runs the full suite of 156+ unit and microarchitectural tests.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

cd "${BUILD_DIR}"

echo "============================================================"
echo " [Step 2/5] Running Full Regression Suite (156+ Tests)      "
echo "============================================================"

ctest --output-on-failure

echo ""
echo "============================================================"
echo " [SUCCESS] 100% tests passed successfully!"
echo " Next step: Run microbenchmarks with './scripts/03_run_ubench.sh'"
echo "============================================================"
