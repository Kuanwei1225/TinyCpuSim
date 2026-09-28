#!/usr/bin/python3
"""
verify_coverage.py - TinyArmSim Automated Test & ISA Coverage Verification Tool

Runs all assembly test fixtures through the TinyArmSim simulator CLI, collects
per-instruction execution frequencies, and verifies that every implemented opcode
is executed at least 3 times.
"""

import os
import sys
import subprocess
import tempfile
import csv
from pathlib import Path

# All 45 implemented opcodes in TinyArmSim
ALL_OPCODES = [
    # Data Processing
    "MOV", "MVN", "MOVT", "MOVW", "ADD", "ADC", "SUB", "SBC", "RSB",
    "MUL", "MLA", "AND", "ORR", "EOR", "BIC", "CMP", "CMN", "TST",
    "TEQ", "ASR", "LSL", "LSR", "ROR",
    # Branching
    "B", "BL", "BX", "BLX", "CBZ", "CBNZ",
    # Memory Access
    "LDR", "LDRB", "LDRH", "LDRSB", "LDRSH",
    "STR", "STRB", "STRH", "LDM", "STM", "PUSH", "POP",
    # System
    "MRS", "MSR", "SVC", "NOP"
]

def find_simulator():
    candidates = [
        Path("build/tinyarmsim"),
        Path("./tinyarmsim"),
        Path("../build/tinyarmsim")
    ]
    for c in candidates:
        if c.is_file() and os.access(c, os.X_OK):
            return c.resolve()
    return None

def find_test_fixtures():
    fixtures_dir = Path("tests/fixtures")
    if not fixtures_dir.is_dir():
        return []
    return sorted(list(fixtures_dir.glob("*.elf")))

def run_test(sim_path, elf_path):
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tmp_csv:
        tmp_csv_path = tmp_csv.name

    cmd = [
        str(sim_path),
        "--elf", str(elf_path),
        "--coverage", tmp_csv_path,
        "--max-steps", "1000000000"
    ]
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Parse CSV counts
    opcode_counts = {}
    if os.path.exists(tmp_csv_path):
        try:
            with open(tmp_csv_path, "r", encoding="utf-8") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    name = row["Name"].strip()
                    count = int(row["Count"].strip())
                    opcode_counts[name] = count
        finally:
            os.remove(tmp_csv_path)

    passed = (result.returncode == 0)
    return {
        "elf": elf_path.name,
        "passed": passed,
        "returncode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "opcode_counts": opcode_counts
    }

import argparse
import concurrent.futures

def main():
    repo_root = Path(__file__).resolve().parent.parent
    os.chdir(repo_root)
    default_jobs = max(1, (os.cpu_count() or 4) // 2)

    parser = argparse.ArgumentParser(description="TinyArmSim Automated Test & ISA Coverage Verification Tool")
    parser.add_argument("-j", "--jobs", type=int, default=default_jobs, help=f"Parallel worker threads/processes (default: {default_jobs}, half of CPU cores)")
    args = parser.parse_args()

    sim_path = find_simulator()
    if not sim_path:
        print("Error: Simulator binary 'build/tinyarmsim' not found. Please run cmake --build build first.", file=sys.stderr)
        sys.exit(1)

    elf_files = find_test_fixtures()
    if not elf_files:
        print("Error: No test fixtures found in 'tests/fixtures/'.", file=sys.stderr)
        sys.exit(1)

    print("=" * 70)
    print("           TinyArmSim Test Suite & ISA Coverage Report           ")
    print("=" * 70)
    print(f"Simulator:        {sim_path}")
    print(f"Fixtures :        {len(elf_files)} patterns found")
    print(f"Parallel Workers: {args.jobs} (half of CPU cores)\n")

    total_opcode_counts = {op: 0 for op in ALL_OPCODES}
    all_tests_passed = True

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        test_results = list(executor.map(lambda elf: run_test(sim_path, elf), elf_files))

    for res in test_results:
        status_str = "PASSED" if res["passed"] else "FAILED"
        print(f"  [RUN] {res['elf']:<30} -> {status_str} (exit code {res['returncode']})")
        if not res["passed"]:
            all_tests_passed = False

        for op, count in res["opcode_counts"].items():
            if op in total_opcode_counts:
                total_opcode_counts[op] += count

    print("\n" + "=" * 70)
    print("                      Per-Opcode Coverage Table                      ")
    print("=" * 70)
    print(f"{'Opcode':<10} | {'Total Executions':<18} | {'Min Required':<14} | {'Status':<10}")
    print("-" * 70)

    covered_count = 0
    failing_opcodes = []

    for op in ALL_OPCODES:
        count = total_opcode_counts.get(op, 0)
        status = "PASS" if count >= 3 else "FAIL"
        if count >= 3:
            covered_count += 1
        else:
            failing_opcodes.append(op)
        print(f"{op:<10} | {count:<18} | {3:<14} | {status:<10}")

    coverage_pct = (covered_count / len(ALL_OPCODES)) * 100.0
    print("=" * 70)
    print(f"Coverage Summary: {covered_count}/{len(ALL_OPCODES)} opcodes meet >= 3 threshold ({coverage_pct:.1f}%)")
    print(f"Test Suite Status: {'ALL PASSED' if all_tests_passed else 'SOME TESTS FAILED'}")

    if failing_opcodes:
        print(f"\n[WARNING] The following opcodes did not reach count >= 3: {', '.join(failing_opcodes)}")
    
    overall_pass = all_tests_passed and (len(failing_opcodes) == 0)
    print("=" * 70)
    if overall_pass:
        print(">>> OVERALL VERIFICATION: SUCCESS (100% ISA Coverage >= 3 & All Tests Passed) <<<")
        print("=" * 70)
        sys.exit(0)
    else:
        print(">>> OVERALL VERIFICATION: FAILED <<<")
        print("=" * 70)
        sys.exit(1)

if __name__ == "__main__":
    main()
