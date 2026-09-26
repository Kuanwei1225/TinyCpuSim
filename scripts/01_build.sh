#!/usr/bin/env bash
# [Step 1] TinyCpuSim Build Script
# Automatically detects system cores, configures CMake Release mode, and builds all binaries.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "============================================================"
echo " [Step 1/5] Building TinyCpuSim (Release Mode)              "
echo "============================================================"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
echo "Configuring CMake (C++17, Release, Parallelism: ${NUM_CORES} cores)..."

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"${NUM_CORES}"

echo ""
echo "============================================================"
echo " [SUCCESS] Build completed!"
echo " Primary simulator binary: ${BUILD_DIR}/tinycpusim"
echo " Next step: Run unit tests using './scripts/02_run_tests.sh'"
echo "============================================================"
