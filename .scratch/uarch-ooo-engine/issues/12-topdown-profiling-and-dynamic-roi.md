# Issue 12: Unified Top-Down Profiling Framework & gem5-Compatible Dynamic ROI Tracking

Status: resolved

## Problem Statement
Implement a unified, low-overhead microarchitecture performance counter and Top-Down profiling subsystem (`PerfCounterManager`) for TinyArmSim's `Cycle Engine`. Support gem5-compatible `m5ops` (`m5_reset_stats`, `m5_dump_stats`) and instruction/PC triggers for dynamic Region-of-Interest (ROI) algorithm profiling, with multi-format export (Text, JSON, CSV) and 100% deterministic unit test validation.

## Scope of Work
1. **Unified Profiler Engine (`PerfCounterManager`)**:
   - Level-1 Top-Down Slots Breakdown (Sum = 100% of pipeline capacity):
     - `Retiring` (Base ALU, Memory Ops)
     - `Bad Speculation` (Mispredict Flushes, Squashed speculative $\mu$ops)
     - `Front-End Bound` (L1I Cache Miss, BTB redirect bubble)
     - `Back-End Bound` (Core Resource Stalls: RS/ROB/FreeList Full; Memory Stalls: L1D Miss, MSHR Full, Store Buffer Full)
   - LSU & Coherence Specific Events (Store Forwarding Hits, MOB Memory Order Violation Squashes, MESI Invalidation Stalls).
   - Top-3 Microarchitectural Bottleneck Ranking / Pareto Analysis.

2. **gem5-Compatible Dynamic ROI (m5ops & Triggers)**:
   - Support `m5_reset_stats` and `m5_dump_stats` hypercalls via opcode / SVC mappings (`SVC #0x50`, `SVC #0x51`, `SVC #0x52`).
   - Support CLI parameter `--topdown [file]` and `--topdown-format=text,json,csv`.

3. **Deterministic Multi-Stage Unit & Self-Test Suite**:
   - Phase A: Profiler Framework & Exporters (JSON/CSV serialization, Slot conservation invariant).
   - Phase B: Dynamic ROI & m5ops state machine verification.
   - Phase C: Front-End & Branch Mispredict slot attribution.
   - Phase D: Back-End Core Resource exhaustion stall counters (ROB/RS full).
   - Phase E: Memory Hierarchy & LSU counters (L1D Miss, MSHR Full, Store-Load Forwarding, MOB Disambiguation Squash).
   - Phase F: CLI Integration & golden regression verification.

## Acceptance Criteria
- [x] `PerfCounterManager` / `TopDownProfiler` integrated with zero overhead when profiling is disabled.
- [x] Pipeline slots rigorously satisfy $\text{Retiring} + \text{BadSpec} + \text{FrontEnd} + \text{BackEnd} = \text{Total Slots}$ (100% conservation invariant verified in `topdown_profiler_test`).
- [x] `m5_reset_stats()` and `m5_dump_stats()` correctly isolate algorithm execution from startup/teardown in both GoogleTest and Assembly Self-Tests (`roi_m5ops_test`).
- [x] Multi-format reports (Text Tree, JSON, CSV) accurately generated and verified.
- [x] 110 / 110 tests passing with 0 compiler warnings.

## Answer
- Implemented deep module `TopDownProfiler` in `include/tinyarmsim/uarch/topdown_profiler.hpp`.
- Implemented gem5-compatible `m5ops` (`m5_reset_stats` = `SVC #0x50`, `m5_dump_stats` = `SVC #0x51`, `m5_exit` = `SVC #0x52`) in `IsaInterpreter` and `TopDownProfiler`.
- Added CLI arguments `--topdown [file]`, `--topdown-format [text|json|csv]`.
- All 110 tests passing with 0 warnings.

