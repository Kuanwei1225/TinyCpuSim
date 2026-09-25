# Feature Spec: Opcode Cache, Direct Dispatch & Spike-Style Commit Log

## 1. Motivation & Objectives
1. **Opcode-Keyed Cache (GDB & Self-Modifying Code Safe)**:
   - Key cache strictly by raw opcode/instruction word, NOT by PC.
   - 16-bit Thumb: Direct 64K lookup table (`65,536` entries).
   - 32-bit Thumb: Hash cache keyed by 32-bit raw instruction word.
   - Safe against GDB software breakpoints (`BKPT` / instruction replacement) and code modification.
2. **Direct Handler Dispatch**:
   - Associate each decoded opcode directly with a dedicated execution handler / function pointer to bypass large `switch-case` branches and achieve target performance (~80 MIPS).
3. **Pre-Generated Disassembly**:
   - Build disassembly string at decode time and store inside the cached object, eliminating runtime string formatting overhead.
4. **Spike-Style Commit Log**:
   - Track per-instruction register writes (`r<d> 0x<val>`), memory accesses (`mem[0x<addr>] <= 0x<val>`), and `NZCV` flags.
   - Output clean single-line format:
     `core 0: 0x00010014 (0x210a) movs r1, #10 | r1 0x0000000a | NZCV=[0000]`
5. **Stress Test Benchmark**:
   - Create `tests/asm/test_stress.s` executing millions of instructions to benchmark and measure MIPS throughput.
