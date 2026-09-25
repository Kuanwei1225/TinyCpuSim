# Issue 09: CLI Integration & End-to-End Microarchitectural Benchmarks

Status: ready-for-agent
Type: task
Blocked by: 01, 07, 08

## Context & Goal
Integrate the uArch engine into the simulator binary CLI and execute end-to-end performance benchmarks:
1. CLI Flags:
   - `--uarch`: Enable microarchitectural cycle simulation mode.
   - `--uarch-config <file>`: Load custom JSON/KV microarchitectural configuration.
   - `--uarch-stats <file>`: Export detailed simulation statistics (Cycles, IPC, Cache stats, Branch stats).
2. Standard Configuration Presets:
   - `config/in_order_simple.json`: Single-issue in-order baseline (no OoO, direct memory).
   - `config/ooo_medium.json`: 4-issue superscalar, 64-entry ROB, 32KB L1I/L1D, 512KB L2, GShare predictor.
   - `config/ooo_aggressive_multicore.json`: 4-core, 8-issue, 128-entry ROB, 64KB L1, 2MB L2.
3. End-to-End Verification:
   - Run assembly benchmarks (`test_arithmetic.elf`, `test_fibonacci.elf`, `test_sort.elf`, `test_stress.elf`).
   - Validate performance output file generation and non-zero IPC measurements.

## Acceptance Criteria
- Full test pass across all unit, integration, and CLI benchmark tests.
- Accurate generation of structured stats file.
