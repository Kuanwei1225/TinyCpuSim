# 06: Load/Store Single, Multiple, and Stack Operations

**What to build:** Memory transfer instructions: single-register load/store (`LDR`, `LDRB`, `LDRH`, `LDRSB`, `LDRSH`, `STR`, `STRB`, `STRH`), stack operations (`PUSH`, `POP`), and block transfer (`LDM`, `STM`) interacting with `MemoryBus`.

**Blocked by:** 04-extended-data-processing, 05-branching-and-control-flow

**Status:** resolved

- [x] Single transfer instructions support immediate and register offset addressing with optional writeback
- [x] Byte and halfword loads correctly apply zero-extension (`LDRB`, `LDRH`) or sign-extension (`LDRSB`, `LDRSH`)
- [x] `PUSH` and `POP` manipulate SP (`R13`) and transfer register lists to/from memory
- [x] `LDM` and `STM` transfer multiple registers with base register writeback
- [x] Tests verify stack operations, array traversals, and boundary load/store behaviors
