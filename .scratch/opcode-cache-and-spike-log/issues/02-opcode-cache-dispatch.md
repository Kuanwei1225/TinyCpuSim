# Issue 02: Opcode-Keyed Cache & Direct Handler Dispatch

Status: resolved
Blocked by: 01
Type: task

## Description
Implement `OpcodeCache` indexed strictly by instruction opcode/word (not by PC):
- 16-bit Thumb: 64K lookup table (`std::array<DecodedInstruction, 65536>`) lazy-populated or pre-initialized on first encounter.
- 32-bit Thumb: Fast hash table / direct map keyed by 32-bit raw word `(w1 << 16) | w2`.
- Direct execution dispatch via function pointer / handler to bypass switch-case overhead.

## Acceptance Criteria
- Fetching and executing instructions via `OpcodeCache` yields bit-accurate architectural state.
- GDB/self-modifying code safety: modifying instruction bytes at any PC immediately executes the new instruction opcode.
- All existing 54 unit tests pass.

## Answer
- Created [`include/tinyarmsim/opcode_cache.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/opcode_cache.hpp) with 64K Thumb-16 flat array table and 32-bit hash cache keyed strictly by raw opcode words.
- Decoded instructions are 100% position-independent; branch target calculations dynamically evaluate relative offsets against current runtime PC.
- Integrated `OpcodeCache` into `IsaInterpreter::step()`.
- Verified all 58/58 unit tests and assembly integration tests pass.
