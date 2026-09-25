# 04: Extended Data Processing, Barrel Shifter, and Wide Immediates

**What to build:** The complete suite of arithmetic and bitwise data processing instructions (`ADC`, `SBC`, `RSB`, `MUL`, `MLA`, `AND`, `ORR`, `EOR`, `BIC`, `CMN`, `TST`, `TEQ`), barrel shifter shift modes (`LSL`, `LSR`, `ASR`, `ROR`), and wide immediate support (`MOVW`, `MOVT`).

**Blocked by:** 03-basic-data-processing

**Status:** resolved

- [x] Barrel shifter supports logical left, logical right, arithmetic right, and rotate right operations by immediate and register values
- [x] Bitwise and multi-cycle arithmetic instructions (`AND`, `ORR`, `EOR`, `BIC`, `MUL`, `MLA`, `ADC`, `SBC`, `RSB`, `CMN`, `TST`, `TEQ`) fully implemented in Decoder and Interpreter
- [x] `MOVW` loads 16-bit lower halfword and `MOVT` loads 16-bit upper halfword into destination register
- [x] Comprehensive unit tests cover full arithmetic/logical matrix, shifts, and 32-bit immediate loading
