# Spec: TinyArmSim Core ISA Simulator & Self-Test Verification Suite

Status: ready-for-agent

## Problem Statement

Developers and computer architects exploring ARM processor architectures, compiler backends, or future Out-of-Order (OoO) pipeline designs need a lightweight, fast, and deterministic ARM CPU simulator in modern C++. Existing industrial simulators (e.g., full Gem5 or QEMU) are often too massive, monolithic, and complex to easily embed into custom testing pipelines, while simplistic toy interpreters tightly couple instruction execution with state management, making it impossible to reuse the frontend and verification harness for future OoO or JIT backends. Furthermore, developers need a reliable, automated way to verify simulator execution using actual cross-compiled ARM assembly test suites with deterministic pass/fail reporting.

## Solution

TinyArmSim provides a modular, high-extensibility ARM CPU simulation library and CLI runner written in C++17. It features:
1. A clean separation between `ArchitecturalState` (R0-R15 + CPSR/APSR), `MemoryBus` (flat 64MB physical address space), and execution engines.
2. A pipeline-friendly instruction decoder producing structured `DecodedInstruction` intermediate representations for ~50-55 core 16/32-bit mixed (Thumb/Thumb-2) instructions.
3. A sequential `IsaInterpreter` serving as the gold-standard functional execution engine.
4. A dual-mode `Loader` capable of loading statically-linked ELF32 executables and raw flat binary files.
5. An automated self-test verification framework that executes cross-compiled assembly test cases and intercepts `SVC #0` calls to evaluate return codes in register `R0`.

## User Stories

1. As an embedded software developer, I want to load and execute statically-linked ARM ELF32 binaries, so that I can run real compiled C and assembly programs in the simulator.
2. As a firmware engineer, I want to load raw flat binary images into specific physical memory addresses, so that I can test bare-metal payloads without ELF headers.
3. As a processor architect, I want the simulator to execute 16-bit and 32-bit mixed Thumb/Thumb-2 instructions, so that I can support modern compact ARM binaries.
4. As a developer, I want support for core arithmetic and logical instructions (`MOV`, `MVN`, `ADD`, `ADDS`, `ADC`, `SUB`, `SUBS`, `SBC`, `RSB`, `MUL`, `MLA`, `AND`, `ORR`, `EOR`, `BIC`), so that basic mathematical and bitwise operations execute accurately.
5. As a developer, I want support for condition flag updates and testing (`CMP`, `CMN`, `TST`, `TEQ`, `MOVS`, and barrel shifter operations `LSL`, `LSR`, `ASR`, `ROR`), so that conditional execution and loop counter logic work as specified in the ARM manual.
6. As a compiler engineer, I want wide immediate support (`MOVW`, `MOVT`), so that full 32-bit constants and address pointers can be loaded into registers in two instructions.
7. As a systems programmer, I want control flow and branching instructions (`B`, `B<cond>`, `BL`, `BX`, `BLX`, `CBZ`, `CBNZ`), so that function calls, returns, switch tables, and conditional branching execute faithfully.
8. As an assembly programmer, I want single register memory access (`LDR`, `LDRB`, `LDRH`, `LDRSB`, `LDRSH`, `STR`, `STRB`, `STRH`), so that local variables and data structures in memory can be read and written with correct signed and unsigned extensions.
9. As a software developer, I want stack and block transfer instructions (`PUSH`, `POP`, `LDM`, `STM`), so that standard ARM ABI function prologues and epilogues operate seamlessly.
10. As a system developer, I want status register access (`MRS`, `MSR`), so that processor mode and condition flags can be inspected and manipulated when necessary.
11. As a verification engineer, I want the simulator to trap `SVC` (Software Interrupt / Supervisor Call) instructions, so that test programs can signal completion and pass exit codes back to the host harness via register `R0`.
12. As a test automation engineer, I want `arm-none-eabi-gcc` assembly self-tests to compile and run automatically via CMake and CTest, so that every commit is regression-tested against ground truth.
13. As a CI runner in a container without an ARM cross-compiler, I want pre-compiled `.elf` test fixtures to run out-of-the-box, so that test suites pass reliably across diverse build environments.
14. As a simulator architect, I want invalid instructions, out-of-bound memory accesses, and unaligned accesses to throw strongly-typed `CpuFault` exceptions, so that erratic behavior is caught immediately and deterministically.
15. As a future microarchitect, I want the instruction decoder to produce a structured `DecodedInstruction` decoupled from the execution engine, so that the future Out-of-Order (OoO) engine can dispatch the same decoded instructions into Reservation Stations and Reorder Buffers without rewriting decoding logic.
16. As a future microarchitect, I want `ArchitecturalState` isolated from execution pipelines, so that branch misprediction flushes can cleanly discard in-flight speculative instructions and rollback to committed state.

## Implementation Decisions

1. **Modular Engine Seam**:
   - `ArchitecturalState` holds committed 32-bit general-purpose registers (R0-R12, SP/R13, LR/R14, PC/R15) and CPSR/APSR (NZCV flags).
   - `MemoryBus` represents a flat 64MB physical RAM space with byte (`read8`/`write8`), halfword (`read16`/`write16`), and word (`read32`/`write32`) accessors.
   - Execution engines implement a common interface accepting `ArchitecturalState` and `MemoryBus`.

2. **Decoupled Instruction Decoding**:
   - The `Decoder` takes a 16-bit or 32-bit instruction stream, determines instruction size and encoding format, and outputs a structured `DecodedInstruction` containing:
     - `Opcode` enumeration
     - `Condition` code (AL, EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE)
     - Destination and operand register indices (`Rd`, `Rn`, `Rm`, `Rs`)
     - Immediate value and shift operands (shift type, shift amount)
     - Flag modification bit (`set_flags`)
     - Memory transfer attributes (addressing mode, writeback, sign-extension, access size)
   - Invalid encodings trigger a typed `UndefinedInstructionFault`.

3. **Core Instruction Coverage (50-55 Instructions)**:
   - Data processing: `MOV`, `MVN`, `MOVT`, `MOVW`, `ADD`, `ADDS`, `ADC`, `SUB`, `SUBS`, `SBC`, `RSB`, `MUL`, `MLA`, `AND`, `ORR`, `EOR`, `BIC`, `CMP`, `CMN`, `TST`, `TEQ`, `ASR`, `LSL`, `LSR`, `ROR`
   - Branch: `B`, `B<cond>`, `BL`, `BX`, `BLX`, `CBZ`, `CBNZ`
   - Load/Store: `LDR`, `LDRB`, `LDRH`, `LDRSB`, `LDRSH`, `STR`, `STRB`, `STRH`, `LDM`, `STM`, `PUSH`, `POP`
   - System/Control: `MRS`, `MSR`, `SVC`, `NOP`

4. **Dual-Mode Loader**:
   - Supports ELF32 static binaries: parses 52-byte ELF header, identifies `PT_LOAD` segments, maps segments to target addresses in `MemoryBus`, and extracts `e_entry` into `ArchitecturalState.PC`.
   - Supports raw binary loading: writes binary payload sequentially starting from a configurable base address (default `0x00000000` or `0x00008000`).

5. **Self-Test Harness Contract**:
   - Test assembly programs are written with a standard macro header.
   - Upon test completion, assembly calls `SVC #0` with the test status in `R0` (0 = PASS, non-zero = FAIL code).
   - The simulator run loop exits on `SVC #0` and returns `R0` as the simulation exit code.

6. **Error and Fault Management**:
   - Memory accesses beyond the 64MB boundary or illegal access alignments throw `MemoryFaultException`.
   - Undefined or unhandled opcodes throw `UndefinedInstructionException`.

## Testing Decisions

1. **Test Seams**:
   - **Primary Seam (Highest level)**: The `IsaInterpreter::step()` and `IsaInterpreter::run()` API driving `ArchitecturalState` against `MemoryBus`.
   - **Decoder Seam**: The `Decoder::decode()` function converting raw bytes into `DecodedInstruction`.
   - **Loader Seam**: The `Loader::load_elf()` and `Loader::load_raw()` populating memory and setting initial PC.
   - **End-to-End Self-Test Seam**: Automated execution of compiled `.s` assembly test cases verifying `R0 == 0` on completion.

2. **Testing Strategy**:
   - TDD Red-Green loop for individual instruction semantics (unit testing data processing, branching, load/store, and flag updates).
   - Integration tests loading and running self-contained ARM assembly programs (e.g. arithmetic tests, loop tests, function call stack frames, sorting algorithms).
   - Pre-compiled ELF fixtures committed under `tests/fixtures/` to guarantee hermetic test runs even without `arm-none-eabi-gcc` installed.

## Out of Scope

- Virtual memory, MMU, page tables, and address translation.
- Dynamic linking, shared object (.so) loading, and dynamic symbols.
- Floating-point instructions (VFP, NEON, SIMD).
- Hardware peripheral simulation (UART, Timers, Interrupt Controllers) beyond flat RAM.
- Out-of-Order (OoO) pipeline, Reservation Stations, and Reorder Buffer (reserved for Cycle Engine phase).
- Dynamic binary translation / JIT compiler backend (reserved for JIT Engine phase).
- Multicore synchronization and atomic instructions (`LDREX`, `STREX`).

## Further Notes

- The project is built with standard C++17 and CMake 3.15+.
- When `arm-none-eabi-gcc` is detected in the system `PATH`, CMake will automatically generate build targets to assemble `.s` files in `tests/asm/` into `.elf` binaries.
