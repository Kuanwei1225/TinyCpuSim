# Issue 02: Parameterized Cache Subsystem (L1I, L1D, Shared L2)

Status: ready-for-agent
Type: task
Blocked by: 01

## Context & Goal
Implement a generic, highly parameterized cache model supporting L1 Instruction, L1 Data, and multi-core Shared L2/LLC caches:
1. `CacheConfig`: size in bytes, cache line size (e.g. 64B), associativity (1-way direct, 2, 4, 8, 16-way), hit latency in cycles, replacement policy (LRU, FIFO, Random), write policy (Write-Back with dirty bit, Write-Through).
2. `Cache`:
   - Bitwise address breakdown: `[Tag | Set Index | Block Offset]`.
   - Set-associative lookup, hit/miss detection, line allocation, dirty line writeback events.
   - Bypass mode: When disabled, returns instant 0-cycle hit without cache overhead.
3. Edge cases: Non-aligned addresses, full set replacement, dirty eviction cascade, capacity miss vs conflict miss counters.

## Acceptance Criteria
- Unit tests verifying:
  - Direct mapped, 2-way, 4-way, fully associative lookups.
  - LRU age updating and exact eviction sequence under pressure.
  - Dirty line tracking and write-back requests on eviction.
  - Multi-line access and boundary conditions.
