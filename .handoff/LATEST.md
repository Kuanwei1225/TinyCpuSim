# Quick Handoff: TinyArmSim

**Updated:** 2026-09-25

## 1. Project Goal
輕量級、模組化 C++17 ARM CPU 模擬器（50~55 道 16/32-bit Mixed Thumb/Thumb-2 指令），以 ISA Interpreter 為基準，預留未來 Cycle OoO (ROB/RS/Mispredict Flush) 與 JIT 擴充能力，支援 ELF32 靜態二進制載入與 GCC Assembly Self-Test (SVC #0 exit)。

## 2. Completed Milestones (ALL TICKETS RESOLVED & COMMITTED)
- [x] **Ticket 01**: CMake C++17 骨架、GoogleTest 整合、Smoke Test
- [x] **Ticket 02**: `ArchitecturalState`、64MB `MemoryBus`、`CpuFault` 例外階層
- [x] **Ticket 03**: `DecodedInstruction` 結構、16-bit Thumb 解碼、基本算術運算與 NZCV 旗標
- [x] **Ticket 04**: 擴充資料處理、桶型移位器 (`LSL`/`LSR`/`ASR`/`ROR`)、`MOVW`/`MOVT` 32-bit 立即數
- [x] **Ticket 05**: 條件判斷碼、分支跳躍 (`B`/`B<cond>`/`BL`/`BX`/`BLX`/`CBZ`/`CBNZ`)
- [x] **Ticket 06**: 記憶體讀寫 (`LDR`/`STR` 等)、堆疊操作 (`PUSH`/`POP`) 與區塊傳輸 (`LDM`/`STM`)
- [x] **Ticket 07**: 狀態暫存器 (`MRS`/`MSR`)、`NOP`、`SVC #0` 模擬結束攔截
- [x] **Ticket 08**: 雙模載入器 `Loader`（ELF32 靜態檔頭/Segment 解析載入 + Raw Binary 載入）
- [x] **Ticket 09**: ARM GCC 自我測試套件（`test_arithmetic.s`, `test_fibonacci.s`, `test_sort.s`）、預編譯二進制 Fixtures 與 CTest 53/53 測試 100% 通過
- [x] **CLI Runner**: `tinyarmsim <path.elf>` 直接載入並執行二進制檔案

## 3. Current Status & Git History
- **CTest**: 53 / 53 測試全部通過（100% Passed）
- **Git Commits**: 全部工單與代碼均已 Commit 進入 `main` 分支

## 4. Next Available Goals (Future Expansion)
- [ ] **Cycle-Accurate Out-of-Order (OoO) Engine**: 實作 Reorder Buffer (ROB)、Reservation Stations (RS)、Register Alias Table (RAT) 以及 Branch Mispredict Flush 機制。
- [ ] **JIT Compiler Engine**: 實作動態二進制轉譯器（Dynamic Binary Translation）。
- [ ] **周邊硬體與 MMIO**: 增加簡易 UART 輸出或計時器中斷支援。
