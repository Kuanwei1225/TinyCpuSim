# Issue 08: Multi-Core Coordinator & Shared LLC Interconnect

Status: ready-for-agent
Type: task
Blocked by: 02, 07

## Context & Goal
Orchestrate multiple OoO cores executing concurrently in cycle lock-step:
1. `MultiCoreCoordinator`:
   - Instantiates $N$ independent `OoOCore` instances (Core 0..N-1).
   - Advances global simulation clock cycle by cycle (`step_cycle()`).
   - Collects per-core progress, IPC, and detects collective termination.
2. Interconnect & Shared L2/LLC:
   - Cores share a common L2 cache and physical memory bus with round-robin or priority bus arbitration.
   - Per-core private L1I / L1D caches connect to shared L2.

## Acceptance Criteria
- Unit tests verifying:
  - Multi-core instantiation (1, 2, 4, 8 cores).
  - Shared L2 cache contention and bus arbitration.
  - Multi-core aggregated statistics calculation.
