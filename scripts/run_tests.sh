#!/usr/bin/env bash
# TinyArmSim Test Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running build.sh first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

cd "${BUILD_DIR}"

echo "============================================================"
echo " Running TinyArmSim Full Unit & Regression Test Suite "
echo "============================================================"

# Run all unit and regression tests (excluding long-running stress test by default)
ctest --output-on-failure -E Stress

echo ""
echo " All tests passed successfully!"
