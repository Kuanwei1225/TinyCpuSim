#!/usr/bin/env python3
"""
Generate gem5 Golden Reference Stats for TinyArmSim Benchmarks
Runs gem5 once per test fixture and stores the golden stats in tests/golden/gem5/
"""

import os
import sys
import glob
import subprocess
import shutil

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    gem5_bin = "/Users/kuanwei/workspace/gem5/build/ARM/gem5.opt"
    gem5_config = "/Users/kuanwei/workspace/gem5/configs/example/arm/starter_se.py"
    fixtures_dir = os.path.join(root_dir, "tests", "fixtures")
    golden_dir = os.path.join(root_dir, "tests", "golden", "gem5")
    
    if not os.path.exists(gem5_bin):
        print(f"Error: gem5 binary not found at {gem5_bin}")
        sys.exit(1)
        
    os.makedirs(golden_dir, exist_ok=True)
    
    elf_files = sorted(glob.glob(os.path.join(fixtures_dir, "*.elf")))
    if not elf_files:
        print(f"No ELF files found in {fixtures_dir}. Please build fixtures first.")
        sys.exit(1)
        
    print(f"Generating gem5 golden stats for {len(elf_files)} fixtures...")
    print(f"Output directory: {golden_dir}")
    print("-" * 65)
    
    for elf in elf_files:
        case_name = os.path.splitext(os.path.basename(elf))[0]
        outdir = f"/tmp/gem5_golden_{case_name}"
        if os.path.exists(outdir):
            shutil.rmtree(outdir)
            
        cmd = [
            gem5_bin,
            f"--outdir={outdir}",
            gem5_config,
            "--cpu=o3",
            elf
        ]
        
        print(f"Running gem5 on [{case_name}]...", end="", flush=True)
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if res.returncode != 0:
            print(f" FAILED (returncode {res.returncode})")
            print(res.stderr[:300])
            continue
            
        stats_file = os.path.join(outdir, "stats.txt")
        if os.path.exists(stats_file):
            dest_file = os.path.join(golden_dir, f"{case_name}.stats.txt")
            shutil.copyfile(stats_file, dest_file)
            print(f" DONE -> {os.path.basename(dest_file)}")
        else:
            print(" FAILED (stats.txt not generated)")
            
    print("-" * 65)
    print("All golden reference statistics generated successfully.")

if __name__ == "__main__":
    main()
