# Wayfinding Map: Opcode Cache, Direct Dispatch & Spike Log

## Decisions & Architecture
- **Opcode-Keyed Cache**: Direct 64K lookup table for 16-bit Thumb (`0x0000`–`0xFFFF`) and Hash Map for 32-bit Thumb, keyed strictly by raw opcode bits to remain completely safe from GDB breakpoints (`BKPT`) and self-modifying code.
- **Direct Dispatch**: Function pointer / handler pointer in `DecodedInstruction` for $O(1)$ direct jump execution.
- **Pre-Generated Disassembly**: Build disassembly string at decode time and store inside the cached object.
- **Zero-Cost Trace**: Trace state collector only invoked when `--log` is enabled.
- **Stress Benchmark**: `tests/asm/test_stress.s` for measuring MIPS up to Spike-comparable throughput.

## Issues
1. [01-disassembler-and-object.md](file:///Users/kuanwei/workspace/TinySim/.scratch/opcode-cache-and-spike-log/issues/01-disassembler-and-object.md) (Status: ready-for-agent)
2. [02-opcode-cache-dispatch.md](file:///Users/kuanwei/workspace/TinySim/.scratch/opcode-cache-and-spike-log/issues/02-opcode-cache-dispatch.md) (Status: ready-for-agent, Blocked by: 01)
3. [03-spike-commit-trace.md](file:///Users/kuanwei/workspace/TinySim/.scratch/opcode-cache-and-spike-log/issues/03-spike-commit-trace.md) (Status: ready-for-agent, Blocked by: 01, 02)
4. [04-stress-benchmark.md](file:///Users/kuanwei/workspace/TinySim/.scratch/opcode-cache-and-spike-log/issues/04-stress-benchmark.md) (Status: ready-for-agent, Blocked by: 02)
