#!/usr/bin/env python3
"""
TinyCpuSim Microarchitectural Experiment & Parameter Exploration Utility
Allows tuning any hardware parameter on single or multiple ELF programs,
listing all tunable knobs, and generating automated side-by-side delta comparisons.
"""

import os
import sys
import argparse
import subprocess
import re
import tempfile
import glob

# -----------------------------------------------------------------------------
# Microarchitectural Parameters Catalog
# -----------------------------------------------------------------------------
PARAM_CATALOG = {
    "core": {
        "description": "Superscalar Out-of-Order Pipeline & Execution Core",
        "params": {
            "enable_ooo": {"default": "true", "type": "bool", "desc": "Out-of-Order engine (true=OoO Tomasulo/ROB, false=In-Order)"},
            "fetch_width": {"default": 4, "type": "int", "desc": "Instruction fetch width per cycle"},
            "decode_width": {"default": 4, "type": "int", "desc": "Instruction decode width per cycle"},
            "rename_width": {"default": 4, "type": "int", "desc": "Register renaming width per cycle"},
            "issue_width": {"default": 4, "type": "int", "desc": "Superscalar issue / dispatch width per cycle"},
            "commit_width": {"default": 4, "type": "int", "desc": "In-order retirement width per cycle"},
            "rob_size": {"default": 64, "type": "int", "desc": "Reorder Buffer (ROB) capacity entries"},
            "rs_size": {"default": 32, "type": "int", "desc": "Issue Queue / Reservation Station capacity"},
            "num_phys_regs": {"default": 128, "type": "int", "desc": "Physical Register File (PRF) total registers"},
        }
    },
    "branch_predictor": {
        "description": "Branch Prediction Unit (BPU)",
        "params": {
            "enabled": {"default": "true", "type": "bool", "desc": "Enable or disable dynamic branch prediction"},
            "type": {"default": "TAGE", "type": "choice [IDEAL, BIMODAL, GSHARE, TAGE]", "desc": "Branch direction predictor algorithm"},
            "table_size": {"default": 4096, "type": "int (power of 2)", "desc": "Branch history table (PHT) entries"},
            "btb_size": {"default": 4096, "type": "int (power of 2)", "desc": "Branch Target Buffer (BTB) cache entries"},
            "ras_size": {"default": 32, "type": "int", "desc": "Return Address Stack (RAS) depth"},
            "tage_tables": {"default": 4, "type": "int", "desc": "Number of geometric history TAGE tables"},
        }
    },
    "lsu": {
        "description": "Load/Store Unit & Memory Disambiguation",
        "params": {
            "lq_size": {"default": 16, "type": "int", "desc": "Load Queue capacity entries"},
            "sq_size": {"default": 16, "type": "int", "desc": "Store Queue capacity entries"},
            "enable_store_forwarding": {"default": "true", "type": "bool", "desc": "Enable 0-cycle Store-to-Load Queue forwarding"},
            "enable_speculative_load": {"default": "true", "type": "bool", "desc": "Enable speculative load execution before prior store addresses resolve"},
            "store_forward_latency": {"default": 1, "type": "int", "desc": "Forwarding bypass latency in cycles"},
        }
    },
    "cache_l1i": {
        "description": "L1 Instruction Cache",
        "params": {
            "enabled": {"default": "true", "type": "bool", "desc": "Enable L1 Instruction cache"},
            "size_bytes": {"default": 32768, "type": "bytes (e.g. 16KB, 32KB, 64KB)", "desc": "L1I total capacity in bytes"},
            "associativity": {"default": 4, "type": "int (e.g. 2, 4, 8)", "desc": "L1I N-way set associativity"},
            "line_size": {"default": 64, "type": "bytes", "desc": "L1I cache block size in bytes"},
            "hit_latency_cycles": {"default": 1, "type": "int", "desc": "L1I hit latency in clock cycles"},
            "mshr_entries": {"default": 8, "type": "int", "desc": "L1I non-blocking Miss Status Holding Registers"},
        }
    },
    "cache_l1d": {
        "description": "L1 Data Cache",
        "params": {
            "enabled": {"default": "true", "type": "bool", "desc": "Enable L1 Data cache"},
            "size_bytes": {"default": 32768, "type": "bytes (e.g. 16KB, 32KB, 64KB)", "desc": "L1D total capacity in bytes"},
            "associativity": {"default": 4, "type": "int (e.g. 2, 4, 8)", "desc": "L1D N-way set associativity"},
            "line_size": {"default": 64, "type": "bytes", "desc": "L1D cache block size in bytes"},
            "hit_latency_cycles": {"default": 1, "type": "int", "desc": "L1D hit latency in clock cycles"},
            "mshr_entries": {"default": 8, "type": "int", "desc": "L1D non-blocking Miss Status Holding Registers"},
        }
    },
    "cache_l2": {
        "description": "Shared L2 Cache Subsystem",
        "params": {
            "enabled": {"default": "true", "type": "bool", "desc": "Enable shared L2 cache"},
            "size_bytes": {"default": 524288, "type": "bytes (e.g. 256KB, 512KB, 1MB)", "desc": "Shared L2 total capacity in bytes"},
            "associativity": {"default": 8, "type": "int (e.g. 4, 8, 16)", "desc": "Shared L2 N-way set associativity"},
            "hit_latency_cycles": {"default": 10, "type": "int", "desc": "L2 hit latency in clock cycles"},
            "mshr_entries": {"default": 16, "type": "int", "desc": "L2 non-blocking Miss Status Holding Registers"},
        }
    },
    "system": {
        "description": "System Bus & Main Memory",
        "params": {
            "dram_latency_cycles": {"default": 80, "type": "int", "desc": "Off-chip DRAM access penalty in cycles"},
            "enable_mesi_coherence": {"default": "true", "type": "bool", "desc": "Enable multi-core MESI snooping protocol"},
        }
    }
}

ELF_CATALOG = {
    "test_fibonacci.elf": "Recursive & iterative Fibonacci calculation (heavy branching, deep call stack, ALU)",
    "test_sort.elf": "In-place Bubble Sort (data-dependent conditional branches, sequential memory writes)",
    "test_stress.elf": "10,000 iteration computational stress loop (maximum superscalar issue saturation)",
    "test_mem_stride.elf": "Non-contiguous memory access stride pattern (cache misses, spatial locality stress)",
    "test_store_forward.elf": "Immediate store-then-load to identical address (LSQ store-to-load forwarding bypass)",
    "test_raw_hazard.elf": "Chained RAW data dependencies (instruction serialization & pipeline dependency latency)",
    "test_branch_pred.elf": "Mixed pattern & alternating branches (BPU saturating counter & TAGE warmup)",
    "test_arithmetic.elf": "Pure arithmetic and logical instructions (straight-line multi-ALU throughput)",
    "test_isa_coverage.elf": "Broad ARMv7-M Thumb-2 instruction coverage (IT blocks, shifts, 32-bit ops)",
}

# Parameter Aliases for Quick CLI Use
PARAM_ALIASES = {
    "ooo": ("core", "enable_ooo"),
    "enable_ooo": ("core", "enable_ooo"),
    "in_order": ("core", "enable_ooo"),
    "width": ("core", "issue_width"),
    "fetch_width": ("core", "fetch_width"),
    "decode_width": ("core", "decode_width"),
    "issue_width": ("core", "issue_width"),
    "commit_width": ("core", "commit_width"),
    "rob": ("core", "rob_size"),
    "rob_size": ("core", "rob_size"),
    "iq": ("core", "rs_size"),
    "iq_size": ("core", "rs_size"),
    "rs": ("core", "rs_size"),
    "rs_size": ("core", "rs_size"),
    "prf": ("core", "num_phys_regs"),
    "num_phys_regs": ("core", "num_phys_regs"),
    "bp": ("branch_predictor", "type"),
    "bp_type": ("branch_predictor", "type"),
    "bpu": ("branch_predictor", "type"),
    "bp_enabled": ("branch_predictor", "enabled"),
    "branch_enabled": ("branch_predictor", "enabled"),
    "bpu_enabled": ("branch_predictor", "enabled"),
    "btb": ("branch_predictor", "btb_size"),
    "btb_size": ("branch_predictor", "btb_size"),
    "ras": ("branch_predictor", "ras_size"),
    "ras_size": ("branch_predictor", "ras_size"),
    "lq": ("lsu", "lq_size"),
    "lq_size": ("lsu", "lq_size"),
    "sq": ("lsu", "sq_size"),
    "sq_size": ("lsu", "sq_size"),
    "store_forward": ("lsu", "enable_store_forwarding"),
    "l1i_size": ("cache_l1i", "size_bytes"),
    "l1d_size": ("cache_l1d", "size_bytes"),
    "l1i_assoc": ("cache_l1i", "associativity"),
    "l1d_assoc": ("cache_l1d", "associativity"),
    "l1d_latency": ("cache_l1d", "hit_latency_cycles"),
    "l2_size": ("cache_l2", "size_bytes"),
    "l2_assoc": ("cache_l2", "associativity"),
    "l2_latency": ("cache_l2", "hit_latency_cycles"),
    "dram_latency": ("system", "dram_latency_cycles"),
}

def parse_byte_size(val_str):
    val_str = str(val_str).strip().upper()
    if val_str.endswith("KB") or val_str.endswith("K"):
        num = int(re.sub(r'[^0-9]', '', val_str))
        return num * 1024
    elif val_str.endswith("MB") or val_str.endswith("M"):
        num = int(re.sub(r'[^0-9]', '', val_str))
        return num * 1024 * 1024
    elif val_str.endswith("B"):
        return int(re.sub(r'[^0-9]', '', val_str))
    return int(val_str)

def list_parameters():
    print("=" * 80)
    print("      TinyCpuSim Tunable Microarchitectural Hardware Parameters Catalog      ")
    print("=" * 80)
    for sec_name, sec_data in PARAM_CATALOG.items():
        print(f"\n[{sec_name}] - {sec_data['description']}")
        print("-" * 80)
        print(f"  {'Parameter':<24} | {'Default':<12} | {'Type / Range':<22} | {'Description'}")
        print("  " + "-" * 76)
        for p_name, p_info in sec_data["params"].items():
            print(f"  {p_name:<24} | {str(p_info['default']):<12} | {str(p_info['type']):<22} | {p_info['desc']}")
    print("\n" + "=" * 80)
    print("CLI Shortcut Aliases (use with --set <alias>=<value>):")
    print("  e.g. --set rob=128 --set width=8 --set l1d_size=64KB --set bp=TAGE")
    print("=" * 80)

def list_elfs():
    print("=" * 80)
    print("              Available Built-in Benchmark ELFs in TinyCpuSim                ")
    print("=" * 80)
    print(f"{'ELF Target':<25} | {'Workload Characteristic & Pattern'}")
    print("-" * 80)
    for elf_name, desc in ELF_CATALOG.items():
        print(f"{elf_name:<25} | {desc}")
    print("=" * 80)

def parse_perf_output(output_text):
    stats = {}
    m = re.search(r'Simulated Total Cycles:\s+(\d+)', output_text)
    if m: stats['cycles'] = int(m.group(1))
    
    m = re.search(r'Total Committed Insts:\s+(\d+)', output_text)
    if m: stats['insts'] = int(m.group(1))

    m = re.search(r'Aggregate Throughput \(IPC\):\s*([\d\.]+)', output_text)
    if m: stats['ipc'] = float(m.group(1))

    m = re.search(r'Branch Predictions:\s+(\d+)\s+\(Accuracy:\s*([\d\.]+)%\)', output_text)
    if m:
        stats['branch_preds'] = int(m.group(1))
        stats['branch_acc'] = float(m.group(2))

    m = re.search(r'Branch Mispredicts:\s+(\d+)', output_text)
    if m: stats['branch_mispredicts'] = int(m.group(1))

    m = re.search(r'L1I Cache Hit Rate:\s*([\d\.]+)%', output_text)
    if m: stats['l1i_hit_rate'] = float(m.group(1))

    m = re.search(r'L1D Cache Hit Rate:\s*([\d\.]+)%', output_text)
    if m: stats['l1d_hit_rate'] = float(m.group(1))

    m = re.search(r'Store-to-Load Forwards:\s+(\d+)', output_text)
    if m: stats['store_forwards'] = int(m.group(1))

    m = re.search(r'Mem Order Violations:\s+(\d+)', output_text)
    if m: stats['mem_order_violations'] = int(m.group(1))

    m = re.search(r'ROB Full Stalls:\s+(\d+)', output_text)
    if m: stats['rob_stalls'] = int(m.group(1))

    m = re.search(r'RS/IQ Full Stalls:\s+(\d+)', output_text)
    if m: stats['rs_stalls'] = int(m.group(1))

    m = re.search(r'Frontend Bound:\s*([\d\.]+)%', output_text)
    if m: stats['frontend_bound'] = float(m.group(1))

    m = re.search(r'Bad Speculation:\s*([\d\.]+)%', output_text)
    if m: stats['bad_spec'] = float(m.group(1))

    m = re.search(r'Backend Bound:\s*([\d\.]+)%', output_text)
    if m: stats['backend_bound'] = float(m.group(1))

    m = re.search(r'Retiring:\s*([\d\.]+)%', output_text)
    if m: stats['retiring'] = float(m.group(1))

    return stats

def generate_config_with_overrides(base_path, overrides):
    content = ""
    if base_path and os.path.exists(base_path):
        with open(base_path, 'r') as f:
            content = f.read()
    else:
        content = """[core]
fetch_width = 4
decode_width = 4
rename_width = 4
issue_width = 4
commit_width = 4
rob_size = 64
rs_size = 32
num_phys_regs = 128

[branch_predictor]
enabled = true
type = TAGE
table_size = 4096
btb_size = 4096
ras_size = 32
tage_tables = 4

[lsu]
lq_size = 16
sq_size = 16
enable_store_forwarding = true
enable_speculative_load = true

[cache_l1i]
size_bytes = 32768
associativity = 4
line_size = 64
hit_latency_cycles = 1

[cache_l1d]
size_bytes = 32768
associativity = 4
line_size = 64
hit_latency_cycles = 1

[cache_l2]
size_bytes = 524288
associativity = 8
hit_latency_cycles = 10
"""
    lines = content.splitlines()
    for sec, key, val in overrides:
        found = False
        new_lines = []
        in_section = False
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('[') and stripped.endswith(']'):
                in_section = (stripped[1:-1] == sec)
            if in_section and (stripped.startswith(f"{key} =") or stripped.startswith(f"{key}=")):
                new_lines.append(f"{key} = {val}")
                found = True
            else:
                new_lines.append(line)
        if not found:
            new_lines.append(f"\n[{sec}]\n{key} = {val}")
        lines = new_lines

    tmp = tempfile.NamedTemporaryFile(mode='w', suffix='.cfg', delete=False)
    tmp.write("\n".join(lines))
    tmp.close()
    return tmp.name

def run_sim(sim_bin, elf_file, cfg_file):
    cmd = [sim_bin, "--uarch", "--uarch-config", cfg_file, "--all-perf", elf_file]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return parse_perf_output(res.stdout), res.stdout

def print_comparison_table(elf_name, changed_params, base_stats, exp_stats):
    print("\n" + "=" * 78)
    print(f"       Experiment Comparison Report: [{os.path.basename(elf_name)}]")
    print("=" * 78)
    
    if changed_params:
        print("  Active Hardware Parameter Modifications:")
        for sec, key, old_v, new_v in changed_params:
            print(f"    - [{sec}] {key}: {old_v}  -->  \033[1;32m{new_v}\033[0m")
        print("-" * 78)

    print(f"  {'Hardware Metric':<30} | {'Baseline':<12} | {'Experiment':<12} | {'Delta (%)':<12}")
    print("  " + "-" * 74)

    metrics = [
        ('cycles', 'Simulated Total Cycles', 'lower_is_better', ''),
        ('insts', 'Committed Instructions', 'neutral', ''),
        ('ipc', 'Throughput (IPC)', 'higher_is_better', ''),
        ('branch_acc', 'Branch Predictor Accuracy', 'higher_is_better', '%'),
        ('branch_mispredicts', 'Branch Mispredict Flushes', 'lower_is_better', ''),
        ('l1i_hit_rate', 'L1I Cache Hit Rate', 'higher_is_better', '%'),
        ('l1d_hit_rate', 'L1D Cache Hit Rate', 'higher_is_better', '%'),
        ('store_forwards', 'Store-to-Load Forwards', 'neutral', ''),
        ('rob_stalls', 'ROB Full Stalls', 'lower_is_better', ' cycles'),
        ('rs_stalls', 'Issue Queue Full Stalls', 'lower_is_better', ' cycles'),
        ('frontend_bound', 'TMAM Frontend Bound', 'lower_is_better', '%'),
        ('bad_spec', 'TMAM Bad Speculation', 'lower_is_better', '%'),
        ('backend_bound', 'TMAM Backend Bound', 'lower_is_better', '%'),
        ('retiring', 'TMAM Retiring', 'higher_is_better', '%'),
    ]

    for key, label, direction, unit in metrics:
        v_base = base_stats.get(key, 'N/A')
        v_exp = exp_stats.get(key, 'N/A')
        
        delta_str = "---"
        if isinstance(v_base, (int, float)) and isinstance(v_exp, (int, float)) and v_base > 0:
            delta = ((v_exp - v_base) / float(v_base)) * 100.0
            if abs(delta) < 0.001:
                delta_str = "0.0%"
            else:
                sign = "+" if delta > 0 else ""
                delta_str = f"{sign}{delta:.2f}%"

        str_base = f"{v_base}{unit}" if v_base != 'N/A' else 'N/A'
        str_exp = f"{v_exp}{unit}" if v_exp != 'N/A' else 'N/A'

        print(f"  {label:<30} | {str_base:<12} | {str_exp:<12} | {delta_str:<12}")

    print("=" * 78)

def main():
    parser = argparse.ArgumentParser(
        description="TinyCpuSim Microarchitectural Experiment & Parameter Tuning Tool",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("--elf", "-e", help="Target ELF binary name or comma-separated list (e.g. test_fibonacci.elf or test_fibonacci,test_sort)")
    parser.add_argument("--set", "-s", action="append", help="Override hardware parameter: --set <param>=<value>\n(e.g. --set rob=128 --set width=8 --set l1d_size=64KB --set bp=TAGE)")
    parser.add_argument("--config", "-c", help="Path to custom baseline configuration file")
    parser.add_argument("--list-params", "-lp", action="store_true", help="List all tunable microarchitecture parameters with descriptions")
    parser.add_argument("--list-elfs", "-le", action="store_true", help="List all available built-in test benchmark ELF files")
    parser.add_argument("--output", "-o", help="Optional report file path to save output")
    args = parser.parse_args()

    if args.list_params:
        list_parameters()
        sys.exit(0)

    if args.list_elfs:
        list_elfs()
        sys.exit(0)

    if not args.elf:
        print("Error: No target ELF specified. Use --elf <name> or --list-elfs to see available workloads.")
        print("Run 'python3 scripts/experiment.py --help' or 'python3 scripts/experiment.py --list-params' for options.")
        sys.exit(1)

    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sim_bin = os.path.join(root_dir, "build", "tinycpusim")
    fixtures_dir = os.path.join(root_dir, "tests", "fixtures")
    reports_dir = os.path.join(root_dir, "reports")
    os.makedirs(reports_dir, exist_ok=True)

    default_base = os.path.join(root_dir, "configs", "default", "default.cfg")
    if not os.path.exists(default_base):
        default_base = os.path.join(root_dir, "configs", "current.cfg")
    base_cfg = args.config if args.config else default_base

    if not os.path.exists(sim_bin):
        print(f"Simulator binary not found. Running build first...")
        subprocess.run([os.path.join(root_dir, "scripts", "01_build.sh")], check=True)

    # Resolve target ELFs
    elf_targets = []
    for item in args.elf.split(','):
        item = item.strip()
        if not item: continue
        if not item.endswith('.elf'): item += '.elf'
        full_path = item if os.path.isabs(item) else os.path.join(fixtures_dir, os.path.basename(item))
        if not os.path.exists(full_path):
            print(f"Error: ELF fixture not found: {full_path}")
            print("Run 'python3 scripts/experiment.py --list-elfs' to view available ELFs.")
            sys.exit(1)
        elf_targets.append(full_path)

    # Process parameter overrides
    overrides = []
    changed_params = []
    if args.set:
        for item in args.set:
            if '=' not in item:
                print(f"Warning: Invalid override format: '{item}'. Expected format is 'key=value'.")
                continue
            k, v = item.split('=', 1)
            k = k.strip().lower()
            v = v.strip()
            
            # Resolve alias or direct name
            sec = "core"
            actual_key = k
            if k in PARAM_ALIASES:
                sec, actual_key = PARAM_ALIASES[k]
            else:
                # Find section from catalog
                for s_name, s_info in PARAM_CATALOG.items():
                    if k in s_info["params"]:
                        sec = s_name
                        actual_key = k
                        break

            # Handle byte sizes (e.g. 64KB -> 65536)
            if actual_key in ["size_bytes", "l1i_size", "l1d_size", "l2_size"] and any(c.isalpha() for c in v):
                v_num = parse_byte_size(v)
                v = str(v_num)

            # Special case for width: expand to fetch, decode, issue, commit
            if actual_key == "issue_width" and k in ["width", "issue_width"]:
                overrides.append(("core", "fetch_width", v))
                overrides.append(("core", "decode_width", v))
                overrides.append(("core", "issue_width", v))
                overrides.append(("core", "commit_width", v))
                changed_params.append(("core", "pipeline_width (fetch/decode/issue/commit)", "4", v))
            else:
                overrides.append((sec, actual_key, v))
                changed_params.append((sec, actual_key, "default", v))

    # Generate experiment config
    exp_cfg = generate_config_with_overrides(base_cfg, overrides)

    try:
        for elf_path in elf_targets:
            # 1. Run Baseline
            base_stats, _ = run_sim(sim_bin, elf_path, base_cfg)
            # 2. Run Experiment
            exp_stats, full_log = run_sim(sim_bin, elf_path, exp_cfg)
            # 3. Print Comparison
            print_comparison_table(elf_path, changed_params, base_stats, exp_stats)
            
            out_file = args.output
            if not out_file:
                import datetime
                ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                base_name = os.path.basename(elf_path).replace('.elf', '')
                out_file = os.path.join(reports_dir, f"exp_{base_name}_{ts}.txt")

            with open(out_file, 'a') as f_out:
                f_out.write(f"\n=== Experiment: {os.path.basename(elf_path)} ===\n")
                f_out.write(f"Baseline Config: {base_cfg}\n")
                f_out.write(full_log)
            print(f"  Full performance log written to: \033[1;36m{out_file}\033[0m")

    finally:
        if os.path.exists(exp_cfg):
            os.remove(exp_cfg)

if __name__ == '__main__':
    main()
