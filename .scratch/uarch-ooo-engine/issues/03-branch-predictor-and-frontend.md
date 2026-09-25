# Issue 03: Parameterized Branch Predictor & Front-End Fetch/Decode

Status: ready-for-agent
Type: task
Blocked by: 01

## Context & Goal
Implement the Front-End pipeline stage with swappable branch prediction engines:
1. `IBranchPredictor` interface:
   - `predict(pc) -> bool`
   - `update(pc, taken, actual_target)`
2. Implementations:
   - `IdealPredictor`: Always 100% accurate (bypassed branch penalty).
   - `BimodalPredictor`: N-entry 2-bit saturating counter table (00: Strongly Not-Taken, 01: Weakly Not-Taken, 10: Weakly Taken, 11: Strongly Taken).
   - `GSharePredictor`: Global branch history register (BHR) XOR branch PC indexed saturating counter table.
3. Front-End Fetch Buffer:
   - Parametric `fetch_width` and `decode_width`.
   - Instruction queue between fetch and rename stages.

## Acceptance Criteria
- Unit tests verifying:
  - Bimodal state transitions on continuous taken/not-taken stream.
  - GShare history shift and aliasing behavior.
  - Fetch queue full stall and empty drain conditions.
