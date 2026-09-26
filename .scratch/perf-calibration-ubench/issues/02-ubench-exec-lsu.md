# Issue 02: Execution Engine and LSU Isolated Microbenchmarks

Status: resolved

## Objective
Implement at least 5 isolated microbenchmarks for Execution Engine (ALU/MDU/Port latencies/Age-ordered issue) and at least 5 for LSU (STLF, Replay, MOB violation, Cache latencies).

## Seams Under Test
- `IssueQueue`, `stage_execute()`, ALU/Multiplier/Divider port latencies.
- `LoadStoreQueue` (`push_load`, `push_store`, `execute_load`, `execute_store`, `check_violations`).

## UBench Test Cases Implemented (12 tests total)
### Execution (6 tests):
1. `Exec_UBench_RawDependencyChainLatency`: Strict RAW accumulator chain latency propagation through physical registers.
2. `Exec_UBench_MulDivPipelinedLatency`: Multiplier (MUL/MLA) operations and port routing verification.
3. `Exec_UBench_MaxIssueWidthSaturation`: Independent ALU uops saturation up to 4-way issue width.
4. `Exec_UBench_AgeOrderedContentionIssue`: Age-ordered issue priority under queue contention (oldest uops first).
5. `Exec_UBench_OutOfOrderConditionEvaluation`: Out-of-order execution evaluating condition flags produced by CMP.
6. `Exec_UBench_ExecutionPortContention`: Execution port classification and target port routing.

### LSU (6 tests):
1. `LSU_UBench_ExactStoreToLoadForwarding`: Store-to-Load Forwarding (STLF) zero-latency bypassing from SQ to LQ.
2. `LSU_UBench_StoreDataPendingReplay`: Store address valid but data pending -> Load replays until store data arrives.
3. `LSU_UBench_MemoryOrderViolationDetection`: Speculative younger load before aliasing store -> MOB violation detection and ROB index targeting.
4. `LSU_UBench_L1CacheHitVsMissLatency`: L1 Data Cache hit latency vs DRAM bus access.
5. `LSU_UBench_StridedAccessCacheThrashing`: Cacheline-strided loads across multiple sets.
6. `LSU_UBench_LoadStoreQueueWrapAround`: Streaming allocation/commit/free without queue slot leakage.

## Verification
- `exec_lsu_ubench_test`: 12 / 12 passed.
- `ctest`: 139 / 139 passed (100%).
