# Issue 10: Multi-Core Execution, Workload Partitioning & Parallel Benchmark Verification

## Problem Statement
Support real multi-core software execution models (Single-ELF SMP with MPIDR/independent stack and Multi-ELF AMP) on top of the coherent memory interconnect, enabling parallel applications with spinlocks (`LDREX`/`STREX`), barriers, and parallel reduction to run and cross-validate answers.

## Scope of Work
1. **CPU Identification (`MPIDR` / System Registers)**:
   - Provide Core ID identification via CP15 / System register (`MPIDR`) for each core.
2. **Multi-Core Stack & Entry Setup**:
   - Single-ELF SMP: Automatically assign distinct Stack Pointer addresses (`SP_core = STACK_TOP - core_id * STACK_SIZE`) for concurrent cores.
   - Multi-ELF AMP: Allow CLI loading of multiple ELF binaries mapped to designated cores (`--core 0 app0.elf --core 1 app1.elf`).
3. **Multi-Core Parallel Test Bench**:
   - Create multi-core assembly parallel workload (e.g. 4-core parallel vector addition / parallel reduction with spinlock synchronization).
   - Cross-check computed results against expected values.

## Acceptance Criteria
- [ ] Core `MPIDR` reflects `core_id` correctly.
- [ ] Parallel workloads execute deterministically without race conditions across 4 cores.
- [ ] Atomic `LDREX`/`STREX` operations guarantee mutual exclusion under high contention.
- [ ] 100% test pass on CTest suite.
