# Issue 02: Execution Engine and LSU Isolated Microbenchmarks

Status: ready-for-agent

## Objective
Implement at least 5 isolated microbenchmarks for Execution Engine (ALU/MDU/Port latencies/Age-ordered issue) and at least 5 for LSU (STLF, Replay, MOB violation, Cache latencies).

## Seams Under Test
- `IssueQueue`, `stage_execute()`, ALU/Multiplier/Divider port latencies.
- `LoadStoreQueue` (`push_load`, `push_store`, `execute_load`, `execute_store`, `check_violations`).

## UBench Test Cases
### Execution:
1. `Exec_UBench_RawDependencyChainLatency`
2. `Exec_UBench_MulDivPipelinedLatency`
3. `Exec_UBench_MaxIssueWidthSaturation`
4. `Exec_UBench_AgeOrderedContentionIssue`
5. `Exec_UBench_OutOfOrderConditionEvaluation`
6. `Exec_UBench_ExecutionPortContention`

### LSU:
1. `LSU_UBench_ExactStoreToLoadForwarding`
2. `LSU_UBench_StoreDataPendingReplay`
3. `LSU_UBench_MemoryOrderViolationDetection`
4. `LSU_UBench_L1CacheHitVsMissLatency`
5. `LSU_UBench_StridedAccessCacheThrashing`
6. `LSU_UBench_MisalignedMemoryAccess`
