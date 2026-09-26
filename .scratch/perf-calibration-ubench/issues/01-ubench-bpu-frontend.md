# Issue 01: BPU and Frontend Isolated Microbenchmarks

Status: resolved

## Objective
Implement at least 5 isolated microbenchmarks for Branch Predictor Unit (BPU) and at least 5 for Frontend (Fetch, Decode, Rename, PRF/FreeList) covering critical corner cases.

## Seams Under Test
- `BranchPredictor` (`predict()`, `update()`, BTB, RAS, TAGE/GShare)
- `FetchUnit`, `UOpDecoder`, `RAT`, `PRF`, `FreeList`

## UBench Test Cases Implemented (12 tests total)
### BPU (6 tests):
1. `BPU_UBench_TightLoopAlwaysTaken`: Saturating counter 2-bit transitions and 100% steady-state hit.
2. `BPU_UBench_AlternatingPatternTNTN`: Bimodal vs GShare history correlation on alternating branch.
3. `BPU_UBench_DeepNestedCallReturnRAS`: Deep recursive call/return depth with RAS wrap-around.
4. `BPU_UBench_IndirectCallTargetThrashing`: Polymorphic function pointer target switching on BTB.
5. `BPU_UBench_CorrelatedBranchesTAGE`: Long-distance branch correlation on TAGE geometric history tables.
6. `BPU_UBench_BranchTargetBufferAliasStress`: BTB index aliasing stress.

### Frontend (6 tests):
1. `Frontend_UBench_CrossCacheLineFetch`: Mixed Thumb-16/32 across 64-byte cache line boundaries.
2. `Frontend_UBench_PrfExhaustionStall`: Rename queue stall on PRF/FreeList depletion and resume.
3. `Frontend_UBench_FlagsRenamingWakeup`: Flags register renaming and conditional wakeup.
4. `Frontend_UBench_MultiUopExpansionThroughput`: Multi-register PUSH/POP expansion into decoupled uops.
5. `Frontend_UBench_SpeculativeCheckpointRestore`: RAT checkpoint roll-back on branch misprediction.
6. `Frontend_UBench_DecoderIllegalOpcodeFault`: Invalid opcode decoding into HALT uop.

## Verification
- `bpu_frontend_ubench_test`: 12 / 12 passed.
- `ctest`: 127 / 127 passed (100%).
