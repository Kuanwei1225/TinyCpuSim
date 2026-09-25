# TinyArmSim Continuation & Handoff Summary

## Current Architecture & State
- **Language & Build**: C++17, CMake, Ninja / Make, Clang / GCC, ARM Toolchain (`arm-none-eabi-gcc`).
- **Core Components**:
  - `ArchitecturalState` ([`include/tinyarmsim/state.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/state.hpp)): 16 general registers (r0-r15), CPSR (NZCV flags).
  - `MemoryBus` ([`include/tinyarmsim/memory_bus.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/memory_bus.hpp)): 64MB RAM, little-endian 8/16/32-bit read/write with alignment enforcement.
  - `Decoder` ([`include/tinyarmsim/decoder.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/decoder.hpp)): 16-bit and 32-bit Thumb/Thumb-2 instruction decoding with pre-computed disassembly generation.
  - `Disassembler` ([`include/tinyarmsim/disassembler.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/disassembler.hpp)): Full ARM Thumb disassembler string formatter.
  - `OpcodeCache` ([`include/tinyarmsim/opcode_cache.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/opcode_cache.hpp)): Position-independent 64K lookup table for 16-bit Thumb and fast hash map for 32-bit Thumb (keyed by raw opcode hex, safe against GDB software breakpoints / instruction patches).
  - `TraceRecord` ([`include/tinyarmsim/trace.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/trace.hpp)): Spike-style single line commit trace capturing register writebacks, memory read/write accesses (with address, data, size), and NZCV flags.
  - `IsaInterpreter` ([`include/tinyarmsim/interpreter.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/interpreter.hpp)): High-speed execution engine with OpcodeCache, Spike commit trace logging, condition evaluation, simulation profiling metrics (instructions, elapsed time, MIPS), and opcode CSV coverage exporter.
  - `Loader` ([`include/tinyarmsim/loader.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/loader.hpp)): Static ELF32 parser and flat binary loader.
  - `CLI` ([`src/main.cpp`](file:///Users/kuanwei/workspace/TinySim/src/main.cpp)): Supports `--elf <path>`, `--log`/`--verbose`, `--coverage <csv_path>`, `--max-steps <N>`, and prints exit banner with status (`SIMULATION PASSED / FAILED`), instruction counts, and MIPS simulation speed.

## Test Suite & Verification
- **Unit & Integration Tests**: 61/61 CTest / GoogleTest test cases passing.
- **Assembly Self-Tests** in `tests/fixtures/`:
  - `test_arithmetic.elf`: basic arithmetic, logic, shifts.
  - `test_fibonacci.elf`: recursive function calls (`bl`/`bx`), stack frames (`push`/`pop`).
  - `test_sort.elf`: bubble sort on memory array (`ldr`/`str`).
  - `test_isa_coverage.elf`: comprehensive test pattern exercising all 45 implemented opcodes.
  - `test_stress.elf`: 5,000,000+ instruction computational loop achieving **73.79 MIPS** on Release build.
- **Automated Verification Script**:
  - `python3 scripts/verify_coverage.py`: runs all test fixtures, aggregates execution counts, checks $\ge 3$ executions for all 45 opcodes (100% coverage achieved), and outputs a formatted report.

## Quick Commands
```bash
# Build project and fixtures in Release mode
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build

# Run all CTests
ctest --test-dir build --output-on-failure

# Run ISA Coverage verification script
python3 scripts/verify_coverage.py

# Run standalone simulation on any ELF with Spike-style log
./build/tinyarmsim --elf tests/fixtures/test_sort.elf --log

# Run stress benchmark
./build/tinyarmsim --elf tests/fixtures/test_stress.elf --max-steps 20000000
```
