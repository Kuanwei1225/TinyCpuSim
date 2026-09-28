#!/bin/bash
# [Step 3] TinyCpuSim Isolated Microbenchmark (uBench) Runner & Performance Suite
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# Resolve default installation path for Python, avoiding env/PATH where possible
find_default_python() {
    for candidate in \
        "/usr/bin/python3" \
        "/usr/local/bin/python3" \
        "/opt/homebrew/bin/python3" \
        "/usr/bin/python"; do
        if [ -x "${candidate}" ]; then
            echo "${candidate}"
            return 0
        fi
    done
    if command -v python3 >/dev/null 2>&1; then
        command -v python3
        return 0
    fi
    echo "python3"
}

PYTHON_BIN=$(find_default_python)

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Running Step 1 first..."
    "${PROJECT_ROOT}/scripts/01_build.sh"
fi

# Pass directly to Python uBench Performance & Diagnostics Engine
"${PYTHON_BIN}" "${PROJECT_ROOT}/scripts/run_ubench.py" "$@"
