# Issue 01: Configuration Schema, Parser & Statistics Reporter

Status: ready-for-agent
Type: task
Blocked by: none

## Context & Goal
Provide the fundamental configuration data model and metrics tracking system for the uArch engine:
1. `UArchConfig`: Struct defining core counts, pipeline widths, ROB/RS/LQ/SQ queue capacities, branch predictor options, cache parameters (L1I, L1D, Shared L2), and memory latencies.
2. `ConfigParser`: Flexible parser loading JSON / Key-Value configuration files with safe defaults.
3. `UArchStats`: Comprehensive hardware counters (cycles, committed instructions, IPC, stalls by category, branch accuracy, cache hit/miss statistics per level, core-by-core metrics).
4. `StatsExporter`: Formats and dumps statistics to JSON and structured text files.

## Acceptance Criteria
- Full deserialization of all uArch parameters with validation checks (e.g. power-of-two cache sizes, non-zero widths).
- Unit tests verifying config parsing, fallback to defaults on omitted fields, error handling on invalid syntax, and JSON/text stats export formatting.
