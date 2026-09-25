# Issue 06 (LSU): Dual-Issue Load/Store Unit & Speculative Memory Disambiguation

Status: ready-for-agent
Type: task
Blocked by: 02, 05

## Context & Goal
Implement a high-performance Dual-Issue Load/Store Unit (LSU) with decoupled Address Generation Units (AGUs), Store Queue (SQ), Load Queue (LQ), and speculative Memory Order Buffer (MOB) disambiguation:

1. **Decoupled Store Micro-Ops**:
   - `STR / STRB / STRH` decoded into:
     - `uop_STA`: Store Address Generation (calculates base + offset, writes address to SQ entry).
     - `uop_STD`: Store Data Generation (reads source register, writes payload to SQ entry).
   - Allows store addresses to resolve early and unblock younger loads even if store data is pending.

2. **Dual-Issue Memory Execution Ports**:
   - **Port 2 (Load AGU)**: Dedicated Load Address generation (`uop_LDA`) and L1D read port.
   - **Port 3 (Load/Store AGU)**: Secondary AGU capable of executing either a 2nd concurrent Load (`uop_LDA`) or a Store Address (`uop_STA`).
   - **Port 4 (Store Data)**: Writes `uop_STD` data payload into the SQ buffer.
   - Sustains **2 Loads/cycle** OR **1 Load + 1 Store/cycle**.

3. **Store-to-Load Forwarding Engine**:
   - When a load executes, searches older uncommitted SQ entries:
     - Exact match: Forwards data directly in 1 cycle.
     - Partial / byte-mask match: Assembles forwarded bytes or stalls appropriately.
     - No match: Issues non-blocking L1 D-Cache read (via MSHR).

4. **Speculative Memory Disambiguation & MOB**:
   - Younger loads execute speculatively even when older store addresses are unresolved.
   - When an older `uop_STA` later resolves its address, it snoops younger executed loads in LQ.
   - If an address aliasing True-RAW hazard is detected: Triggers **Memory Order Violation Squash** (recovers pipeline to the checkpointed state).

## Acceptance Criteria
- Unit tests verifying:
  - Dual-issue concurrency: Executing 2 loads in the same cycle.
  - Store micro-op decoupling: `uop_STA` unblocking loads before `uop_STD` arrives.
  - Exact address store-to-load forwarding without cache hit delay.
  - Memory order violation detection and recovery when older store changes aliased address.
