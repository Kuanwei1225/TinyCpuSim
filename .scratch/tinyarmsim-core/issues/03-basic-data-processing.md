# 03: Structured Decoder Seam and Basic Data Processing Instructions

**What to build:** The `DecodedInstruction` structured intermediate representation, the `Decoder` interface, and the initial vertical slice of `IsaInterpreter` implementing basic data processing instructions (`MOV`, `MVN`, `ADD`, `ADDS`, `SUB`, `SUBS`, `CMP`) with NZCV flag updates.

**Blocked by:** 02-architectural-state-and-memory-bus

**Status:** resolved

- [x] `DecodedInstruction` struct defines opcode, operands (`Rd`, `Rn`, `Rm`), immediates, condition codes, and flag modification bits
- [x] `Decoder` decodes basic 16/32-bit Thumb/ARM data processing opcodes into `DecodedInstruction`
- [x] `IsaInterpreter::step()` executes decoded instructions against `ArchitecturalState`
- [x] Tests verify arithmetic computation results and NZCV flag mutations for both positive and negative/overflow cases
