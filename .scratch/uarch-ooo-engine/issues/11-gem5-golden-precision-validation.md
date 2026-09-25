# Issue 11: Golden Precision Cross-Validation against gem5 (RTL / Reference Target)

## Problem Statement
Establish a rigorous golden reference cross-validation pipeline comparing TinyArmSim uArch performance metrics (Cycles, IPC, Cache Hit/Miss Rates, Branch Prediction Accuracy, and Memory Contention) against **gem5 (ARM O3CPU / MinorCPU / Classic / Ruby Memory)** running the exact same benchmark ELF binaries under matching hardware parameter configurations.

gem5 acts as our "RTL / Golden Reference Target" for cycle-accurate precision calibration.

## Scope of Work
1. **Benchmark Suite Alignment**:
   - Run identical ELF binaries (CoreMark / Dhrystone / Memory stress / Sorting / Multi-core reduction) on both TinyArmSim and gem5.
2. **Hardware Configuration Equivalence**:
   - Align ROB size (e.g. 64 / 128 entries), RS size (32 entries), L1I/L1D cache geometry (32KB, 4-way, 64B line), L2 cache, and DRAM latency (100 cycles) between TinyArmSim `.cfg` and gem5 Python configuration scripts.
3. **Automated Cross-Validation & Metric Comparison Tool**:
   - Create comparison script (`scripts/compare_with_gem5.py` / shell workflow) parsing TinyArmSim stats report and gem5 `stats.txt`.
   - Calculate precision delta:
     $$\Delta \text{Cycles} = \left|\frac{\text{Cycles}_{\text{TinySim}} - \text{Cycles}_{\text{gem5}}}{\text{Cycles}_{\text{gem5}}}\right|$$
     $$\Delta \text{IPC} = \left|\frac{\text{IPC}_{\text{TinySim}} - \text{IPC}_{\text{gem5}}}{\text{IPC}_{\text{gem5}}}\right|$$
4. **Discrepancy Root-Cause Analysis & Calibration**:
   - Document any precision delta (e.g. pipeline front-end bubble model, branch penalty cycles, memory arbiter delays) and calibrate TinyArmSim pipeline stages accordingly.

## Acceptance Criteria
- [ ] Automated runner executes identical ELF benchmarks on both TinyArmSim and gem5.
- [ ] Precision delta for core metrics (IPC, Cycles, Cache Miss Rate) within agreed tolerance bounds (< 5~10% error margin for identical microarchitectural models).
- [ ] Simulation speed comparison report generated (demonstrating TinyArmSim's MIPS advantage while maintaining golden accuracy).
