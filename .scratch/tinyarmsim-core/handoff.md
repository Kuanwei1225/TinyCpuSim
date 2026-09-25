# TinyArmSim Continuation & Handoff Summary

## Current Architecture & State
- **Language & Build**: C++17, CMake, Ninja / Make, Clang / GCC, ARM Toolchain (`arm-none-eabi-gcc`).
- **Core Components**:
  - `ArchitecturalState` ([`include/tinyarmsim/state.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/state.hpp)): 16 general registers (r0-r15), CPSR (NZCV flags).
  - `MemoryBus` ([`include/tinyarmsim/memory_bus.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/memory_bus.hpp)): 64MB RAM, little-endian 8/16/32-bit read/write with alignment enforcement.
  - `Decoder` ([`include/tinyarmsim/decoder.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/decoder.hpp)): 16-bit and 32-bit Thumb/Thumb-2 instruction decoding.
  - `IsaInterpreter` ([`include/tinyarmsim/interpreter.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/interpreter.hpp)): Execution engine, trace logging, condition code evaluation, simulation profiling metrics (instructions, elapsed time, MIPS), and opcode CSV coverage exporter.
  - `Loader` ([`include/tinyarmsim/loader.hpp`](file:///Users/kuanwei/workspace/TinySim/include/tinyarmsim/loader.hpp)): Static ELF32 parser and flat binary loader.
  - `CLI` ([`src/main.cpp`](file:///Users/kuanwei/workspace/TinySim/src/main.cpp)): Supports `--elf <path>`, `--log`/`--verbose`, `--coverage <csv_path>`, `--max-steps <N>`, and prints exit banner with status (`SIMULATION PASSED / FAILED`), instruction counts, and MIPS simulation speed.

## Test Suite & Verification
- **Unit & Integration Tests**: 54/54 CTest / GoogleTest test cases passing.
- **Assembly Self-Tests** in `tests/fixtures/`:
  - `test_arithmetic.elf`: basic arithmetic, logic, shifts.
  - `test_fibonacci.elf`: recursive function calls (`bl`/`bx`), stack frames (`push`/`pop`).
  - `test_sort.elf`: bubble sort on memory array (`ldr`/`str`).
  - `test_isa_coverage.elf`: comprehensive test pattern exercising all 45 implemented opcodes.
- **Automated Verification Script**:
  - `python3 scripts/verify_coverage.py`: runs all test fixtures, aggregates execution counts, checks $\ge 3$ executions for all 45 opcodes (100% coverage achieved), and outputs a formatted report.

## Quick Commands
```bash
# Build project and fixtures
cmake -B build -S . && cmake --build build

# Run all CTests
ctest --test-dir build --output-on-failure

# Run ISA Coverage verification script
python3 scripts/verify_coverage.py

# Run standalone simulation on any ELF
./build/tinyarmsim --elf tests/fixtures/test_sort.elf --log --coverage sort_cov.csv
```
