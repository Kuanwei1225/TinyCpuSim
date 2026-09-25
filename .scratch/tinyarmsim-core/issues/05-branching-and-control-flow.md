# 05: Branching, Control Flow, and Condition Codes

**What to build:** Full branching and conditional execution support: unconditional branches (`B`), conditional branches (`B<cond>` for all standard ARM condition codes), link branches (`BL`), exchange branches (`BX`, `BLX`), and compare-and-branch (`CBZ`, `CBNZ`).

**Blocked by:** 03-basic-data-processing

**Status:** resolved

- [x] Condition evaluator implements all 15 ARM condition codes (EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL)
- [x] `B` and `B<cond>` correctly compute relative target PC offsets and update `ArchitecturalState.PC`
- [x] `BL` and `BLX` save return address into LR (`R14`) and jump to target
- [x] `BX` switches PC to target address in register
- [x] `CBZ` and `CBNZ` test zero register condition and jump
- [x] Tests verify all condition branches taken vs not taken, loops, and function call/return sequences
