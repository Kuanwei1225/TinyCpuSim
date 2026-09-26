# Issue 03: ROB, Retirement and Top-Down Profiling Isolated Microbenchmarks

Status: ready-for-agent

## Objective
Implement at least 5 isolated microbenchmarks for ROB & Retirement and at least 5 for Top-Down Profiler / ROI.

## Seams Under Test
- `ReorderBuffer`, `TopDownProfiler`

## UBench Test Cases
### ROB:
1. `ROB_UBench_SustainedRetireThroughput`
2. `ROB_UBench_HeadOfRobBlockingRetire`
3. `ROB_UBench_CircularBufferWrapAroundStress`
4. `ROB_UBench_SpeculativeStoreDrainOnRetire`
5. `ROB_UBench_MultipleBranchMispredictFlushes`

### Top-Down & ROI:
1. `TopDown_UBench_SlotConservationInvariant`
2. `TopDown_UBench_FrontendVsBackendBreakdown`
3. `TopDown_UBench_BadSpeculationAccounting`
4. `ROB_UBench_RoiBoundaryResetStats`
5. `TopDown_UBench_BottleneckParetoRanking`
