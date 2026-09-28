# gem5 Integration & Calibration Guide (Optional)

> **重要說明 / NOTE**:  
> **gem5 是可選（Optional）的輔助工具**。  
> TinyCpuSim 已內建完整的預先校準黃金參考數據（存放在 [`tests/golden/gem5/`](../tests/golden/gem5/) 與 [`tests/uarch/golden_counters.json`](../tests/uarch/golden_counters.json)），因此在日常開發、編譯模擬器、執行 156+ 單元測試、執行微基準測試（uBench）或進行微架構實驗時，**完全不需要安裝或編譯 gem5**。

---

## 1. 什麼時候需要 gem5？

只有在以下情境才需要實際執行 gem5：
1. 為**全新撰寫的 ARM Assembly Workload** 產生全新的 gem5 官方 cycle-accurate 基準統計檔案（`*.stats.txt`）。
2. 需要與最新的 gem5 O3 ARM 模擬器原始碼進行即時對齊驗證。

---

## 2. gem5 目錄配置位置與環境變數

TinyCpuSim 預設尋找 gem5 的路徑為：
- **預設目錄**：`/home/kw/workspace/gem5`
- **ARM 模擬器執行檔**：`/home/kw/workspace/gem5/build/ARM/gem5.opt`
- **Syscall Emulation 設定檔**：`/home/kw/workspace/gem5/configs/example/arm/starter_se.py`

### 自訂 gem5 路徑環境變數（可選）
若您的 gem5 位於其他目錄，可透過環境變數指定：
```bash
export GEM5_BIN="/path/to/gem5/build/ARM/gem5.opt"
export GEM5_CONFIG="/path/to/gem5/configs/example/arm/starter_se.py"
```

---

## 3. gem5 ARM 編譯完整流程 (Ubuntu / Linux)

### 步驟 1：安裝 gem5 建置相依套件
在主機終端機中執行：
```bash
sudo apt update && sudo apt install -y \
    build-essential \
    scons \
    python3-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libgoogle-perftools-dev \
    libboost-all-dev \
    pkg-config \
    m4 \
    zlib1g-dev \
    libhdf5-dev \
    pydot
```

### 步驟 2：平行編譯 gem5 ARM 最佳化版本
前往 gem5 專案目錄並使用 SCons 編譯 ARM 目標：
```bash
cd /home/kw/workspace/gem5

# 使用全核心平行編譯 (預計約 15 ~ 30 分鐘)
scons build/ARM/gem5.opt -j$(nproc)
```
> **檢查**：編譯完成後，確認 `/home/kw/workspace/gem5/build/ARM/gem5.opt` 已成功生成。

---

## 4. 產生與驗證 Golden Reference 統計資料

當 gem5 ARM 編譯完成後，可透過 TinyCpuSim 的自動化腳本進行 Golden Stats 生成與比較：

```bash
cd /home/kw/workspace/TinyCpuSim

# 1. 自動針對 tests/fixtures/*.elf 執行 gem5 並更新 tests/golden/gem5/*.stats.txt
python3 scripts/generate_gem5_golden.py

# 2. 執行端到端精確度相關性比對
./run.sh gem5
```
