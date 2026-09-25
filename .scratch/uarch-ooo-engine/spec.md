# Specification: TinyArmSim Microarchitecture (uArch) & Out-of-Order Engine

## 1. Overview & Goals
The TinyArmSim microarchitecture (uArch) simulation engine introduces a cycle-accurate / cycle-approximate Out-of-Order (OoO) superscalar core model, non-blocking cache hierarchy with MESI coherence, and multi-core coordinator on top of the functional ISA interpreter.

---

## 2. OoO Pipeline Architecture & Cycle Stages

```
====================================================================================================
                                      OoO Pipeline Stages
====================================================================================================

+--------------------------------------------------------------------------------------------------+
| 1. FETCH (IF1 / IF2)                                                                             |
|    - 4-wide fetch from L1 I-Cache with Branch Predictor (GShare/Bimodal) + BTB (4K) + RAS (32)   |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 2. DECODE (ID)                                                                                   |
|    - Unpacks Thumb/ARM instructions into atomic micro-ops (uops).                                |
|    - Decouples Store instructions into: uop_STA (Store Address) + uop_STD (Store Data).          |
|    - Expands LDM/STM/PUSH/POP into multiple single-register load/store uops.                     |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 3. RENAME / ALLOCATE (RN/AL)                                                                     |
|    - Speculative Register Alias Table (RAT): R0..R15 -> Physical Registers (P0..P127).           |
|    - Allocates Free List entries, ROB slots (64 entries), RS slots (32 entries).                 |
|    - Allocates Load Queue (LQ: 16) & Store Queue (SQ: 16) entries.                              |
|    - Checkpoints RAT on conditional/indirect branches.                                           |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 4. DISPATCH / ISSUE QUEUE (RS)                                                                   |
|    - Holds dispatched uops waiting for physical register source operands to become ready.        |
|    - Dynamic scoreboard tag match (Wakeup) on producer writeback broadcast.                      |
|    - Select logic picks oldest ready uops for Execution Ports 0..4 (Issue Width: 4..5 uops/cyc). |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 5. EXECUTION UNITS (EX) & DUAL-ISSUE LSU PORTS                                                   |
|                                                                                                  |
|    Port 0: Simple ALU / Branch / Flags (1 cyc)                                                   |
|    Port 1: Complex ALU / Shift / Multiplier (1..3 cyc)                                           |
|    Port 2: LSU Load AGU + Cache Read Port 0 (1 cyc L1D hit)                                      |
|    Port 3: LSU Load/Store AGU (Dual AGU for 2nd Load or Store Address uop_STA)                   |
|    Port 4: Store Data Unit (Writes data payload uop_STD into SQ buffer)                          |
|                                                                                                  |
|    * Sustained memory throughput: Dual-Issue (2 Loads/cyc OR 1 Load + 1 Store/cyc)               |
|    * Execution Bypass / Forwarding Matrix delivers results directly to dependent RS entries.     |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 6. LOAD/STORE UNIT (LSU) & SPECULATIVE MEMORY DISAMBIGUATION (MOB)                               |
|                                                                                                  |
|    - Store Queue (SQ): In-flight stores buffered with resolved address & data.                   |
|    - Store-to-Load Forwarding:                                                                   |
|        * Exact Address Match: Older uncommitted store forwards data directly in 1 cycle.        |
|        * No Overlap: Non-blocking L1 D-Cache read issued immediately (Hit-under-Miss via MSHR).  |
|    - Speculative Load Execution & Memory Order Violation Monitor:                               |
|        * Younger loads execute speculatively before older store addresses are known.             |
|        * When older uop_STA calculates its address, it snoops younger executed loads in LQ.      |
|        * If an address aliasing RAW hazard is detected: Triggers Memory Order Violation Flush!   |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 7. WRITEBACK (WB)                                                                                |
|    - Physical register results written to PRF.                                                   |
|    - Completion flags broadcast to ROB and wake up waiting instructions in RS.                  |
+--------------------------------------------------------------------------------------------------+
                                                 |
                                                 v
+--------------------------------------------------------------------------------------------------+
| 8. COMMIT / RETIRE (CM)                                                                          |
|    - Strictly in-order retirement from ROB head (up to commit_width = 4 uops/cyc).               |
|    - Store Commit: Drains committed store from SQ to L1 D-Cache (initiating MESI bus requests).  |
|    - Frees superseded physical registers back to Free List.                                      |
|    - Updates Architectural / Commit RAT.                                                         |
+--------------------------------------------------------------------------------------------------+
```

---

## 3. Detailed Subsystem Specifications

### 3.1 LSU Microarchitecture & Dual-Issue AGU
- **Address Generation Units (AGUs)**:
  - **AGU 0 (Port 2)**: Dedicated Load Address generation (`uop_LDA`) and L1D read port.
  - **AGU 1 (Port 3)**: Multi-function AGU capable of computing Store Address (`uop_STA`) or a 2nd concurrent Load Address (`uop_LDA`).
- **Store Micro-op Decoupling**:
  - `STR / STRB / STRH`: Decoded into two separate micro-ops:
    - $\mu\text{op}_{\text{STA}}$ ($R_n + \text{imm}/R_m \to \text{SQ Address}$)
    - $\mu\text{op}_{\text{STD}}$ ($R_d \to \text{SQ Data}$)
  - Allows store addresses to resolve early in the pipeline to unblock younger loads even if store data is still computing.
- **Store-to-Load Forwarding Engine**:
  - Checks SQ entries older than the load (in program order).
  - Handles exact 4B/2B/1B match, byte-enable alignments, and partial forwarding stalls.
- **Memory Order Buffer (MOB) & Violation Recovery**:
  - Tracks all in-flight speculative loads.
  - Re-evaluates load validity when older stores commit address resolution.
  - On memory hazard violation, squashes the offending load and all younger instructions in the ROB, rolling back RAT to the checkpoint.

### 3.2 Non-Blocking Cache & MSHR (Miss Status Holding Registers)
- **L1 Data Cache**:
  - Configurable: 32KB, 4-way set associative, 64B line size, 2 read ports, 1 write port.
  - **MSHR**: 8 entries tracking in-flight misses, supporting **Hit-under-Miss** and **Miss-under-Miss**.
- **L1 Instruction Cache**:
  - Configurable: 32KB, 4-way, 64B line size, 1-cycle hit latency.

### 3.3 MESI Cache Coherence Protocol & Snooping Crossbar
- **State Model**:
  - `M` (Modified): Dirty, exclusive to local core.
  - `E` (Exclusive): Clean, exclusive to local core.
  - `S` (Shared): Clean, present in multiple cores.
  - `I` (Invalid): Line not present or invalidated.
- **Coherence Interconnect Bus**:
  - Broadcast transactions: `BusRd`, `BusRdX`, `BusUpgr`, `BusWB`, `Flush`.
  - Snooping logic handles transitions on remote core reads/writes.
  - Local & Global Exclusive Monitors for ARM `LDREX` / `STREX` atomic synchronization.

---

## 4. Phased Implementation Roadmap

### Phase 1: Coherent Memory Hierarchy & Validation Demo (Walking Skeleton)
1. **Issue 01**: Configuration Schema, Parser & Statistics Reporter.
2. **Issue 02**: Parameterized Cache Subsystem (L1I, L1D, Shared L2, MSHR).
3. **Issue 03**: Multi-Core MESI Snooping Coherence Protocol & Crossbar Interconnect.
4. **Issue 04**: Phase 1 Cache Speedup Benchmark & MESI State Transition Demo.

### Phase 2: Front-End, Branch Prediction & Micro-Op Decode
5. **Issue 05**: Decoupled Fetch with GShare/Bimodal Predictor + BTB + RAS + Decode / Micro-op expansion (Store split to STA/STD).

### Phase 3: Out-of-Order Core, Dual-Issue LSU & Memory Disambiguation
6. **Issue 06**: Physical Register File (PRF), RAT & Free List with Branch Checkpointing.
7. **Issue 07**: Reorder Buffer (ROB) & Issue Queue (Wakeup/Select) with Bypass Matrix.
8. **Issue 08**: Dual-Issue Load/Store Unit (Dual AGU, LQ, SQ, Store-to-Load Forwarding, MOB Violation Squashing).
9. **Issue 09**: Integrated Multi-Core OoO Pipeline, CLI Integration (`--uarch-config`, `--uarch-stats`), and End-to-End Stress Verification.
