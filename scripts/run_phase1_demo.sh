#!/usr/bin/env bash
# TinyArmSim Phase 1 Cache Speedup & MESI Coherence Demo Runner
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO_BIN="${PROJECT_ROOT}/build/cache_speedup_demo_test"

if [ ! -f "${DEMO_BIN}" ]; then
    echo "Demo binary not found. Building first..."
    "${PROJECT_ROOT}/scripts/build.sh"
fi

echo "============================================================"
echo " Running TinyArmSim Phase 1 Cache & MESI Speedup Demo "
echo "============================================================"

"${DEMO_BIN}" --gtest_filter="*"
