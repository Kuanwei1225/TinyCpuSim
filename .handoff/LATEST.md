# Quick Handoff: TinyArmSim

**Updated:** 2026-09-24

## 1. Project Goal
輕量級、模組化 C++17 ARM CPU 模擬器（50~55 道 16/32-bit Mixed Thumb/Thumb-2 指令），以 ISA Interpreter 為基準，預留未來 Cycle OoO (ROB/RS/Mispredict Flush) 與 JIT 擴充能力，支援 ELF32 靜態二進制載入與 GCC Assembly Self-Test (SVC #0 exit)。

## 2. Completed Milestones
- [x] 專案設定與規範初始化（`AGENTS.md`, `docs/agents/`）
- [x] 領域模型與架構決策（`CONTEXT.md`, `docs/adr/0001~0003`）
- [x] 完整規格書產出（`.scratch/tinyarmsim-core/spec.md`）
- [x] 工單拆解（`.scratch/tinyarmsim-core/issues/01~09`）
- [x] **Ticket 01 完成**：CMake C++17 骨架、GoogleTest 整合、Smoke Test 通過

## 3. Current Frontier (Next Step)
- **當前可執行工單**：`02-architectural-state-and-memory-bus`（ArchitecturalState, 64MB MemoryBus, CpuFault 例外階層）
- **工單路徑**：`.scratch/tinyarmsim-core/issues/02-architectural-state-and-memory-bus.md`

## 4. Prompt for Next Model (一鍵複製給新模型)
```
請閱讀 CONTEXT.md、docs/adr/ 與 .scratch/tinyarmsim-core/spec.md，接著開始執行工單 .scratch/tinyarmsim-core/issues/02-architectural-state-and-memory-bus.md。實作時請遵循 /tdd 原則並確保 CTest 通過。
```
