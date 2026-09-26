# Issue 06: Cache and Memory Hierarchy Isolated Microbenchmarks

Status: resolved

## Objective
Implement at least 5 isolated microbenchmarks for Cache, MESI Coherence, and Memory Hierarchy covering critical cache and coherence scenarios.

## Seams Under Test
- `Cache` (`access`, LRU/FIFO/Random, Writeback/Writethrough, MSHR)
- `MESICoherenceEngine` (Shared/Exclusive/Modified transitions, peer snoop invalidation, upgrade)
- `CoherentMemoryHierarchy` (L1 Hit -> L2 Hit -> DRAM Miss latency hierarchy)

## UBench Test Cases (6 tests):
1. `Cache_UBench_L1HitLatencyAndThroughput`: Validates L1 hit latency, hit rate statistics, and line tag extraction.
2. `Cache_UBench_LruReplacementSetAssociativity`: Validates N-way set associativity and precise LRU line eviction.
3. `Cache_UBench_WriteBackDirtyEviction`: Validates dirty bit tracking on store and dirty line writeback on replacement.
4. `Cache_UBench_MshrNonBlockingAllocation`: Validates MSHR allocation on cache misses and non-blocking capacity limits.
5. `Cache_UBench_MesiCoherenceStateTransitions`: Validates full MESI state machine transitions across multi-core snooping.
6. `Cache_UBench_SharedL2HierarchicalInclusion`: Validates L1 miss falling through to Shared L2 hit and latency aggregation.

## Verification
- `cache_ubench_test`: 6 / 6 passed.
- `ctest`: 156 / 156 passed (100%).
