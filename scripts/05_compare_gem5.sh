#!/usr/bin/env bash
# [Step 5] TinyCpuSim vs gem5 Golden Reference Accuracy Comparator
# Runs regression accuracy comparison against gem5 cycle-accurate golden models.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "============================================================"
echo " [Step 5/5] TinyCpuSim vs gem5 Golden Reference Accuracy   "
echo "============================================================"

python3 "${PROJECT_ROOT}/scripts/verify_gem5.py" "$@"
