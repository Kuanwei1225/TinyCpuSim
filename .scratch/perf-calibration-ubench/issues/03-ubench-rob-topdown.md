# Issue 03: ROB, Retirement and Top-Down Profiling Isolated Microbenchmarks

Status: resolved

## Objective
Implement at least 5 isolated microbenchmarks for ROB & Retirement and at least 5 for Top-Down Profiler / ROI.

## Seams Under Test
- `ReorderBuffer`, `TopDownProfiler`

## UBench Test Cases Implemented (11 tests total)
### ROB (6 tests):
1. `ROB_UBench_SustainedRetireThroughput`: Sustained retirement throughput at 4-way commit.
2. `ROB_UBench_HeadOfRobBlockingRetire`: Head-of-ROB unready blocking younger ready uops from committing.
3. `ROB_UBench_CircularBufferWrapAroundStress`: Continuous 256 uop allocate/complete/commit wrap-around.
4. `ROB_UBench_SpeculativeStoreDrainOnRetire`: Store commits to memory only when reaching ROB head.
5. `ROB_UBench_MultipleBranchMispredictFlushes`: Tail rollback and younger uop purging on branch mispredict.
6. `ROB_UBench_IsYoungerCircularAgeDistance`: Relative distance and age evaluation across buffer boundaries.

### Top-Down & ROI (5 tests):
1. `TopDown_UBench_SlotConservationInvariant`: Slot conservation invariant holds across 100 cycles of mixed events (`Total == Retiring + BadSpec + FE + BE`).
2. `TopDown_UBench_FrontendVsBackendBreakdown`: Precise classification of I-Cache vs D-Cache stalls.
3. `TopDown_UBench_BadSpeculationAccounting`: Flushed slot accounting on branch misprediction.
4. `ROB_UBench_RoiBoundaryResetStats`: Dynamic ROI isolation and stat resetting via `m5ops` (0x50/0x51/0x52).
5. `TopDown_UBench_BottleneckParetoRanking`: Pareto ranking of dominant architectural bottlenecks.

## Verification
- `rob_topdown_ubench_test`: 11 / 11 passed.
- `ctest`: 150 / 150 passed (100%).
