#!/usr/bin/env bash
# [Step 3] TinyCpuSim Isolated Microbenchmark (uBench) Runner & Performance Suite
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

# Pass directly to Python uBench Performance & Diagnostics Engine
python3 "${PROJECT_ROOT}/scripts/run_ubench.py" "$@"
