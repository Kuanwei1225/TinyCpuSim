# Issue 01: Pre-Generated Disassembler & DecodedInstruction Enhancement

Status: ready-for-agent
Type: task

## Description
Enhance `DecodedInstruction` to hold pre-computed disassembly strings and execution handler pointers. Implement a dedicated disassembler utility that formats the instruction string once during opcode decoding.

## Acceptance Criteria
- `DecodedInstruction` contains:
  - `std::string disasm` (pre-computed disassembly text, e.g., `movs r1, #10`, `ldr r2, [r4, #0]`)
  - `raw_hex` and size (`instr_size`)
  - Function pointer / handler for direct execution
- Unit tests verifying disassembly string accuracy across all 45 opcodes.
