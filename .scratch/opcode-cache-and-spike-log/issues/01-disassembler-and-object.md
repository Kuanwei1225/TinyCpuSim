# Issue 01: Pre-Generated Disassembler & DecodedInstruction Enhancement

Status: resolved
Type: task

## Description
Enhance `DecodedInstruction` to hold pre-computed disassembly strings and execution handler pointers. Implement a dedicated disassembler utility that formats the instruction string once during opcode decoding.

## Acceptance Criteria
- `DecodedInstruction` contains:
  - `std::string disasm` (pre-computed disassembly text, e.g., `movs r1, #10`, `ldr r2, [r4, #0]`)
  - `raw_hex` and size (`instr_size`)
  - Function pointer / handler for direct execution
- Unit tests verifying disassembly string accuracy across all 45 opcodes.

## Answer
- Created [`include/tinyarmsim/disassembler.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/disassembler.hpp) with full ARM Thumb / Thumb-2 disassembly string generation.
- Enhanced `DecodedInstruction` in [`include/tinyarmsim/instruction.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/instruction.hpp) with `raw_hex` and `disasm`.
- Integrated `Disassembler::disassemble()` inside [`include/tinyarmsim/decoder.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/decoder.hpp) so every decoded instruction is pre-formatted once at decode time.
- Added comprehensive unit tests in [`tests/disassembler_test.cpp`](file:///Users/kuanwei/workspace/TinySim/tests/disassembler_test.cpp) (58/58 CTest test cases passing).
