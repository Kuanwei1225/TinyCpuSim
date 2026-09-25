# Issue 04: Stress Test Benchmark Pattern & MIPS Profiling

Status: ready-for-agent
Blocked by: 02
Type: task

## Description
Create `tests/asm/test_stress.s` containing heavy computational loops (matrix multiplication / checksum / prime sieve) executing 10M+ instructions to measure and verify MIPS simulation throughput.

## Acceptance Criteria
- `test_stress.s` compiles cleanly with `arm-none-eabi-gcc` into `test_stress.elf`.
- CLI simulator reports execution time and MIPS performance.
- Demonstrate significant MIPS performance improvement towards the ~80 MIPS target.
