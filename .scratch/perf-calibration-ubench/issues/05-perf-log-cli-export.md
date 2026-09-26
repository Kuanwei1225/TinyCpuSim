# Issue 05: Perf Log, CLI Integration and Multi-Format Export

Status: resolved

## Objective
Provide command-line options (`--perf-log <file>`, `-p <file>`, `--topdown [file]`, `--topdown-format <fmt>`, `--uarch-stats <file>`) and export structured performance logs and Top-Down reports.

## Features Implemented
1. `--perf-log <file>` / `-p <file>`: Exports complete hardware performance counter breakdown and uArch stats to specified log file.
2. `--topdown [file] --topdown-format <text|json|csv>`: Exports Top-Down microarchitectural bottleneck report.
3. `--coverage <file>`: Exports instruction opcode coverage report to CSV.
4. Dynamic ROI isolation via m5ops (0x50 reset, 0x51 dump, 0x52 exit).

## Verification
- CLI execution with `--perf-log /tmp/perf_test.log` succeeded with full structured output.
- `ctest`: 150 / 150 passed (100%).
