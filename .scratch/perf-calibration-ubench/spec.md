# Spec: Microarchitectural Isolated Microbenchmarks & Precision Calibration

## Overview
This effort builds comprehensive isolated microbenchmarks (at least 5 per component) across 5 core microarchitectural components:
1. Branch Prediction Unit (BPU)
2. Frontend (Fetch, Decode, Rename, PRF/FreeList)
3. Execution Engine & Issue Queue (IQ, ALU/MDU, Port Latencies)
4. Load/Store Unit & Memory Disambiguation (LSU, MOB, STLF, Cache)
5. Reorder Buffer, Retirement & Top-Down Profiling (ROB, TopDownProfiler, ROI)

Following isolated test creation, latency calibration against gem5 and CLI/Perf log exporting are implemented.

## Seams Under Test
- **BPU**: `BranchPredictor` (`predict`, `update`, BTB, RAS, TAGE/GShare)
- **Frontend**: `FetchUnit`, `UOpDecoder`, `RAT`, `PRF`, `FreeList`
- **Exec**: `IssueQueue`, `stage_execute`, ALU/MDU latencies
- **LSU**: `LoadStoreQueue` (`push_load`, `push_store`, `execute_load`, `execute_store`, `check_violations`)
- **ROB/TopDown**: `ReorderBuffer`, `TopDownProfiler` (Retire rate, Slot conservation, ROI m5ops)

## Plan of Work
- Issue 01: BPU & Frontend Isolated Microbenchmarks
- Issue 02: Execution & LSU Isolated Microbenchmarks
- Issue 03: ROB & Top-Down Profiler Isolated Microbenchmarks
- Issue 04: Precision Calibration against gem5 (Latencies & Branch Penalty)
- Issue 05: Perf Log, CLI Integration & Multi-Format Report Export
