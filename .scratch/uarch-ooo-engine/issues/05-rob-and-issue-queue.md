# Issue 05: Reorder Buffer (ROB) & Reservation Stations / Issue Queue

Status: ready-for-agent
Type: task
Blocked by: 04

## Context & Goal
Implement out-of-order dispatch, execution wake-up, and in-order retirement:
1. `ReorderBuffer (ROB)`:
   - Parametric capacity ($N$ entries).
   - Circular FIFO tracking instruction lifecycle: allocated -> issued -> completed -> committed.
   - Preserves in-order architectural retirement and precise exceptions.
   - Mispredict / exception flush squashing all younger instructions.
2. `IssueQueue / ReservationStations`:
   - Holds dispatched micro-ops waiting for source operands.
   - Dynamic tag matching (wake-up) on completion broadcast from execution units.
   - Out-of-order select (oldest ready first) limited by `issue_width`.

## Acceptance Criteria
- Unit tests verifying:
  - Out-of-order completion: younger fast instruction completes before older multi-cycle instruction, but commits only in strict program order.
  - ROB full stall when head cannot advance.
  - Flush operation squashes all entries younger than mispredicted branch.
