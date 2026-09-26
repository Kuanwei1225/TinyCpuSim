#!/usr/bin/env bash
#!/usr/bin/env python3
"""
TinyCpuSim Comprehensive Configuration Lifecycle Manager (scripts/config.py)
Manages configs/current.cfg, presets in configs/default/, snapshots in configs/save/,
and parameter sweep sets in configs/sweep/, with validation and human-readable KB/MB conversion.
"""

import os
import sys
import argparse
import configparser
import shutil
import re
import glob
import subprocess

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONFIGS_DIR = os.path.join(PROJECT_ROOT, "configs")
DEFAULT_DIR = os.path.join(CONFIGS_DIR, "default")
SAVE_DIR = os.path.join(CONFIGS_DIR, "save")
SWEEP_DIR = os.path.join(CONFIGS_DIR, "sweep")
CURRENT_CFG = os.path.join(CONFIGS_DIR, "current.cfg")
FIXTURES_DIR = os.path.join(PROJECT_ROOT, "tests", "fixtures")

# Parameter aliases for convenient CLI use
ALIASES = {
    "elf": ("simulation", "elf_path"),
    "elf_path": ("simulation", "elf_path"),
    "mode": ("simulation", "mode"),
    "all_perf": ("simulation", "all_perf"),
    "verbose": ("simulation", "all_perf"),
    "topdown": ("simulation", "enable_topdown"),
    "enable_topdown": ("simulation", "enable_topdown"),
    "max_steps": ("simulation", "max_steps"),
    
    "cores": ("system", "num_cores"),
    "num_cores": ("system", "num_cores"),
    "dram_latency": ("system", "dram_latency_cycles"),
    "mesi": ("system", "enable_mesi_coherence"),
    
    "width": ("core", "issue_width"),
    "fetch_width": ("core", "fetch_width"),
    "decode_width": ("core", "decode_width"),
    "rename_width": ("core", "rename_width"),
    "issue_width": ("core", "issue_width"),
    "commit_width": ("core", "commit_width"),
    "rob": ("core", "rob_size"),
    "rob_size": ("core", "rob_size"),
    "iq": ("core", "rs_size"),
    "rs": ("core", "rs_size"),
    "rs_size": ("core", "rs_size"),
    "prf": ("core", "num_phys_regs"),
    "num_phys_regs": ("core", "num_phys_regs"),
    
    "bp": ("branch_predictor", "type"),
    "bpu": ("branch_predictor", "type"),
    "bp_type": ("branch_predictor", "type"),
    "btb": ("branch_predictor", "btb_size"),
    "btb_size": ("branch_predictor", "btb_size"),
    "ras": ("branch_predictor", "ras_size"),
    "ras_size": ("branch_predictor", "ras_size"),
    
    "lq": ("lsu", "lq_size"),
    "sq": ("lsu", "sq_size"),
    "store_forward": ("lsu", "enable_store_forwarding"),
    
    "l1i_size": ("cache_l1i", "size_bytes"),
    "l1i_assoc": ("cache_l1i", "associativity"),
    "l1d_size": ("cache_l1d", "size_bytes"),
    "l1d_assoc": ("cache_l1d", "associativity"),
    "l1d_latency": ("cache_l1d", "hit_latency_cycles"),
    "l2_size": ("cache_l2", "size_bytes"),
    "l2_assoc": ("cache_l2", "associativity"),
    "l2_latency": ("cache_l2", "hit_latency_cycles"),
}

def ensure_dirs():
    os.makedirs(DEFAULT_DIR, exist_ok=True)
    os.makedirs(SAVE_DIR, exist_ok=True)
    os.makedirs(SWEEP_DIR, exist_ok=True)
    if not os.path.exists(CURRENT_CFG):
        default_src = os.path.join(DEFAULT_DIR, "default.cfg")
        if os.path.exists(default_src):
            shutil.copyfile(default_src, CURRENT_CFG)

def parse_byte_size(val_str):
    """Convert human-readable units like '32KB', '512KB', '1MB' to integer bytes."""
    val_str = str(val_str).strip().upper()
    if val_str.endswith("KB") or val_str.endswith("K"):
        num = int(re.sub(r'[^0-9]', '', val_str))
        return num * 1024
    elif val_str.endswith("MB") or val_str.endswith("M"):
        num = int(re.sub(r'[^0-9]', '', val_str))
        return num * 1024 * 1024
    elif val_str.endswith("B"):
        return int(re.sub(r'[^0-9]', '', val_str))
    try:
        return int(val_str)
    except:
        return val_str

def format_byte_size(bytes_val):
    try:
        b = int(bytes_val)
        if b >= 1048576 and b % 1048576 == 0:
            return f"{b // 1048576} MB ({b} B)"
        elif b >= 1024 and b % 1024 == 0:
            return f"{b // 1024} KB ({b} B)"
        return f"{b} B"
    except:
        return str(bytes_val)

def is_power_of_two(n):
    return n > 0 and (n & (n - 1)) == 0

def validate_config(cfg):
    """Validates configuration parameters and returns a list of warning/error messages."""
    errors = []
    
    # Check simulation section
    if cfg.has_section("simulation"):
        elf = cfg.get("simulation", "elf_path", fallback="")
        if elf:
            elf_full = elf if os.path.isabs(elf) else os.path.join(PROJECT_ROOT, elf)
            if not os.path.exists(elf_full):
                errors.append(f"ELF binary does not exist: {elf}")
        mode = cfg.get("simulation", "mode", fallback="uarch")
        if mode not in ["uarch", "isa_only"]:
            errors.append(f"Invalid simulation mode: '{mode}'. Must be 'uarch' or 'isa_only'.")

    # Check core section
    if cfg.has_section("core"):
        for w in ["fetch_width", "decode_width", "rename_width", "issue_width", "commit_width"]:
            val = cfg.getint("core", w, fallback=4)
            if val <= 0:
                errors.append(f"Core {w} must be > 0 (got {val})")
        rob = cfg.getint("core", "rob_size", fallback=64)
        if rob <= 0: errors.append(f"ROB size must be > 0 (got {rob})")
        prf = cfg.getint("core", "num_phys_regs", fallback=128)
        if prf <= 16: errors.append(f"Physical register count must be > 16 (got {prf})")

    # Check branch predictor
    if cfg.has_section("branch_predictor"):
        bp_type = cfg.get("branch_predictor", "type", fallback="TAGE").upper()
        if bp_type not in ["IDEAL", "BIMODAL", "GSHARE", "TAGE"]:
            errors.append(f"Invalid branch predictor type: '{bp_type}'. Must be IDEAL, BIMODAL, GSHARE, or TAGE.")
        for table in ["table_size", "btb_size"]:
            t_val = cfg.getint("branch_predictor", table, fallback=4096)
            if not is_power_of_two(t_val):
                errors.append(f"Branch predictor {table} must be a power of 2 (got {t_val})")

    # Check caches
    for c_sec in ["cache_l1i", "cache_l1d", "cache_l2"]:
        if cfg.has_section(c_sec):
            c_size = cfg.getint(c_sec, "size_bytes", fallback=32768)
            c_assoc = cfg.getint(c_sec, "associativity", fallback=4)
            if not is_power_of_two(c_size):
                errors.append(f"Cache [{c_sec}] size_bytes must be a power of 2 (got {c_size})")
            if not is_power_of_two(c_assoc):
                errors.append(f"Cache [{c_sec}] associativity must be a power of 2 (got {c_assoc})")

    return errors

def load_ini(file_path):
    cfg = configparser.ConfigParser()
    cfg.read(file_path)
    return cfg

def save_ini(cfg, file_path):
    errors = validate_config(cfg)
    if errors:
        print("\n\033[1;31m[VALIDATION WARNING / ERROR]\033[0m")
        for err in errors:
            print(f"  ❌ {err}")
        print("\033[1;33mPlease fix invalid parameters before using this configuration.\033[0m\n")
    
    with open(file_path, 'w') as f:
        cfg.write(f)
    print(f"\033[1;32m[OK]\033[0m Configuration written to: \033[1m{file_path}\033[0m")

def print_config_table(cfg, title="Active Configuration"):
    print("\n" + "=" * 76)
    print(f"       TinyCpuSim Configuration: {title}")
    print("=" * 76)
    
    # Simulation Section
    if cfg.has_section("simulation"):
        print("[simulation]")
        print(f"  Target ELF Binary:       \033[1;36m{cfg.get('simulation', 'elf_path', fallback='N/A')}\033[0m")
        print(f"  Simulation Mode:         {cfg.get('simulation', 'mode', fallback='uarch')}")
        print(f"  Detailed Hardware Log:   {cfg.get('simulation', 'all_perf', fallback='true')}")
        print(f"  Top-Down TMAM Profiler:  {cfg.get('simulation', 'enable_topdown', fallback='true')}")
        print(f"  Max Instruction Steps:   {cfg.get('simulation', 'max_steps', fallback='1000000000')}")
        print("-" * 76)

    # System Section
    if cfg.has_section("system"):
        print("[system]")
        print(f"  Core Count:              {cfg.get('system', 'num_cores', fallback='1')} core(s)")
        print(f"  DRAM Latency:            {cfg.get('system', 'dram_latency_cycles', fallback='80')} cycles")
        print(f"  MESI Coherence:          {cfg.get('system', 'enable_mesi_coherence', fallback='true')}")
        print("-" * 76)

    # Core Section
    if cfg.has_section("core"):
        print("[core]")
        print(f"  Pipeline Width (F/D/I/C):{cfg.get('core', 'fetch_width', fallback='4')} / {cfg.get('core', 'decode_width', fallback='4')} / {cfg.get('core', 'issue_width', fallback='4')} / {cfg.get('core', 'commit_width', fallback='4')}")
        print(f"  ROB Size:                {cfg.get('core', 'rob_size', fallback='64')} entries")
        print(f"  Issue Queue (RS) Size:   {cfg.get('core', 'rs_size', fallback='32')} entries")
        print(f"  Physical Registers (PRF):{cfg.get('core', 'num_phys_regs', fallback='128')} registers")
        print("-" * 76)

    # Branch Predictor
    if cfg.has_section("branch_predictor"):
        print("[branch_predictor]")
        print(f"  Predictor Algorithm:     \033[1;33m{cfg.get('branch_predictor', 'type', fallback='TAGE')}\033[0m")
        print(f"  BTB Capacity:            {cfg.get('branch_predictor', 'btb_size', fallback='4096')} entries")
        print(f"  RAS Depth:               {cfg.get('branch_predictor', 'ras_size', fallback='32')} entries")
        print(f"  PHT Table Size:          {cfg.get('branch_predictor', 'table_size', fallback='4096')} entries")
        print("-" * 76)

    # LSU
    if cfg.has_section("lsu"):
        print("[lsu]")
        print(f"  Load / Store Queue:      {cfg.get('lsu', 'lq_size', fallback='16')} / {cfg.get('lsu', 'sq_size', fallback='16')} entries")
        print(f"  Store Forwarding:        {cfg.get('lsu', 'enable_store_forwarding', fallback='true')}")
        print("-" * 76)

    # Caches
    print("[caches]")
    if cfg.has_section("cache_l1i"):
        print(f"  L1 Instruction Cache:    {format_byte_size(cfg.get('cache_l1i', 'size_bytes', fallback='32768'))} ({cfg.get('cache_l1i', 'associativity', fallback='4')}-way)")
    if cfg.has_section("cache_l1d"):
        print(f"  L1 Data Cache:           {format_byte_size(cfg.get('cache_l1d', 'size_bytes', fallback='32768'))} ({cfg.get('cache_l1d', 'associativity', fallback='4')}-way)")
    if cfg.has_section("cache_l2"):
        print(f"  Shared L2 Cache:         {format_byte_size(cfg.get('cache_l2', 'size_bytes', fallback='524288'))} ({cfg.get('cache_l2', 'associativity', fallback='8')}-way)")
    print("=" * 76)

def set_key_val(cfg, key, val):
    key = key.strip().lower()
    val = val.strip()

    sec = "core"
    actual_key = key
    if key in ALIASES:
        sec, actual_key = ALIASES[key]
    else:
        for s in cfg.sections():
            if cfg.has_option(s, key):
                sec = s
                actual_key = key
                break

    # Auto convert KB / MB to integer bytes
    if any(k_sub in actual_key for k_sub in ["size_bytes", "size"]) and any(c.isalpha() for c in val):
        val = str(parse_byte_size(val))

    # Special case for width
    if actual_key == "issue_width" and key in ["width", "issue_width"]:
        if not cfg.has_section("core"): cfg.add_section("core")
        cfg.set("core", "fetch_width", val)
        cfg.set("core", "decode_width", val)
        cfg.set("core", "rename_width", val)
        cfg.set("core", "issue_width", val)
        cfg.set("core", "commit_width", val)
        print(f"  Updated pipeline widths (fetch/decode/rename/issue/commit) = {val}")
        return

    if not cfg.has_section(sec):
        cfg.add_section(sec)
    cfg.set(sec, actual_key, val)
    print(f"  Updated [{sec}] {actual_key} = {val}")

def run_sweep(sweep_cfg_files):
    if not sweep_cfg_files:
        sweep_cfg_files = sorted(glob.glob(os.path.join(SWEEP_DIR, "*.cfg")))
    
    if not sweep_cfg_files:
        print("No sweep configurations found in configs/sweep/")
        print("Create sweep configs in configs/sweep/ (e.g. rob16.cfg, rob32.cfg, rob64.cfg).")
        return

    print("=" * 76)
    print(f" Running Batch Sweep across {len(sweep_cfg_files)} configuration files")
    print("=" * 76)

    # 1. Check for ELF mismatches across sweep configs
    elf_map = {}
    for f in sweep_cfg_files:
        c = load_ini(f)
        elf = c.get("simulation", "elf_path", fallback="tests/fixtures/test_fibonacci.elf")
        elf_map[f] = elf

    unique_elfs = set(elf_map.values())
    if len(unique_elfs) > 1:
        print("\n\033[1;33m⚠️  [WARNING: MISMATCHED ELF PATHS DETECTED ACROSS SWEEP CONFIGS]\033[0m")
        print("  Sweep Mode is intended to compare different hardware configurations on the SAME program.")
        print("  Detected conflicting ELF paths:")
        for cfg_file, e_path in elf_map.items():
            print(f"    - {os.path.basename(cfg_file)}: {e_path}")
        
        print("\nUnifying all sweep runs to the current active ELF binary...")
        curr_cfg = load_ini(CURRENT_CFG)
        target_elf = curr_cfg.get("simulation", "elf_path", fallback="tests/fixtures/test_fibonacci.elf")
        print(f"  Target ELF for all runs: \033[1;32m{target_elf}\033[0m\n")
    else:
        target_elf = list(unique_elfs)[0]
        print(f"  Target ELF for all sweep runs: \033[1;32m{target_elf}\033[0m\n")

    sim_bin = os.path.join(PROJECT_ROOT, "build", "tinycpusim")
    if not os.path.exists(sim_bin):
        subprocess.run([os.path.join(PROJECT_ROOT, "scripts", "01_build.sh")], check=True)

    elf_full = target_elf if os.path.isabs(target_elf) else os.path.join(PROJECT_ROOT, target_elf)

    print(f"{'Config Name':<20} | {'Cycles':<10} | {'IPC':<8} | {'Branch Acc':<12} | {'L1D Hit Rate':<12}")
    print("-" * 76)

    for cfg_path in sweep_cfg_files:
        cmd = [sim_bin, "--uarch", "--uarch-config", cfg_path, elf_full]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        out = res.stdout
        
        m_c = re.search(r'Simulated Total Cycles:\s+(\d+)', out)
        cycles = m_c.group(1) if m_c else 'N/A'
        
        m_ipc = re.search(r'Aggregate Throughput \(IPC\):\s*([\d\.]+)', out)
        ipc = m_ipc.group(1) if m_ipc else 'N/A'
        
        m_ba = re.search(r'Branch Predictions:\s+\d+\s+\(Accuracy:\s*([\d\.]+)%\)', out)
        b_acc = f"{m_ba.group(1)}%" if m_ba else 'N/A'
        
        m_l1d = re.search(r'L1D Cache Hit Rate:\s*([\d\.]+)%', out)
        l1d = f"{m_l1d.group(1)}%" if m_l1d else 'N/A'

        print(f"{os.path.basename(cfg_path):<20} | {cycles:<10} | {ipc:<8} | {b_acc:<12} | {l1d:<12}")

    print("=" * 76)

def interactive_menu():
    ensure_dirs()
    while True:
        cfg = load_ini(CURRENT_CFG)
        print("\n============================================================")
        print("          TinyCpuSim Configuration Manager (TUI)            ")
        print("============================================================")
        print(f" Active Config: \033[1m{CURRENT_CFG}\033[0m")
        print(f" Target ELF:    \033[1;36m{cfg.get('simulation', 'elf_path', fallback='tests/fixtures/test_fibonacci.elf')}\033[0m")
        print(f" Hardware Core: {cfg.get('core', 'issue_width', fallback='4')}-wide | ROB: {cfg.get('core', 'rob_size', fallback='64')} | BPU: {cfg.get('branch_predictor', 'type', fallback='TAGE')} | Cores: {cfg.get('system', 'num_cores', fallback='1')}")
        print("------------------------------------------------------------")
        print("  [1] Show Full Active Configuration Table")
        print("  [2] Select Target ELF Workload (Fibonacci, Sort, Stress, etc.)")
        print("  [3] Set Hardware Knobs (ROB, Width, PRF, Cache size/assoc, etc.)")
        print("  [4] Quick Preset (1-Core Default vs 4-Core Multicore)")
        print("  [5] Save Current Configuration Snapshot (to configs/save/)")
        print("  [6] Load Configuration Snapshot (from configs/save/)")
        print("  [7] Run Sweep Batch Evaluation (on configs/sweep/)")
        print("  [8] Reset Active Config to Factory Default")
        print("  [0] Exit / Return to Main Launcher")
        print("============================================================")
        choice = input("Enter choice [0-8]: ").strip()

        if choice == "1":
            print_config_table(cfg, "Active (configs/current.cfg)")
        elif choice == "2":
            print("\nAvailable ELF Fixtures:")
            elfs = sorted(glob.glob(os.path.join(FIXTURES_DIR, "*.elf")))
            for i, e in enumerate(elfs, 1):
                print(f"  [{i}] {os.path.basename(e)}")
            sel = input("Select ELF number or enter filename: ").strip()
            if sel.isdigit() and 1 <= int(sel) <= len(elfs):
                chosen_elf = os.path.relpath(elfs[int(sel)-1], PROJECT_ROOT)
                set_key_val(cfg, "elf_path", chosen_elf)
                save_ini(cfg, CURRENT_CFG)
            elif sel:
                set_key_val(cfg, "elf_path", sel)
                save_ini(cfg, CURRENT_CFG)
        elif choice == "3":
            print("\nModify Hardware Knobs (e.g. 'rob=128', 'width=8', 'l1d_size=64KB', 'bp=TAGE'):")
            expr = input("Enter parameter assignment(s) [e.g. rob=128 width=8]: ").strip()
            if expr:
                for pair in expr.split():
                    if '=' in pair:
                        k, v = pair.split('=', 1)
                        set_key_val(cfg, k, v)
                save_ini(cfg, CURRENT_CFG)
        elif choice == "4":
            print("\nPresets:")
            print("  [1] Single-Core Baseline (OoO 4-wide, 64 ROB, 32KB L1)")
            print("  [2] Multi-Core Baseline (4-Core OoO, MESI Coherence, 1MB L2)")
            p_sel = input("Choose preset [1-2]: ").strip()
            if p_sel == "1":
                shutil.copyfile(os.path.join(DEFAULT_DIR, "default.cfg"), CURRENT_CFG)
                print(f"\033[1;32m[OK]\033[0m Loaded Single-Core Baseline into {CURRENT_CFG}")
            elif p_sel == "2":
                shutil.copyfile(os.path.join(DEFAULT_DIR, "multicore.cfg"), CURRENT_CFG)
                print(f"\033[1;32m[OK]\033[0m Loaded Multi-Core Baseline into {CURRENT_CFG}")
        elif choice == "5":
            name = input("Enter snapshot name to save in configs/save/ (e.g. opt_v1): ").strip()
            if name:
                if not name.endswith('.cfg'): name += '.cfg'
                save_path = os.path.join(SAVE_DIR, name)
                save_ini(cfg, save_path)
        elif choice == "6":
            saved_files = sorted(glob.glob(os.path.join(SAVE_DIR, "*.cfg")))
            if not saved_files:
                print("No saved snapshots found in configs/save/.")
            else:
                print("\nSaved Snapshots:")
                for i, s_file in enumerate(saved_files, 1):
                    print(f"  [{i}] {os.path.basename(s_file)}")
                s_sel = input("Select snapshot to load: ").strip()
                if s_sel.isdigit() and 1 <= int(s_sel) <= len(saved_files):
                    chosen = saved_files[int(s_sel)-1]
                    shutil.copyfile(chosen, CURRENT_CFG)
                    print(f"\033[1;32m[OK]\033[0m Loaded {os.path.basename(chosen)} into \033[1m{CURRENT_CFG}\033[0m")
        elif choice == "7":
            run_sweep([])
        elif choice == "8":
            shutil.copyfile(os.path.join(DEFAULT_DIR, "default.cfg"), CURRENT_CFG)
            print(f"\033[1;32m[OK]\033[0m Reset {CURRENT_CFG} to factory default.")
        elif choice in ["0", "q", "Q"]:
            print(f"\nExiting Configuration Manager. Current active config is ready at: \033[1m{CURRENT_CFG}\033[0m")
            break

def main():
    ensure_dirs()
    parser = argparse.ArgumentParser(description="TinyCpuSim Configuration Manager")
    subparsers = parser.add_subparsers(dest="command")

    # show
    subparsers.add_parser("show", help="Display the active configuration table")

    # set
    set_parser = subparsers.add_parser("set", help="Set one or more configuration parameters")
    set_parser.add_argument("assignments", nargs="+", help="Assignments in format key=value (e.g. rob=128 width=8 l1d_size=64KB)")

    # reset
    subparsers.add_parser("reset", help="Reset active configuration to default template")

    # preset
    preset_parser = subparsers.add_parser("preset", help="Apply official preset")
    preset_parser.add_argument("name", choices=["default", "multicore"], help="Preset name")

    # save
    save_parser = subparsers.add_parser("save", help="Save active configuration to configs/save/<name>.cfg")
    save_parser.add_argument("name", help="Snapshot filename")

    # load
    load_parser = subparsers.add_parser("load", help="Load saved configuration from configs/save/<name>.cfg")
    load_parser.add_argument("name", help="Snapshot filename")

    # list
    subparsers.add_parser("list", help="List all configs in default/, save/, and sweep/")

    # delete
    del_parser = subparsers.add_parser("delete", help="Delete a saved configuration in configs/save/")
    del_parser.add_argument("name", help="Snapshot filename to delete")

    # sweep
    sweep_parser = subparsers.add_parser("sweep", help="Run batch sweep evaluation across configs/sweep/")
    sweep_parser.add_argument("configs", nargs="*", help="Optional list of config files")

    # menu
    subparsers.add_parser("menu", help="Launch interactive TUI menu")

    args = parser.parse_args()

    if not args.command or args.command == "menu":
        interactive_menu()
        return

    cfg = load_ini(CURRENT_CFG)

    if args.command == "show":
        print_config_table(cfg, f"Active ({CURRENT_CFG})")

    elif args.command == "set":
        for a in args.assignments:
            if '=' in a:
                k, v = a.split('=', 1)
                set_key_val(cfg, k, v)
        save_ini(cfg, CURRENT_CFG)

    elif args.command == "reset":
        shutil.copyfile(os.path.join(DEFAULT_DIR, "default.cfg"), CURRENT_CFG)
        print(f"\033[1;32m[OK]\033[0m Reset active configuration to: \033[1m{CURRENT_CFG}\033[0m")

    elif args.command == "preset":
        src = os.path.join(DEFAULT_DIR, f"{args.name}.cfg")
        if os.path.exists(src):
            shutil.copyfile(src, CURRENT_CFG)
            print(f"\033[1;32m[OK]\033[0m Applied preset '{args.name}' to: \033[1m{CURRENT_CFG}\033[0m")
        else:
            print(f"Error: Preset {src} not found.")

    elif args.command == "save":
        name = args.name
        if not name.endswith('.cfg'): name += '.cfg'
        save_path = os.path.join(SAVE_DIR, name)
        save_ini(cfg, save_path)

    elif args.command == "load":
        name = args.name
        if not name.endswith('.cfg'): name += '.cfg'
        src_path = os.path.join(SAVE_DIR, name)
        if not os.path.exists(src_path):
            src_path = os.path.join(DEFAULT_DIR, name)
        if os.path.exists(src_path):
            shutil.copyfile(src_path, CURRENT_CFG)
            print(f"\033[1;32m[OK]\033[0m Loaded configuration from {src_path} into: \033[1m{CURRENT_CFG}\033[0m")
        else:
            print(f"Error: Config {name} not found in configs/save/ or configs/default/.")

    elif args.command == "list":
        print("=" * 60)
        print("          TinyCpuSim Configuration Repository               ")
        print("=" * 60)
        print("  [default/]")
        for f in sorted(glob.glob(os.path.join(DEFAULT_DIR, "*.cfg"))):
            print(f"    - {os.path.basename(f)}")
        print("  [save/]")
        saved = sorted(glob.glob(os.path.join(SAVE_DIR, "*.cfg")))
        if not saved: print("    (No saved configurations)")
        for f in saved:
            print(f"    - {os.path.basename(f)}")
        print("  [sweep/]")
        sweeps = sorted(glob.glob(os.path.join(SWEEP_DIR, "*.cfg")))
        if not sweeps: print("    (No sweep configurations)")
        for f in sweeps:
            print(f"    - {os.path.basename(f)}")
        print("=" * 60)

    elif args.command == "delete":
        name = args.name
        if not name.endswith('.cfg'): name += '.cfg'
        target = os.path.join(SAVE_DIR, name)
        if os.path.exists(target):
            os.remove(target)
            print(f"\033[1;32m[OK]\033[0m Deleted {target}")
        else:
            print(f"Error: {target} not found.")

    elif args.command == "sweep":
        run_sweep(args.configs)

if __name__ == '__main__':
    main()
