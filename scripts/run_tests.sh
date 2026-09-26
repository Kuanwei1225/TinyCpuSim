#!/usr/bin/env bash
# TinyCpuSim Test Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running build.sh first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

cd "${BUILD_DIR}"

echo "============================================================"
echo " Running TinyCpuSim Full Unit & Regression Test Suite       "
echo "============================================================"

ctest --output-on-failure

echo ""
echo " All 156+ unit and microarchitecture tests passed successfully!"
