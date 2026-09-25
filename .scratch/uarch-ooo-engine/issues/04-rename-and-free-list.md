# Issue 04: Register Alias Table (RAT), Physical Register File & Free List

Status: ready-for-agent
Type: task
Blocked by: 01

## Context & Goal
Implement speculative register renaming to eliminate false dependencies (WAR / WAW hazards):
1. `PhysicalRegisterFile (PRF)`: Configurable size (e.g. 64..256 registers), tracking readiness status bit and values.
2. `FreeList`: Manages unallocated physical registers with fast allocation/reclaim.
3. `RegisterAliasTable (RAT)`:
   - Speculative Frontend RAT: Maps R0-R15 -> Physical Register (P_i).
   - Architectural/Commit RAT: Updated at instruction retirement.
   - Checkpoint & Rollback: Checkpointed RAT state on branch dispatch for single-cycle rollback on misprediction squashes.

## Acceptance Criteria
- Unit tests verifying:
  - Free register allocation until exhaustion triggers stall.
  - WAW renaming (two writes to same architectural register get distinct physical registers).
  - Checkpoint creation, restore on branch mispredict, and free list recovery.
  - Physical register reclamation only when superseding instruction commits.
