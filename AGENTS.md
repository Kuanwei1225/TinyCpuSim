# TinyArmSim

## Agent Skills & Engineering Operating Guidelines

### 1. Skill Library & Automatic Discovery
All engineering and productivity skills are tracked directly in `.agents/skills/skills/` (originally credited to Matt Pocock with domain-specific extensions):
- **Core Engineering**: `.agents/skills/skills/engineering/` (`tdd`, `implement`, `codebase-design`, `code-review`, `diagnosing-bugs`, `domain-modeling`, `wayfinder`, `uarch-perf-correlation`).
- **Productivity & Review**: `.agents/skills/skills/productivity/` (`grill-me`, `grilling`, `handoff`, `teach`).

### 2. Microarchitectural Performance Correlation (`uarch-perf-correlation`)
- **Layer A Focus**: Performance calibration is conducted exclusively via isolated C++ subsystem microbenchmarks (`tests/uarch/*_ubench_test.cpp`). Full ELF binaries are NOT used for component invariant calibration due to pipeline noise.
- **Empirical Ground Truth**: Comparison baselines come strictly from real gem5 execution stats cached in [`tests/uarch/golden_counters.json`](file:///Users/kuanwei/workspace/TinySim/tests/uarch/golden_counters.json).
- **Comparator Tool**: Run [`scripts/report_ubench_perf.py`](file:///Users/kuanwei/workspace/TinySim/scripts/report_ubench_perf.py) with `--suite bp|core|lsu|rob|cache|all`.
- **Zero Drift Gate**: All microbenchmark invariants must achieve `<1%` delta against gem5 empirical references with 100% functional regression pass.

### 3. TDD & Codebase Design
- Follow strict TDD Red-Green-Refactor cycles and vertical slicing.
- Design deep modules with clean, agreed seams (`CompositeBranchPredictor`, `FetchUnit`, `LoadStoreQueue`, `ReorderBuffer`, `OOOCore`).
- Modern C++20 standard, strict RAII, and zero compiler warnings.

### 4. Project Tracking & Documentation
- Issues and specs: Local Markdown under `.scratch/` (see `docs/agents/issue-tracker.md`).
- Canonical 5-role triage labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`.
- Single-context repo layout: `CONTEXT.md` + `docs/adr/`.
