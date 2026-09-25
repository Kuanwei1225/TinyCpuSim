# Issue 07: Integrated Out-of-Order Single-Core Pipeline & Mispredict Recovery

Status: ready-for-agent
Type: task
Blocked by: 02, 03, 04, 05, 06

## Context & Goal
Integrate all single-core components into a cohesive cycle-by-cycle Out-of-Order CPU Core (`OoOCore`):
1. Pipeline Stages:
   - `Fetch`: Retrieves instructions via L1I Cache using branch predictor.
   - `Decode`: Unpacks ARM/Thumb instructions into micro-ops.
   - `Rename/Allocate`: Claims physical registers and entries in ROB, RS, LQ/SQ.
   - `Issue/Dispatch`: Out-of-order dispatch to execution units when operands ready.
   - `Execute`: Multi-port ALU, Multiplier, Branch, and LSU units with realistic cycle latencies.
   - `Writeback`: Broadcasts results, wakes up dependent instructions in RS/ROB.
   - `Commit/Retire`: In-order retirement, frees old physical registers, drains committed stores to L1D.
2. Mispredict Recovery Protocol:
   - Restores RAT from checkpoint, clears speculative ROB/RS/LQ/SQ entries, redirects PC fetch.
3. Functional Verification:
   - Validates committed architectural register state matches ISA Interpreter results.

## Acceptance Criteria
- Unit tests verifying:
  - Full execution of arithmetic, branch loops, Fibonacci, memory sort on OoO core.
  - Correct recovery and valid result output on intentional branch mispredictions.
