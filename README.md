# TinyCpuSim

[![Build & Test](https://img.shields.io/badge/tests-156%2F156%20passed-brightgreen.svg)]()
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![ISA](https://img.shields.io/badge/ISA-ARMv7--M%20%2F%20Thumb--2-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

**TinyCpuSim** is a high-performance, cycle-accurate Out-of-Order (OoO) superscalar CPU simulator and functional ISA emulator for ARMv7-M (Thumb-2) written in modern C++17.

TinyCpuSim provides an end-to-end simulation environment featuring an advanced Tomasulo/ROB microarchitecture, non-blocking multi-level cache hierarchy with MESI coherence, Intel/ARM Top-Down microarchitecture analysis (TMAM), comprehensive 40+ hardware performance counters, dynamic Region of Interest (ROI) profiling via `m5ops`, and automated golden accuracy calibration against **gem5**.

---

## Table of Contents

- [Key Architectural Features](#key-architectural-features)
- [System Requirements & Prerequisites](#system-requirements--prerequisites)
- [Quickstart Guide (1-Click Workflow)](#quickstart-guide-1-click-workflow)
- [Sequential 5-Step Workflow Guide](#sequential-5-step-workflow-guide)
  - [Step 1: Building the Simulator (`01_build.sh`)](#step-1-building-the-simulator-01_buildsh)
  - [Step 2: Full Unit & Regression Tests (`02_run_tests.sh`)](#step-2-full-unit--regression-tests-02_run_testssh)
  - [Step 3: Component Microbenchmarks & Sweeps (`03_run_ubench.sh`)](#step-3-component-microbenchmarks--sweeps-03_run_ubenchsh)
  - [Step 4: Full-System Simulation & Custom Configs (`04_run_simulation.sh`)](#step-4-full-system-simulation--custom-configs-04_run_simulationsh)
  - [Step 5: gem5 Golden Reference Accuracy Comparison (`05_compare_gem5.sh`)](#step-5-gem5-golden-reference-accuracy-comparison-05_compare_gem5sh)
- [Hardware Performance Counters & Diagnostics (`--all-perf`)](#hardware-performance-counters--diagnostics---all-perf)
- [Microarchitecture Parameter Sweeps (`sweep_parameters.py`)](#microarchitecture-parameter-sweeps-sweep_parameterspy)
- [Microarchitecture Configuration Presets](#microarchitecture-configuration-presets)
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

- **Comprehensive 40+ Hardware Performance Counters & TMAM**:
  - Full execution port utilization, stall cycles, branch diagnostics, LSU forwarding rates, and cache/MSHR counters.
  - Cycle-accurate Top-Down slot accounting (Level 1 & Level 2 breakdown).
  - Dynamic Region of Interest (ROI) statistics collection via `m5_reset_stats()` and `m5_dump_stats()`.

---

## System Requirements & Prerequisites

TinyCpuSim has minimal dependencies and runs seamlessly on macOS and Linux.

- **Operating System**: macOS (Apple Silicon / Intel) or Linux (Ubuntu 20.04+, Debian, Fedora, Arch).
- **C++ Compiler**: Clang (`clang++ >= 11` / Apple Clang 13+) or GCC (`g++ >= 9`) with C++17 support.
- **Build System**: CMake `>= 3.15` and Make or Ninja.
- **Python**: Python 3.8+ (used for parameter sweeps and gem5 regression).

---

## Quickstart Guide (1-Click Workflow)

Anyone cloning the repository can immediately run the unified interactive menu or direct shortcuts:

```bash
# 1. Clone the repository
git clone git@github.com:Kuanwei1225/TinyCpuSim.git
cd TinyCpuSim

# 2. Launch the interactive manager
./run.sh
```

Or run direct shortcuts:
```bash
./run.sh build                  # [Step 1] Build simulator and all tests
./run.sh test                   # [Step 2] Run 156+ unit & regression tests
./run.sh ubench all             # [Step 3] Run isolated microbenchmarks
./run.sh sim tests/fixtures/test_fibonacci.elf --all-perf   # [Step 4] Run full simulation
./run.sh gem5 --all             # [Step 5] Compare against gem5 golden
./run.sh sweep --param=rob_size # Run automated parameter sweep
```

---

## Sequential 5-Step Workflow Guide

For step-by-step sequential execution, simply follow the numbered scripts in order:

### Step 1: Building the Simulator (`01_build.sh`)
```bash
./scripts/01_build.sh
```
Automatically detects system CPU cores, configures CMake Release mode, and builds the `tinycpusim` binary alongside all 156+ test executables.

---

### Step 2: Full Unit & Regression Tests (`02_run_tests.sh`)
```bash
./scripts/02_run_tests.sh
```
Executes all 156 unit and microarchitecture test cases with 100% pass guarantee.

---

### Step 3: Component Microbenchmarks & Sweeps (`03_run_ubench.sh`)

When developing, refactoring, or optimizing individual processor submodules, execute isolated microbenchmarks:

```bash
# Run all 35 microbenchmark cases
./scripts/03_run_ubench.sh all

# Test Branch Predictor & Frontend (12 tests)
./scripts/03_run_ubench.sh bpu

# Test Execution Engine & LSU / Forwarding (12 tests)
./scripts/03_run_ubench.sh exec

# Test ROB & Top-Down Profiler (11 tests)
./scripts/03_run_ubench.sh rob

# Test Cache Hierarchy, MSHR & MESI Coherence (6 tests)
./scripts/03_run_ubench.sh cache

# Filter specific corner case
./scripts/03_run_ubench.sh exec --gtest_filter=*ExactStoreToLoadForwarding*
```

---

### Step 4: Full-System Simulation & Custom Configs (`04_run_simulation.sh`)

Run bare-metal ARM ELF programs on the cycle-accurate Out-of-Order processor:

```bash
# Basic run with default OoO configuration (Medium)
./scripts/04_run_simulation.sh tests/fixtures/test_fibonacci.elf

# Run with full 40+ hardware performance counters displayed
./scripts/04_run_simulation.sh tests/fixtures/test_fibonacci.elf --all-perf

# Run with custom microarchitecture config (e.g. 8-wide core)
./scripts/04_run_simulation.sh tests/fixtures/test_stress.elf configs/ooo_wide.cfg --all-perf

# Run pure functional ISA simulation
./scripts/04_run_simulation.sh tests/fixtures/test_arithmetic.elf --isa-only
```
> **Note**: Every simulation run automatically saves a full timestamped performance report to the `reports/` folder (e.g. `reports/test_fibonacci_20260926_214831.txt`).

---

### Step 5: gem5 Golden Reference Accuracy Comparison (`05_compare_gem5.sh`)

Compare TinyCpuSim's execution statistics against the cycle-accurate **gem5** golden reference:

```bash
# Run batch regression across all 9 benchmark suites
./scripts/05_compare_gem5.sh --all

# Compare single benchmark
./scripts/05_compare_gem5.sh --case=test_fibonacci
```

---

## Hardware Performance Counters & Diagnostics (`--all-perf`)

Running with `--all-perf` (or `-v`) outputs all 40+ hardware counters across all processor stages:

```text
============================================================
               TinyCpuSim uArch Simulation Report           
============================================================
Simulated Target Clock:    1000.00 MHz
Simulated Total Cycles:    2132 ticks
Total Committed Insts:     1708
Total Committed uOps:      3301
Aggregate Throughput (IPC):0.801 inst/cycle (uOp IPC: 1.548)
------------------------------------------------------------
[ Core 0 Summary ]
  Committed Insts / uOps:  1708 / 3301 (uOp Ratio: 1.93x)
  Core Throughput (IPC):   0.801 inst/cycle
  Branch Predictions:      911 (Accuracy: 74.2%)
  Branch Mispredicts:      235 (Penalty Flushes: 235)
  L1I Cache Hit Rate:      99.96% (5608/5610)
  L1D Cache Hit Rate:      99.64% (1107/1111)
  Loads / Stores:          634 / 531
  Store-to-Load Forwards:  54 (Rate: 8.52%)
  Mem Order Violations:    0
  --- Top-Down Breakdown (Level 1) ---
    Frontend Bound:        18.42%
    Bad Speculation:       4.12%
    Backend Bound:         42.85%
    Retiring:              34.61%
  --- Detailed Execution Ports ---
    Port ALU uOps:         1465
    Port MUL uOps:         0
    Port DIV uOps:         0
    Port Branch uOps:      700
    Port LSU uOps:         1815
  --- Pipeline Stalls Breakdown ---
    ROB Full Stalls:       0 cycles
    RS/IQ Full Stalls:     0 cycles
    PRF Exhaustion Stalls: 0 cycles
    LQ / SQ Full Stalls:   0 / 0 cycles
    Head-of-ROB Stalls:    0 cycles
  --- Branch Predictor Diagnostics ---
    Direct Cond / Uncond:  321 / 0
    Calls / Returns:       379 / 0
    Indirect Branches:     0
    BTB Hits / Misses:     589 / 322 (Hit Rate: 64.6%)
    RAS Hits / Misses:     379 / 0 (Hit Rate: 100.0%)
  --- LSU & Memory Disambiguation ---
    Store-to-Load Forwards:54
    Store Data Replays:    0
    Memory Order Flushes:  0
  --- Cache & MSHR Subsystem ---
    L1I Misses / Evictions:2 / 0
    L1D Misses / Evictions:4 / 0
    L1D Dirty Writebacks:  0
    L1D MSHR Allocations:  0 (Stalls: 0)
============================================================
```

---

## Microarchitecture Parameter Sweeps (`sweep_parameters.py`)

Explore the design space by sweeping hardware parameters across target programs:

```bash
# Sweep Reorder Buffer (ROB) size (16, 32, 64, 128, 256)
python3 scripts/sweep_parameters.py --elf tests/fixtures/test_fibonacci.elf --param rob_size

# Sweep Issue Width (1-way, 2-way, 4-way, 8-way)
python3 scripts/sweep_parameters.py --elf tests/fixtures/test_stress.elf --param issue_width

# Sweep L1 Data Cache Size (8KB, 16KB, 32KB, 64KB)
python3 scripts/sweep_parameters.py --elf tests/fixtures/test_mem_stride.elf --param l1d_size

# Sweep Branch Predictor Algorithm (BIMODAL, GSHARE, TAGE)
python3 scripts/sweep_parameters.py --elf tests/fixtures/test_branch_pred.elf --param bp_type
```

#### Example Sweep Output:
```text
===========================================================================
  TinyCpuSim Parameter Sweep: [issue_width] on test_stress.elf
  Base Config: ooo_medium.cfg
===========================================================================
Configuration      | Cycles     | IPC      | Branch Acc   | L1D Hit Rate
---------------------------------------------------------------------------
Width=1-way        | 130029     | 0.923    | 100.0%       | 100.0%      
Width=2-way        | 80019      | 1.500    | 100.0%       | 100.0%      
Width=4-way        | 80013      | 1.500    | 100.0%       | 100.0%      
Width=8-way        | 80012      | 1.500    | 100.0%       | 100.0%      
===========================================================================
```

---

## Microarchitecture Configuration Presets

Configuration files in `configs/` allow customization of core and cache hierarchy parameters:

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

## Project Directory Structure

```text
TinyCpuSim/
├── CMakeLists.txt            # Main CMake build configuration
├── README.md                 # Complete documentation & usage guide
├── run.sh                    # Top-level unified launcher & workflow manager
├── configs/                  # Microarchitecture configuration presets
│   ├── ooo_narrow.cfg
│   ├── ooo_medium.cfg
│   └── ooo_wide.cfg
├── include/tinyarmsim/       # Public headers
│   ├── isa/                  # ISA decoder, instruction definitions, register state
│   ├── memory/               # Memory system, non-blocking caches, MESI coherence
│   └── uarch/                # OoO pipeline: BPU, Frontend, PRF, Issue, Exec, LSU, ROB, TMAM
├── src/                      # Source implementations
│   ├── main.cpp              # CLI driver with --uarch, --all-perf, and options
│   ├── isa/                  # ISA simulation & disassembler
│   ├── memory/               # Memory bus & cache controller
│   └── uarch/                # Cycle-accurate OoO pipeline stages
├── scripts/                  # Step-by-step workflow scripts
│   ├── 01_build.sh           # Step 1: 1-click build script
│   ├── 02_run_tests.sh       # Step 2: Full test suite runner (156 tests)
│   ├── 03_run_ubench.sh      # Step 3: Component microbenchmark runner
│   ├── 04_run_simulation.sh  # Step 4: Full-system simulation runner
│   ├── 05_compare_gem5.sh    # Step 5: gem5 golden comparison tool
│   ├── sweep_parameters.py   # Hardware parameter sweep utility
│   └── compare_with_gem5.py  # gem5 accuracy comparator
├── reports/                  # Automatically saved simulation performance logs
└── tests/                    # Tests and benchmarks
    ├── fixtures/             # Bare-metal ELF binaries & assembly sources
    ├── golden/gem5/          # gem5 reference statistics logs
    └── unit & ubench tests   # 156 CTest GoogleTest cases
```

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.
