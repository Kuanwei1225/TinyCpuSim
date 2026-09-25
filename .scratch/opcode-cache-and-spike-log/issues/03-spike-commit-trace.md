# Issue 03: Spike-Style Commit Trace with Register & Memory Diff

Status: ready-for-agent
Blocked by: 01, 02
Type: task

## Description
Implement a state diff tracker that captures per-instruction modifications:
- Register writes: `r<d> 0x<value>`
- Memory accesses: `mem[0x<addr>] <= 0x<value> (<size>B)` (writes) and `mem[0x<addr>]` (reads)
- CPSR / NZCV flags: `NZCV=[<N><Z><C><V>]`
- Integrate with `--log` / `--log-file` to output standard single-line Spike-style commit logs.

## Acceptance Criteria
- Trace format matches:
  `core 0: 0x00010014 (0x210a) movs r1, #10 | r1 0x0000000a | NZCV=[0000]`
- Zero-cost when `--log` is disabled.
- Unit and integration tests for trace output formatting.
