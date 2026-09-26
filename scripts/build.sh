#!/usr/bin/env bash
# TinyCpuSim Quick Build Script
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "============================================================"
echo " Building TinyCpuSim (Simulator + uArch + All Unit Tests)  "
echo "============================================================"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo ""
echo " Build completed successfully!"
echo " Binaries located in ${BUILD_DIR}/tinycpusim"
echo " Example command: ./build/tinycpusim --uarch tests/fixtures/test_fibonacci.elf"
