# Wayfinding Map: Microarchitecture (uArch) & Out-of-Order Engine

## Effort Overview
Construct the modular cycle-accurate Out-of-Order (OoO) superscalar core simulation engine, non-blocking cache hierarchy with MESI coherence, dual-issue decoupled LSU, and multi-core coordinator for TinyArmSim.

## Phased Roadmap & Milestones

### Phase 1: Coherent Memory Hierarchy & Validation Demo (Completed)
- [x] **Issue 01**: Configuration Schema, Parser & Hardware Counters Reporter (`.scratch/uarch-ooo-engine/issues/01-config-and-stats.md`)
- [x] **Issue 02**: Parameterized Cache Subsystem (L1I, L1D, Shared L2, MSHR, Non-blocking) (`.scratch/uarch-ooo-engine/issues/02-cache-subsystem.md`)
- [x] **Issue 03**: Multi-Core MESI Snooping Coherence Protocol & Crossbar Interconnect (`.scratch/uarch-ooo-engine/issues/03-branch-predictor-and-frontend.md` -> Coherence ticket)
- [x] **Issue 04**: Phase 1 Cache Speedup Benchmark & MESI State Transition Demo

### Phase 2: Front-End, Branch Prediction & Micro-Op Decode
- [ ] **Issue 05**: Decoupled Fetch with GShare/Bimodal Predictor + BTB + RAS + Micro-Op Decode (Store split to STA/STD)

### Phase 3: Out-of-Order Core, Dual-Issue LSU & Memory Disambiguation
- [ ] **Issue 06**: Physical Register File (PRF), RAT & Free List with Branch Checkpointing (`.scratch/uarch-ooo-engine/issues/04-rename-and-free-list.md`)
- [ ] **Issue 07**: Reorder Buffer (ROB) & Issue Queue (Wakeup/Select) with Bypass Matrix (`.scratch/uarch-ooo-engine/issues/05-rob-and-issue-queue.md`)
- [ ] **Issue 08**: Dual-Issue Load/Store Unit (Dual AGU, LQ, SQ, Store-to-Load Forwarding, MOB Violation Squashing) (`.scratch/uarch-ooo-engine/issues/06-load-store-unit.md`)
- [ ] **Issue 09**: Integrated Multi-Core OoO Pipeline, CLI Integration (`--uarch-config`, `--uarch-stats`), and End-to-End Stress Verification (`.scratch/uarch-ooo-engine/issues/09-cli-and-benchmarks.md`)

### Phase 4: Multi-Core Workloads & Parallel Verification
- [ ] **Issue 10**: Multi-Core Execution, Workload Partitioning & Parallel Benchmark Verification (`.scratch/uarch-ooo-engine/issues/10-multicore-workloads.md`)

### Phase 5: Golden Reference & Precision Cross-Validation against gem5
- [ ] **Issue 11**: Golden Precision Cross-Validation against gem5 (RTL / Reference Target) (`.scratch/uarch-ooo-engine/issues/11-gem5-golden-precision-validation.md`)

## Key Architecture Decisions
- **Decoupled Store AGU/Data**: Store instructions are split into `uop_STA` and `uop_STD` so address resolution proceeds independently of data readiness.
- **Dual-Issue LSU Ports**: Port 2 (Load AGU) + Port 3 (Dual Load/Store AGU) + Port 4 (Store Data) allows 2 Loads/cycle or 1 Load + 1 Store/cycle.
- **Speculative Memory Disambiguation (MOB)**: Loads execute ahead of unresolved store addresses; when older `uop_STA` resolves, it snoops younger loads in LQ and triggers a Memory Violation Flush on RAW address collisions.
- **Non-blocking Cache & MSHR**: In-flight misses do not block the pipeline (Hit-under-Miss).
- **MESI Snooping Coherence**: M, E, S, I state transitions with BusRd, BusRdX, BusUpgr, BusWB broadcast events.
