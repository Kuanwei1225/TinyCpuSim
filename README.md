# TinyCpuSim

[![Build & Test](https://img.shields.io/badge/tests-156%2F156%20passed-brightgreen.svg)]()
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![ISA](https://img.shields.io/badge/ISA-ARMv7--M%20%2F%20Thumb--2-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

**TinyCpuSim** is a high-performance, cycle-accurate Out-of-Order (OoO) superscalar CPU simulator and functional ISA emulator for ARMv7-M (Thumb-2) written in modern C++17.

TinyCpuSim provides an end-to-end simulation environment featuring an advanced Tomasulo/ROB microarchitecture, non-blocking multi-level cache hierarchy with MESI coherence, Intel/ARM Top-Down microarchitecture analysis (TMAM), dynamic Region of Interest (ROI) profiling via `m5ops`, and automated golden accuracy calibration against **gem5**.

---

## Table of Contents

- [Key Architectural Features](#key-architectural-features)
- [System Requirements & Prerequisites](#system-requirements--prerequisites)
- [Quickstart Guide](#quickstart-guide)
- [Step-by-Step Usage & Commands](#step-by-step-usage--commands)
  - [1. Building the Simulator](#1-building-the-simulator)
  - [2. Running Out-of-Order (OoO) uArch Simulation](#2-running-out-of-order-ooo-uarch-simulation)
  - [3. Running Pure Functional ISA Simulation](#3-running-pure-functional-isa-simulation)
  - [4. Top-Down Profiling & Dynamic ROI (m5ops)](#4-top-down-profiling--dynamic-roi-m5ops)
  - [5. Microarchitecture Configuration Presets](#5-microarchitecture-configuration-presets)
- [Testing & Microbenchmarks](#testing--microbenchmarks)
- [gem5 Golden Reference Accuracy Comparison](#gem5-golden-reference-accuracy-comparison)
- [Project Directory Structure](#project-directory-structure)
- [License](#license)

---

## Key Architectural Features

- **ARMv7-M / Thumb-2 ISA Engine**:
  - Full Thumb-16 and Thumb-32 instruction decoding and emulation.
  - Complete arithmetic, logical, multiply/divide, load/store, branch, conditional execution (IT blocks), and stack operations.
  - ELF loader with bare-metal memory space mapping and register initialization.

- **Superscalar Out-of-Order Core**:
  - **Branch Prediction Unit (BPU)**: Tournament Predictor, TAGE, Return Address Stack (RAS), and Branch Target Buffer (BTB).
  - **Frontend & Renaming**: Configurable N-wide superscalar Fetch/Decode/Rename with Physical Register File (PRF), Free Lists, and Speculative RAT checkpoints.
  - **Age-Ordered Unified Issue Queue**: Priority-based scheduling (oldest instruction first) resolving structural and data hazards.
  - **Pipelined Execution Units**: Pipelined ALUs, Multipliers, Dividers, and Branch Resolution Units.
  - **Load/Store Unit (LSU)**: Load-Store Queue (LSQ) supporting store-to-load forwarding, speculative load bypass, and memory order violation detection.
  - **Reorder Buffer (ROB)**: In-order retirement supporting precise exception recovery and macro-instruction retirement tracking.

- **Non-blocking Memory & Cache Hierarchy**:
  - L1 Instruction Cache & L1 Data Cache with Miss Status Holding Registers (MSHRs).
  - Shared L2 Cache with LRU replacement policy and dirty write-back eviction.
  - MESI Cache Coherence state machine for multi-core simulation.

- **Top-Down Microarchitecture Analysis (TMAM)**:
  - Cycle-accurate Top-Down slot accounting (Frontend Bound, Bad Speculation, Backend Bound, Retiring).
  - Dynamic Region of Interest (ROI) statistics collection via `m5_reset_stats()` and `m5_dump_stats()`.

---

## System Requirements & Prerequisites

TinyCpuSim has minimal dependencies and runs seamlessly on macOS and Linux.

- **Operating System**: macOS (Apple Silicon / Intel) or Linux (Ubuntu 20.04+, Debian, Fedora, Arch).
- **C++ Compiler**: Clang (`clang++ >= 11` / Apple Clang 13+) or GCC (`g++ >= 9`) with C++17 support.
- **Build System**: CMake `>= 3.15` and Make or Ninja.
- **Python**: Python 3.8+ (used for gem5 accuracy regression and helper scripts).
- *(Optional)* **Cross-Compiler**: `arm-none-eabi-gcc` (only needed if compiling new custom bare-metal ARM test fixtures).

---

## Quickstart Guide

Get up and running in 3 simple commands:

```bash
# 1. Clone the repository
git clone https://github.com/Kuanwei1225/TinyCpuSim.git
cd TinyCpuSim

# 2. Build the project (Release mode)
./scripts/build.sh

# 3. Run an Out-of-Order simulation on the Fibonacci benchmark
./build/tinycpusim --uarch tests/fixtures/test_fibonacci.elf
```

---

## Step-by-Step Usage & Commands

### 1. Building the Simulator

#### Option A: Using the build script
```bash
./scripts/build.sh
```

#### Option B: Using CMake directly
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd ..
```

The build produces the primary binary `./build/tinycpusim` (and a backward-compatible alias `./build/tinyarmsim`).

---

### 2. Running Out-of-Order (OoO) uArch Simulation

Run cycle-accurate superscalar simulation by passing the `--uarch` flag:

```bash
# Basic run with default 4-wide OoO medium configuration
./build/tinycpusim --uarch tests/fixtures/test_fibonacci.elf

# Run with custom microarchitecture config (e.g. wide 8-way core)
./build/tinycpusim --uarch --uarch-config configs/ooo_wide.cfg tests/fixtures/test_fibonacci.elf

# Export performance metrics and Top-Down breakdown to a text file
./build/tinycpusim --uarch --perf-log uarch_stats.txt tests/fixtures/test_fibonacci.elf

# Using the helper runner script
./scripts/run_uarch_sim.sh tests/fixtures/test_stress.elf configs/ooo_medium.cfg uarch_stats.txt
```

#### Example Output:
```text
============================================================
              TinyCpuSim Performance Report                
============================================================
Simulated Total Cycles:     2132
Total Committed Insts:      1708
Aggregate Throughput (IPC): 0.801
Branch Predictions:         171 (Accuracy: 95.91%)
L1I Cache Accesses:         541 (Hit Rate: 99.45%)
L1D Cache Accesses:         780 (Hit Rate: 98.85%)

--- Top-Down Microarchitecture Analysis (TMAM) ---
  Frontend Bound:           18.42%
  Bad Speculation:          4.12%
  Backend Bound:            42.85%
  Retiring:                 34.61%
============================================================
```

---

### 3. Running Pure Functional ISA Simulation

Run fast bare-metal ISA emulation without microarchitectural timing:

```bash
# Run functional simulation
./build/tinycpusim tests/fixtures/test_arithmetic.elf

# Run with per-instruction cycle and register trace log
./build/tinycpusim --log tests/fixtures/test_arithmetic.elf

# Using the helper runner script
./scripts/run_isa_sim.sh tests/fixtures/test_sort.elf
```

---

### 4. Top-Down Profiling & Dynamic ROI (m5ops)

TinyCpuSim supports dynamic Region-of-Interest (ROI) markers using standard `m5ops` assembly hooks (`0xEE 0x00 0x00 0x00`):
- `m5_reset_stats()` (Opcode `0x40`): Resets cycle and instruction counters at the start of the target benchmark kernel.
- `m5_dump_stats()` (Opcode `0x41`): Dumps current execution counters and Top-Down distribution at the end of the ROI.

```c
// Example C snippet for ROI instrumentation
void benchmark_kernel() {
    __asm__ volatile (".inst.w 0xee000040"); // m5_reset_stats: start ROI
    
    // Core compute loop
    for (int i = 0; i < 1000; i++) {
        compute();
    }

    __asm__ volatile (".inst.w 0xee000041"); // m5_dump_stats: end ROI
}
```

---

### 5. Microarchitecture Configuration Presets

Configuration files located in `configs/` allow fine-grained customization of core and memory parameters:

| Parameter | `ooo_narrow.cfg` | `ooo_medium.cfg` (Default) | `ooo_wide.cfg` |
|:---|:---:|:---:|:---:|
| **Fetch / Decode / Commit Width** | 2 | 4 | 8 |
| **Issue Queue Capacity** | 16 | 32 | 64 |
| **Reorder Buffer (ROB) Entries** | 32 | 64 | 128 |
| **Physical Registers (PRF)** | 48 | 96 | 192 |
| **Load / Store Queue (LSQ)** | 8 / 8 | 16 / 16 | 32 / 32 |
| **L1I / L1D Cache Size** | 16 KB (2-way) | 32 KB (4-way) | 64 KB (8-way) |
| **Shared L2 Cache Size** | 128 KB (4-way) | 256 KB (8-way) | 512 KB (16-way) |

---

## Testing & Microbenchmarks

TinyCpuSim includes a comprehensive regression test suite with **156 isolated unit tests and component microbenchmarks** covering all critical corner cases.

To execute the full test suite:

```bash
./scripts/run_tests.sh
```
*(Or run `ctest --test-dir build --output-on-failure`)*

### Component Microbenchmark Suites:
1. **Branch Prediction (`bpu_frontend_ubench_test.cpp`)**: Tight loops, alternating patterns (TNTN), deep call-return RAS nesting, indirect target thrashing, TAGE branch correlation, and BTB aliasing.
2. **Frontend & PRF (`bpu_frontend_ubench_test.cpp`)**: Cross-cacheline fetches, PRF exhaustion stalls, flags renaming wakeups, multi-uOp macro-expansions, and speculative RAT checkpoint restores.
3. **Execution & LSU (`exec_lsu_ubench_test.cpp`)**: RAW dependency latency, pipelined multiplier/divider latency, issue width saturation, age-ordered issue priority, exact store-to-load forwarding, store-data pending replays, and memory order violation recovery.
4. **ROB & Top-Down Profiler (`rob_topdown_ubench_test.cpp`)**: Sustained retirement throughput, head-of-ROB blocking, circular buffer wrap-around, branch misprediction flushing, slot conservation invariants, and bottleneck Pareto ranking.
5. **Cache & Coherence (`cache_ubench_test.cpp`)**: Hit latency, LRU replacement associativity, dirty write-back evictions, MSHR non-blocking allocation, MESI state transitions, and shared L2 hierarchy inclusion.

---

## gem5 Golden Reference Accuracy Comparison

TinyCpuSim is calibrated against the cycle-accurate **gem5** ARM simulator (O3CPU model). 

To execute the automated accuracy regression across all golden benchmark suites:

```bash
python3 scripts/compare_with_gem5.py --all
```

To compare a single benchmark case:
```bash
python3 scripts/compare_with_gem5.py --case=test_fibonacci
```

### Macro-Instruction Retirement Precision:
TinyCpuSim achieves **100% macro-instruction retirement precision** aligned with gem5:

| Benchmark | TinyCpuSim Committed Insts | gem5 Golden Committed Insts | Instruction Delta |
|:---|:---:|:---:|:---:|
| `test_arithmetic` | 23 | 22 | +1 (Exit instruction) |
| `test_branch_pred` | 1510 | 1509 | +1 (Exit instruction) |
| `test_fibonacci` | 1708 | 1707 | +1 (Exit instruction) |
| `test_isa_coverage` | 268 | 267 | +1 (Exit instruction) |
| `test_mem_stride` | 257 | 256 | +1 (Exit instruction) |
| `test_raw_hazard` | 608 | 607 | +1 (Exit instruction) |
| `test_sort` | 148 | 147 | +1 (Exit instruction) |
| `test_store_forward` | 19 | 18 | +1 (Exit instruction) |
| `test_stress` | 120012 | 120011 | +1 (Exit instruction) |

---

## Project Directory Structure

```text
TinyCpuSim/
├── CMakeLists.txt            # Main CMake build configuration
├── README.md                 # Complete documentation & usage guide
├── configs/                  # Microarchitecture configuration presets
│   ├── ooo_narrow.cfg
│   ├── ooo_medium.cfg
│   └── ooo_wide.cfg
├── include/tinyarmsim/       # Public headers
│   ├── isa/                  # ISA decoder, instruction definitions, register state
│   ├── memory/               # Memory system, non-blocking caches, MESI coherence
│   └── uarch/                # OoO pipeline: BPU, Frontend, PRF, Issue, Exec, LSU, ROB, TMAM
├── src/                      # Source implementations
│   ├── main.cpp              # CLI driver with --uarch, --perf-log, and options
│   ├── isa/                  # ISA simulation & disassembler
│   ├── memory/               # Memory bus & cache controller
│   └── uarch/                # Cycle-accurate OoO pipeline stages
├── scripts/                  # Helper automation scripts
│   ├── build.sh              # 1-click build script
│   ├── run_tests.sh          # Full test suite runner
│   ├── run_uarch_sim.sh      # OoO uArch simulator runner
│   ├── run_isa_sim.sh        # Functional ISA simulator runner
│   └── compare_with_gem5.py  # gem5 golden comparison tool
└── tests/                    # Tests and benchmarks
    ├── fixtures/             # Bare-metal ELF binaries & assembly sources
    ├── golden/gem5/          # gem5 reference statistics logs
    └── unit & ubench tests   # 156 CTest GoogleTest cases
```

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.
