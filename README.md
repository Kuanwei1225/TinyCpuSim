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

TinyCpuSim features a two-tiered testing methodology:
1. **Isolated Component Microbenchmarks (uBench)**: Fine-grained, cycle-by-cycle C++ unit tests to verify and stress individual submodules (BPU, PRF, Issue Queue, LSU, ROB, Caches) in isolation.
2. **End-to-End Bare-Metal ELF Benchmarks**: Full-system simulations on compiled ARM bare-metal binaries compared against **gem5**.

---

### Running All Tests
To run the complete test suite (156+ unit tests & microbenchmarks):
```bash
./scripts/run_tests.sh
```
*(Or run `ctest --test-dir build --output-on-failure`)*

---

### Running Component Microbenchmarks (uBench)

For rapid iterative development and submodule replacement (e.g. swapping in a new branch predictor, implementing a new LSU forwarding network, or hooking an external RTL co-simulator), use `./scripts/run_ubench.sh`:

```bash
# 1. Run all isolated microbenchmarks
./scripts/run_ubench.sh all

# 2. Run Branch Prediction & Frontend microbenchmarks (12 tests)
./scripts/run_ubench.sh bpu

# 3. Run Execution Engine & LSU / Forwarding microbenchmarks (12 tests)
./scripts/run_ubench.sh exec

# 4. Run Reorder Buffer (ROB) & Top-Down Profiler microbenchmarks (11 tests)
./scripts/run_ubench.sh rob

# 5. Run Cache Hierarchy, MSHR & MESI Coherence microbenchmarks (6 tests)
./scripts/run_ubench.sh cache

# 6. Run a specific test case with GoogleTest filter
./scripts/run_ubench.sh exec --gtest_filter=*ExactStoreToLoadForwarding*
./scripts/run_ubench.sh bpu  --gtest_filter=*CorrelatedBranchesTAGE*
./scripts/run_ubench.sh rob  --gtest_filter=*BottleneckParetoRanking*
./scripts/run_ubench.sh cache --gtest_filter=*MesiCoherence*
```

### Component Microbenchmark Suites Breakdown:
1. **Branch Prediction & Frontend (`bpu_frontend_ubench_test.cpp`)**:
   - `BPU_UBench_TightLoopAlwaysTaken`: 2-bit saturating counter warmup & sustained loop prediction.
   - `BPU_UBench_AlternatingPatternTNTN`: Periodic T-N-T-N branch pattern tracking.
   - `BPU_UBench_DeepNestedCallReturnRAS`: 16-level deep call/return stack recovery.
   - `BPU_UBench_IndirectCallTargetThrashing`: Dynamic indirect function pointer resolution.
   - `BPU_UBench_CorrelatedBranchesTAGE`: Long history geometric correlation via TAGE tables.
   - `BPU_UBench_BranchTargetBufferAliasStress`: BTB direct target caching and collision resilience.
   - `Frontend_UBench_CrossCacheLineFetch`: 32-bit Thumb-2 instructions spanning 64B cache line boundaries.
   - `Frontend_UBench_PrfExhaustionStall`: Zero-leakage stall & recovery under physical register starvation.
   - `Frontend_UBench_FlagsRenamingWakeup`: Speculative CPSR/flags dependency rename & broadcast.
   - `Frontend_UBench_MultiUopExpansionThroughput`: Multi-uOp macro-instruction expansions (e.g. `PUSH`/`POP`).
   - `Frontend_UBench_SpeculativeCheckpointRestore`: Exact RAT rollback on branch mispredictions.
   - `Frontend_UBench_DecoderIllegalOpcodeFault`: Undefined instruction decode fault trapping.

2. **Execution & LSU (`exec_lsu_ubench_test.cpp`)**:
   - `Exec_UBench_RawDependencyChainLatency`: Strict RAW latency serialization.
   - `Exec_UBench_MulDivPipelinedLatency`: Pipelined multi-cycle integer arithmetic.
   - `Exec_UBench_MaxIssueWidthSaturation`: Sustained N-wide superscalar throughput.
   - `Exec_UBench_AgeOrderedContentionIssue`: Age-based issue priority under resource contention.
   - `Exec_UBench_OutOfOrderConditionEvaluation`: Speculative condition code evaluation.
   - `Exec_UBench_ExecutionPortContention`: Multi-port execution binding and arbitration.
   - `LSU_UBench_ExactStoreToLoadForwarding`: 0-cycle store-queue to load bypass forwarding.
   - `LSU_UBench_StoreDataPendingReplay`: Safe stall and replay when store data is unready.
   - `LSU_UBench_MemoryOrderViolationDetection`: Store vs speculative load address collision squashing.
   - `LSU_UBench_L1CacheHitVsMissLatency`: Accurate hit vs miss cycle penalty differential.
   - `LSU_UBench_StridedAccessCacheThrashing`: Cache thrashing under non-contiguous strides.
   - `LSU_UBench_LoadStoreQueueWrapAround`: Circular LSQ head/tail index wrap-around stress.

3. **ROB & Top-Down Profiler (`rob_topdown_ubench_test.cpp`)**:
   - `ROB_UBench_SustainedRetireThroughput`: Sustained maximum commit width.
   - `ROB_UBench_HeadOfRobBlockingRetire`: In-order commit stalling on uncompleted head entry.
   - `ROB_UBench_CircularBufferWrapAroundStress`: 1000+ instruction ROB index wraparound.
   - `ROB_UBench_SpeculativeStoreDrainOnRetire`: Committed store buffer draining to memory.
   - `ROB_UBench_MultipleBranchMispredictFlushes`: Consecutive branch squash and younger uop purge.
   - `ROB_UBench_IsYoungerCircularAgeDistance`: High-precision circular age comparison.
   - `TopDown_UBench_SlotConservationInvariant`: Mathematical slot conservation law (`Sum == Total Slots`).
   - `TopDown_UBench_FrontendVsBackendBreakdown`: Quantitative stall slot classification.
   - `TopDown_UBench_BadSpeculationAccounting`: Slot waste tracking during mispredicted execution paths.
   - `ROB_UBench_RoiBoundaryResetStats`: Atomic stats reset at ROI boundary markers (`m5ops`).
   - `TopDown_UBench_BottleneckParetoRanking`: Automatic identification of primary performance bottlenecks.

4. **Cache & Coherence (`cache_ubench_test.cpp`)**:
   - `Cache_UBench_L1HitLatencyAndThroughput`: Single-cycle L1 cache access timing.
   - `Cache_UBench_LruReplacementSetAssociativity`: LRU set associativity replacement correctness.
   - `Cache_UBench_WriteBackDirtyEviction`: Dirty line eviction and memory writeback.
   - `Cache_UBench_MshrNonBlockingAllocation`: Non-blocking hit-under-miss via MSHR registers.
   - `Cache_UBench_MesiCoherenceStateTransitions`: MESI protocol transitions (Modified/Exclusive/Shared/Invalid).
   - `Cache_UBench_SharedL2HierarchicalInclusion`: Inclusive shared L2 cache behavior.

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
