# 02: Architectural State, 64MB Flat Memory Bus, and Fault Hierarchy

**What to build:** The foundational state abstractions for TinyArmSim: `ArchitecturalState` (32-bit registers R0-R15 + CPSR/APSR flags NZCV), a flat 64MB `MemoryBus` with 8/16/32-bit read/write accessors, and typed `CpuFault` exceptions (`MemoryFaultException`, `UndefinedInstructionException`).

**Blocked by:** 01-project-scaffold

**Status:** resolved

- [x] `ArchitecturalState` implements register read/write (R0-R15), PC advancement helper, and condition flag helpers (N, Z, C, V)
- [x] `MemoryBus` allocates flat 64MB address space and provides `read8`, `read16`, `read32`, `write8`, `write16`, `write32`
- [x] Out-of-bound or unaligned accesses throw typed `MemoryFaultException`
- [x] Unit tests verify register operations, flag manipulations, memory operations, and fault handling
