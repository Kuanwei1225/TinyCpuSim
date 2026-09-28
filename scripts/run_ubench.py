#!/usr/bin/python3
"""
TinyCpuSim Microbenchmark Performance Suite & Configuration Sensitivity Engine
Evaluates component microbenchmarks, captures cycle-accurate hardware timing,
analyzes which config modification has the best/worst effect on specific test cases,
and saves individual & summary reports to reports/ubench/.
"""

import os
import sys
import re
import argparse
import subprocess
import datetime
import glob
import configparser

def find_default_tool(name):
    for candidate in [
        f"/usr/bin/{name}",
        f"/usr/local/bin/{name}",
        f"/opt/homebrew/bin/{name}",
        f"/bin/{name}"
    ]:
        if os.path.exists(candidate) and os.access(candidate, os.X_OK):
            return candidate
    return name

CMAKE_BIN = find_default_tool("cmake")

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
CONFIGS_DIR = os.path.join(PROJECT_ROOT, "configs")
DEFAULT_CFG = os.path.join(CONFIGS_DIR, "default", "default.cfg")
CURRENT_CFG = os.path.join(CONFIGS_DIR, "current.cfg")
REPORTS_DIR = os.path.join(PROJECT_ROOT, "reports", "ubench")
DEFAULT_REPORTS_DIR = os.path.join(REPORTS_DIR, "default")
FIXTURES_DIR = os.path.join(PROJECT_ROOT, "tests", "fixtures")

UBENCH_SUITES = {
    "bpu": {
        "name": "BPU & Frontend",
        "description": "Branch Prediction (TAGE/BTB/RAS), Instruction Fetch & Renaming",
        "binary": "bpu_frontend_ubench_test",
        "workload_elf": "test_branch_pred.elf",
        "sensitive_knobs": ["btb_size", "type", "table_size", "ras_size", "enabled", "fetch_width", "decode_width"],
        "tests": [
            ("BPU_UBench_TightLoopAlwaysTaken", "Loop branch saturation & steady-state 100% accuracy", "Direction Acc", 100.0, "%", "high"),
            ("BPU_UBench_DeepNestedCallReturnRAS", "Call/Return stack prediction up to 16 depths", "RAS Hit Rate", 100.0, "%", "high"),
            ("BPU_UBench_CorrelatedBranchesTAGE", "Multi-table TAGE geometric history correlation", "TAGE Accuracy", 97.5, "%", "high"),
            ("BPU_UBench_AlternatingPatternTNTN", "GShare global history pattern correlation", "History Acc", 92.5, "%", "high"),
            ("Frontend_UBench_MultiUopExpansionThroughput", "32-bit Thumb / LDM multi-uOp decode expansion", "Decode Width", 4.0, " uOp/cyc", "high"),
            ("Frontend_UBench_SpeculativeCheckpointRestore", "Branch misprediction RAT snapshot restore", "Restore Latency", 0.0, " cyc", "low"),
            ("Frontend_UBench_CrossCacheLineFetch", "Unaligned multi-word instruction boundary fetch", "Fetch Throughput", 4.0, " uOp/cyc", "high"),
            ("Frontend_UBench_FlagsRenamingWakeup", "NZCV flags condition code dependency renaming", "Wakeup Latency", 1.0, " cyc", "low"),
            ("Frontend_UBench_PrfExhaustionStall", "Physical register file freelist starvation stall", "Recovery Latency", 1.0, " cyc", "low"),
            ("Frontend_UBench_DecoderIllegalOpcodeFault", "Undefined instruction precise fault detection", "Fault Precision", 100.0, "%", "high"),
            ("BPU_UBench_IndirectCallTargetThrashing", "Polymorphic indirect branch target switching", "BTB Hit Rate", 42.1, "%", "high"),
            ("BPU_UBench_BranchTargetBufferAliasStress", "BTB hash collision & capacity stress", "BTB Alias Stalls", 328.0, " cyc", "low"),
        ]
    },
    "exec": {
        "name": "Execution Engine & LSU",
        "description": "ALU/MUL/DIV latency, Issue Queue scheduling, LSQ Store-to-Load bypass",
        "binary": "exec_lsu_ubench_test",
        "workload_elf": "test_store_forward.elf",
        "sensitive_knobs": ["issue_width", "enable_ooo", "rs_size", "lq_size", "sq_size", "enable_store_forwarding"],
        "tests": [
            ("LSU_UBench_ExactStoreToLoadForwarding", "0-cycle Store Queue to Load Queue memory forwarding", "Forward Rate", 100.0, "%", "high"),
            ("Exec_UBench_RawDependencyChainLatency", "Back-to-back RAW physical register dependency", "Latency / uop", 1.0, " cyc", "low"),
            ("Exec_UBench_MaxIssueWidthSaturation", "Peak 4-wide superscalar issue port saturation", "Issue Width", 4.0, " uop/cyc", "high"),
            ("Exec_UBench_MulDivPipelinedLatency", "Pipelined 3-cycle MUL and 8-cycle DIV non-blocking execution", "Throughput", 1.0, " uop/cyc", "high"),
            ("Exec_UBench_OutOfOrderConditionEvaluation", "Out-of-order execution with speculative flags", "OoO Speedup", 1.50, "x", "high"),
            ("Exec_UBench_AgeOrderedContentionIssue", "Oldest-ready-first priority issue queue arbitration", "Priority Order", 100.0, "%", "high"),
            ("Exec_UBench_ExecutionPortContention", "Port 0/1/2 structural conflict resolution", "Port Efficiency", 98.0, "%", "high"),
            ("LSU_UBench_LoadStoreQueueWrapAround", "Circular ring buffer LSU pointer wrap stress", "Ring Integrity", 100.0, "%", "high"),
            ("LSU_UBench_L1CacheHitVsMissLatency", "1-cycle L1D hit vs DRAM access latency differential", "L1 Latency", 1.0, " cyc", "low"),
            ("LSU_UBench_MemoryOrderViolationDetection", "Younger speculative load address aliasing squashing", "Violation Recovery", 100.0, "%", "high"),
            ("LSU_UBench_StoreDataPendingReplay", "Load replay on uncommitted store data pend", "Replay Penalty", 2.0, " cyc", "low"),
            ("LSU_UBench_StridedAccessCacheThrashing", "Non-contiguous stride access cache miss penalty", "Miss Latency", 80.0, " cyc", "low"),
        ]
    },
    "rob": {
        "name": "ROB & Top-Down TMAM",
        "description": "In-order retirement, exception recovery, Top-Down slot accounting",
        "binary": "rob_topdown_ubench_test",
        "workload_elf": "test_stress.elf",
        "sensitive_knobs": ["rob_size", "commit_width", "num_phys_regs", "enable_topdown"],
        "tests": [
            ("TopDown_UBench_SlotConservationInvariant", "Frontend + Backend + BadSpec + Retiring == 100%", "Slot Conservation", 100.0, "%", "high"),
            ("ROB_UBench_SustainedRetireThroughput", "4-wide sustained retirement throughput saturation", "Retire Width", 4.0, " uop/cyc", "high"),
            ("ROB_UBench_SpeculativeStoreDrainOnRetire", "Speculative store commits and drains to memory bus", "Drain Rate", 100.0, "%", "high"),
            ("ROB_UBench_CircularBufferWrapAroundStress", "Head/tail pointer 64-entry wrap around stress", "Pointer Integrity", 100.0, "%", "high"),
            ("TopDown_UBench_FrontendVsBackendBreakdown", "Fetch stall vs execution stall slot attribution", "Attribution Acc", 100.0, "%", "high"),
            ("TopDown_UBench_BadSpeculationAccounting", "Squashed speculative issue slot accounting", "Squash Attribution", 100.0, "%", "high"),
            ("ROB_UBench_RoiBoundaryResetStats", "Dynamic m5ops ROI performance counter reset & dump", "ROI Precision", 100.0, "%", "high"),
            ("TopDown_UBench_BottleneckParetoRanking", "Automated Pareto bottleneck ranking generator", "Ranking Acc", 100.0, "%", "high"),
            ("ROB_UBench_IsYoungerCircularAgeDistance", "Modulo circular index age comparison correctness", "Age Ordering", 100.0, "%", "high"),
            ("ROB_UBench_MultipleBranchMispredictFlushes", "Back-to-back branch misprediction squashes", "Squash Cleanliness", 100.0, "%", "high"),
            ("ROB_UBench_HeadOfRobBlockingRetire", "Uncompleted head-of-ROB instruction blocking retire", "Head Block Stalls", 25.0, " cyc", "low"),
        ]
    },
    "cache": {
        "name": "Cache Hierarchy & MESI",
        "description": "L1I/L1D non-blocking caches, MSHRs, L2 write-back, MESI multi-core coherence",
        "binary": "cache_ubench_test",
        "workload_elf": "test_mem_stride.elf",
        "sensitive_knobs": ["size_bytes", "associativity", "hit_latency_cycles", "mshr_entries", "enable_mesi_coherence"],
        "tests": [
            ("Cache_UBench_L1HitLatencyAndThroughput", "1-cycle L1 cache access latency & line refill", "Hit Latency", 1.0, " cyc", "low"),
            ("Cache_UBench_MshrNonBlockingAllocation", "8 outstanding non-blocking cache misses in flight", "MSHR Concurrency", 8.0, " misses", "high"),
            ("Cache_UBench_MesiCoherenceStateTransitions", "Modified, Exclusive, Shared, Invalid state transitions", "MESI Transitions", 100.0, "%", "high"),
            ("Cache_UBench_WriteBackDirtyEviction", "Dirty cache line write-back to L2/DRAM on eviction", "Writeback Integrity", 100.0, "%", "high"),
            ("Cache_UBench_SharedL2HierarchicalInclusion", "L2 inclusive cache tag lookups & L1 invalidate", "Inclusion Check", 100.0, "%", "high"),
            ("Cache_UBench_LruReplacementSetAssociativity", "4-way LRU set replacement eviction correctness", "Eviction Cycles", 10.0, " cyc", "low"),
        ]
    }
}

def parse_ini_file(file_path):
    config = configparser.ConfigParser(strict=False, inline_comment_prefixes=('#', ';'))
    if file_path and os.path.exists(file_path):
        try:
            config.read(file_path)
        except Exception:
            pass
    return config

def diff_hardware_configs(base_path, active_path):
    base_cfg = parse_ini_file(base_path)
    act_cfg = parse_ini_file(active_path)
    diffs = []
    all_secs = sorted(list(set(base_cfg.sections()) | set(act_cfg.sections())))
    for sec in all_secs:
        if sec in ["workload", "simulation"]:
            continue
        base_items = dict(base_cfg.items(sec)) if base_cfg.has_section(sec) else {}
        act_items = dict(act_cfg.items(sec)) if act_cfg.has_section(sec) else {}
        for k in sorted(list(set(base_items.keys()) | set(act_items.keys()))):
            v_b = base_items.get(k, "<unset>")
            v_a = act_items.get(k, "<unset>")
            if v_b != v_a:
                diffs.append((sec, k, v_b, v_a))
    return diffs

SUITE_METRICS = {
    "bpu": [
        ("Simulated Total Cycles", "cycles", "", True),
        ("Throughput (IPC)", "ipc", "", False),
        ("Branch Predictor Accuracy", "branch_acc", "%", False),
        ("Branch Mispredict Penalty Flushes", "branch_mispredicts", "", True),
        ("BTB Target Hit Rate", "btb_hit_rate", "%", False),
        ("RAS Return Hit Rate", "ras_hit_rate", "%", False),
        ("Front-End Bound (Fetch/BPU)", "frontend_bound", "%", True),
        ("Bad Speculation (Squashed)", "bad_spec", "%", True),
        ("Retiring (Useful Work)", "retiring", "%", False),
    ],
    "exec": [
        ("Simulated Total Cycles", "cycles", "", True),
        ("Throughput (IPC)", "ipc", "", False),
        ("Issue Queue (RS) Full Stalls", "rs_stalls", " cyc", True),
        ("Store-to-Load Bypass Rate", "forwarding_rate", "%", False),
        ("ROB Full Stalls", "rob_stalls", " cyc", True),
        ("L1D Cache Hit Rate", "l1d_hit_rate", "%", False),
        ("Back-End Bound (Stalls)", "backend_bound", "%", True),
        ("Retiring (Useful Work)", "retiring", "%", False),
    ],
    "rob": [
        ("Simulated Total Cycles", "cycles", "", True),
        ("Throughput (IPC)", "ipc", "", False),
        ("ROB Full Stalls", "rob_stalls", " cyc", True),
        ("Head-of-ROB Stalls", "head_rob_stalls", " cyc", True),
        ("PRF FreeList Exhaustion Stalls", "prf_stalls", " cyc", True),
        ("Retiring (Useful Work)", "retiring", "%", False),
        ("Bad Speculation (Squashed)", "bad_spec", "%", True),
        ("Back-End Bound (Stalls)", "backend_bound", "%", True),
    ],
    "cache": [
        ("Simulated Total Cycles", "cycles", "", True),
        ("Throughput (IPC)", "ipc", "", False),
        ("L1I Cache Hit Rate", "l1i_hit_rate", "%", False),
        ("L1D Cache Hit Rate", "l1d_hit_rate", "%", False),
        ("Shared L2 Cache Hit Rate", "l2_hit_rate", "%", False),
        ("L1D MSHR Saturation Stalls", "l1d_mshr_stalls", " cyc", True),
        ("Back-End Bound (Mem Stalls)", "backend_bound", "%", True),
    ]
}

def format_row(label, v_base, v_exp, unit="", lower_better=False):
    delta_str = "---"
    if isinstance(v_base, (int, float)) and isinstance(v_exp, (int, float)):
        if v_base > 0:
            delta = ((v_exp - v_base) / float(v_base)) * 100.0
            if abs(delta) < 0.001:
                delta_str = "0.0%"
            else:
                sign = "+" if delta > 0 else ""
                # Coloring
                if lower_better:
                    color = "\033[1;32m" if delta < 0 else ("\033[1;31m" if delta > 0 else "")
                else:
                    color = "\033[1;32m" if delta > 0 else ("\033[1;31m" if delta < 0 else "")
                delta_str = f"{color}{sign}{delta:.2f}%\033[0m"
        elif v_base == 0 and v_exp > 0:
            color = "\033[1;31m" if lower_better else "\033[1;32m"
            delta_val = f"{v_exp:.2f}{unit}" if isinstance(v_exp, float) else f"{v_exp}{unit}"
            delta_str = f"{color}+{delta_val}\033[0m"
        elif v_base == 0 and v_exp == 0:
            delta_str = "0.0%"

    if "IPC" in label and isinstance(v_base, float):
        fmt_base = f"{v_base:.3f}"
        fmt_exp = f"{v_exp:.3f}" if isinstance(v_exp, float) else str(v_exp)
    else:
        fmt_base = f"{v_base:.2f}{unit}" if isinstance(v_base, float) else f"{v_base}{unit}" if v_base != 'N/A' and v_base is not None else 'N/A'
        fmt_exp = f"{v_exp:.2f}{unit}" if isinstance(v_exp, float) else f"{v_exp}{unit}" if v_exp != 'N/A' and v_exp is not None else 'N/A'
    return f"    {label:<34} | {fmt_base:<13} | {fmt_exp:<13} | {delta_str:<12}"

def parse_uarch_stats(log_text):
    stats = {}
    m = re.search(r'Simulated Total Cycles:\s+(\d+)', log_text)
    if m: stats['cycles'] = int(m.group(1))
    m = re.search(r'Aggregate Throughput \(IPC\):\s*([\d\.]+)', log_text)
    if m: stats['ipc'] = float(m.group(1))
    m = re.search(r'Total Committed Insts:\s+(\d+)', log_text)
    if m: stats['insts'] = int(m.group(1))
    m = re.search(r'Total Committed uOps:\s+(\d+)', log_text)
    if m: stats['uops'] = int(m.group(1))
    m = re.search(r'Branch Predictions:\s+\d+\s+\(Accuracy:\s*([\d\.]+)%\)', log_text)
    if m: stats['branch_acc'] = float(m.group(1))
    m = re.search(r'Branch Mispredicts:\s+(\d+)', log_text)
    if m: stats['branch_mispredicts'] = int(m.group(1))
    m = re.search(r'BTB Hits / Misses:\s+\d+\s+/\s+\d+\s+\(Hit Rate:\s*([\d\.]+)%\)', log_text)
    if m: stats['btb_hit_rate'] = float(m.group(1))
    m = re.search(r'RAS Hits / Misses:\s+\d+\s+/\s+\d+\s+\(Hit Rate:\s*([\d\.]+)%\)', log_text)
    if m: stats['ras_hit_rate'] = float(m.group(1))
    m = re.search(r'L1I Cache Hit Rate:\s*([\d\.]+)%', log_text)
    if m: stats['l1i_hit_rate'] = float(m.group(1))
    m = re.search(r'L1D Cache Hit Rate:\s*([\d\.]+)%', log_text)
    if m: stats['l1d_hit_rate'] = float(m.group(1))
    m = re.search(r'L2 Hits / Misses:\s+\d+\s+/\s+\d+\s+\(Hit Rate:\s*([\d\.]+)%\)', log_text)
    if m: stats['l2_hit_rate'] = float(m.group(1))
    m = re.search(r'Store-to-Load Forwards:\s+(\d+)(?:\s+\(Rate:\s*([\d\.]+)%\))?', log_text)
    if m:
        stats['store_forwards'] = int(m.group(1))
        if m.group(2): stats['forwarding_rate'] = float(m.group(2))
        else: stats['forwarding_rate'] = 100.0 if int(m.group(1)) > 0 else 0.0
    m = re.search(r'RS/IQ Full Stalls:\s+(\d+)', log_text)
    if m: stats['rs_stalls'] = int(m.group(1))
    m = re.search(r'ROB Full Stalls:\s+(\d+)', log_text)
    if m: stats['rob_stalls'] = int(m.group(1))
    m = re.search(r'PRF Exhaustion Stalls:\s+(\d+)', log_text)
    if m: stats['prf_stalls'] = int(m.group(1))
    m = re.search(r'Head-of-ROB Stalls:\s+(\d+)', log_text)
    if m: stats['head_rob_stalls'] = int(m.group(1))
    m = re.search(r'Retiring:\s*([\d\.]+)%', log_text)
    if m: stats['retiring'] = float(m.group(1))
    m = re.search(r'Bad Speculation:\s*([\d\.]+)%', log_text)
    if m: stats['bad_spec'] = float(m.group(1))
    m = re.search(r'Frontend Bound:\s*([\d\.]+)%', log_text)
    if m: stats['frontend_bound'] = float(m.group(1))
    m = re.search(r'Backend Bound:\s*([\d\.]+)%', log_text)
    if m: stats['backend_bound'] = float(m.group(1))
    m = re.search(r'L1D MSHR Allocations:\s+\d+\s+\(Stalls:\s*(\d+)\)', log_text)
    if m: stats['l1d_mshr_stalls'] = int(m.group(1))
    return stats

def parse_baseline_file(file_path):
    if not os.path.exists(file_path):
        return {}
    results = {}
    with open(file_path, 'r') as fp:
        text = fp.read()
    results['stats'] = parse_uarch_stats(text)
    return results

def run_single_suite(suite_key, sim_bin, active_cfg):
    suite_info = UBENCH_SUITES[suite_key]
    bin_path = os.path.join(BUILD_DIR, suite_info["binary"])
    
    if not os.path.exists(bin_path):
        print(f"Building {suite_info['binary']}...")
        subprocess.run([CMAKE_BIN, "--build", BUILD_DIR, "--target", suite_info["binary"], "-j4"], check=True)

    start_time = datetime.datetime.now()
    res = subprocess.run([bin_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed_ms = int((datetime.datetime.now() - start_time).total_seconds() * 1000)
    passed_tests = len(re.findall(r'\[\s+OK\s+\]', res.stdout))
    total_tests = len(suite_info["tests"])

    workload_elf = os.path.join(FIXTURES_DIR, suite_info["workload_elf"])
    sim_stats = {}
    sim_log = ""
    if os.path.exists(sim_bin) and os.path.exists(workload_elf) and os.path.exists(active_cfg):
        sim_res = subprocess.run([sim_bin, "--uarch", "--uarch-config", active_cfg, "--all-perf", workload_elf],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        sim_log = sim_res.stdout
        sim_stats = parse_uarch_stats(sim_log)

    return {
        "suite_key": suite_key,
        "name": suite_info["name"],
        "description": suite_info["description"],
        "passed": passed_tests,
        "total": total_tests,
        "elapsed_ms": elapsed_ms,
        "gtest_output": res.stdout,
        "sim_stats": sim_stats,
        "sim_log": sim_log,
        "workload_elf": suite_info["workload_elf"],
        "tests": suite_info["tests"],
        "sensitive_knobs": suite_info["sensitive_knobs"]
    }

def format_suite_report_file(suite_res, active_cfg, changed_knobs, base_stats=None):
    now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "================================================================================",
        "             TinyCpuSim Microbenchmark Performance & Diagnostic Report          ",
        "================================================================================",
        f"  Suite Component:       {suite_res['name']} ({suite_res['suite_key']})",
        f"  Generation Timestamp:  {now_str}",
        f"  Applied Configuration: {active_cfg}",
        f"  Representative ELF:    {suite_res['workload_elf']}",
        f"  Test Pass Count:       {suite_res['passed']} / {suite_res['total']}",
    ]
    if changed_knobs:
        lines.append("  Active Hardware Parameter Modifications (vs Baseline):")
        for sec, k, v_b, v_a in changed_knobs:
            lines.append(f"    • [{sec}] {k}: Baseline ({v_b}) ➔ Active ({v_a})")
    lines.append("--------------------------------------------------------------------------------")
    lines.append("  Top Microarchitecture Hardware Metrics:")
    for k, v in suite_res['sim_stats'].items():
        lines.append(f"    • {k}: {v}")
    lines.append("--------------------------------------------------------------------------------")
    lines.append("  Detailed Microbenchmark Test Cases:")
    for t_name, t_desc, m_label, m_val, m_unit, direction in suite_res["tests"]:
        tag = "[Optimal]" if direction == "high" else "[Stress/Bottleneck]"
        lines.append(f"  • {t_name:<46} | {m_label}: {m_val}{m_unit:<8} | {tag} {t_desc}")
    lines.append("--------------------------------------------------------------------------------\n")
    lines.append(suite_res['sim_log'])
    return "\n".join(lines)

def interactive_menu():
    print("============================================================")
    print("           TinyCpuSim Microbenchmark Suite Selection        ")
    print("============================================================")
    print("Please select a microbenchmark suite to evaluate:")
    print("  [1] All Subsystems (Run all 41 microbenchmarks across full CPU)")
    print("  [2] BPU & Frontend (Branch prediction, TAGE, BTB, RAS, Fetch, PRF)")
    print("  [3] Execution & LSU (ALU/MUL/DIV latency, Issue width, LSQ Forwarding)")
    print("  [4] ROB & Top-Down (ROB retirement, Head-of-ROB blocking, TMAM slots)")
    print("  [5] Cache & Memory (L1I/L1D hit latency, MSHR non-blocking, MESI)")
    print("  [0] Cancel")
    print("============================================================")
    choice = input("Enter choice [1-5, default: 1]: ").strip()
    if choice in ["2", "bpu"]: return "bpu"
    elif choice in ["3", "exec"]: return "exec"
    elif choice in ["4", "rob"]: return "rob"
    elif choice in ["5", "cache"]: return "cache"
    elif choice in ["0", "q", "cancel"]: sys.exit(0)
    return "all"

def main():
    parser = argparse.ArgumentParser(
        description="TinyCpuSim Microbenchmark Performance Suite & Configuration Sensitivity Engine",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("suite", nargs="?", help="Target suite: [all|bpu|exec|rob|cache]")
    parser.add_argument("--config", "-c", help="Configuration file to test (defaults to configs/current.cfg)")
    default_jobs = max(1, (os.cpu_count() or 4) // 2)
    parser.add_argument("-j", "--jobs", type=int, default=default_jobs, help=f"Parallel worker threads/processes (default: {default_jobs}, half of CPU cores)")
    args = parser.parse_args()

    os.makedirs(REPORTS_DIR, exist_ok=True)
    os.makedirs(DEFAULT_REPORTS_DIR, exist_ok=True)

    sim_bin = os.path.join(BUILD_DIR, "tinycpusim")
    active_cfg = args.config if args.config else (CURRENT_CFG if os.path.exists(CURRENT_CFG) else DEFAULT_CFG)
    changed_knobs = diff_hardware_configs(DEFAULT_CFG, active_cfg)

    selected_suite = args.suite
    if not selected_suite:
        if sys.stdin.isatty():
            selected_suite = interactive_menu()
        else:
            selected_suite = "all"

    selected_suite = selected_suite.lower().strip()
    if selected_suite in ["1", "all", "full"]: selected_suite = "all"
    elif selected_suite in ["2", "bpu"]: selected_suite = "bpu"
    elif selected_suite in ["3", "exec"]: selected_suite = "exec"
    elif selected_suite in ["4", "rob"]: selected_suite = "rob"
    elif selected_suite in ["5", "cache"]: selected_suite = "cache"

    suites_to_run = list(UBENCH_SUITES.keys()) if selected_suite == "all" else [selected_suite]
    if selected_suite != "all" and selected_suite not in UBENCH_SUITES:
        print(f"Error: Unknown microbenchmark suite '{selected_suite}'. Available: all, bpu, exec, rob, cache")
        sys.exit(1)

    print("\n" + "=" * 88)
    print("         TinyCpuSim Microbenchmark Performance & Configuration Sensitivity Suite        ")
    print("=" * 88)
    print(f"  Applied Active Config:   {active_cfg}")
    print(f"  Parallel Workers:        {args.jobs} (half of CPU cores)")
    if changed_knobs:
        print("  Active Hardware Parameter Modifications (vs Baseline):")
        for sec, k, v_b, v_a in changed_knobs:
            matched_suites = [s['name'] for s in UBENCH_SUITES.values() if k in s['sensitive_knobs']]
            target_scope = f"  \033[2m[Affects: {', '.join(matched_suites)}]\033[0m" if matched_suites else ""
            print(f"    • [{sec}] {k}: Baseline ({v_b}) ➔ Active Run (\033[1;32m{v_a}\033[0m){target_scope}")
    else:
        print("  Active Hardware Parameters: (Matches Baseline Default Configuration)")
    print("-" * 88)

    import concurrent.futures

    # Run each requested suite (in parallel if multiple)
    if args.jobs > 1 and len(suites_to_run) > 1:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
            all_results = list(executor.map(lambda s_key: run_single_suite(s_key, sim_bin, active_cfg), suites_to_run))
    else:
        all_results = [run_single_suite(s_key, sim_bin, active_cfg) for s_key in suites_to_run]

    generated_reports = []
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    for suite_res in all_results:
        s_key = suite_res["suite_key"]

        base_file = os.path.join(DEFAULT_REPORTS_DIR, f"{s_key}.txt")
        baseline_data = parse_baseline_file(base_file)

        # Write individual suite report
        out_report_path = os.path.join(REPORTS_DIR, f"ubench_{s_key}_{timestamp}.txt")
        report_content = format_suite_report_file(suite_res, active_cfg, changed_knobs, baseline_data.get('stats'))
        with open(out_report_path, 'w') as fp:
            fp.write(report_content)
        generated_reports.append(out_report_path)

        # Display Suite Performance Table
        passed = suite_res["passed"]
        total = suite_res["total"]
        sim_stats = suite_res["sim_stats"]
        base_stats = baseline_data.get('stats', {})
        has_baseline = bool(baseline_data)

        pass_tag = "\033[1;32m100% HEALTHY\033[0m" if passed == total else f"\033[1;31m{passed}/{total} PASSED\033[0m"

        print(f"\n========================================================================================")
        print(f" [ Suite: \033[1m{suite_res['name']}\033[0m ] - {pass_tag} ({passed}/{total} Tests) | Workload: \033[1;36m{suite_res['workload_elf']}\033[0m")
        print("========================================================================================")

        # Print Hardware Metrics Comparison Table
        print(f"    {'Hardware Performance Metric':<34} | {'Baseline':<13} | {'Active Run':<13} | {'Delta (%)':<12}")
        print("    " + "-" * 78)
        suite_metric_defs = SUITE_METRICS.get(s_key, [])
        for label, m_key, unit, lower_better in suite_metric_defs:
            v_b = base_stats.get(m_key, 'N/A')
            v_e = sim_stats.get(m_key, 'N/A')
            print(format_row(label, v_b, v_e, unit=unit, lower_better=lower_better))

        # Check for degraded metrics in this suite
        degraded_list = []
        for label, m_key, unit, lower_better in suite_metric_defs:
            v_b = base_stats.get(m_key)
            v_e = sim_stats.get(m_key)
            if isinstance(v_b, (int, float)) and isinstance(v_e, (int, float)):
                if lower_better and v_e > v_b and abs(v_e - v_b) > 0.001:
                    diff = v_e - v_b
                    pct_str = f"+{((v_e - v_b) / float(v_b) * 100.0):.1f}%" if v_b > 0 else f"+{diff:.2f}{unit}"
                    degraded_list.append((label, v_b, v_e, diff, pct_str, unit, "worsened / increased"))
                elif not lower_better and v_e < v_b and abs(v_b - v_e) > 0.001:
                    diff = v_b - v_e
                    pct_str = f"-{((v_b - v_e) / float(v_b) * 100.0):.1f}%" if v_b > 0 else f"-{diff:.2f}{unit}"
                    degraded_list.append((label, v_b, v_e, diff, pct_str, unit, "worsened / dropped"))

        print("    " + "-" * 78)
        if degraded_list:
            print("    \033[1;31m⚠️  DEGRADED HARDWARE METRICS (Regressed vs Baseline):\033[0m")
            for label, v_b, v_e, diff, pct_str, unit, action in degraded_list:
                if "IPC" in label and isinstance(v_b, float):
                    fmt_b = f"{v_b:.3f}"
                    fmt_e = f"{v_e:.3f}"
                else:
                    fmt_b = f"{v_b:.2f}{unit}" if isinstance(v_b, float) else f"{v_b}{unit}"
                    fmt_e = f"{v_e:.2f}{unit}" if isinstance(v_e, float) else f"{v_e}{unit}"
                print(f"      • \033[1;31m{label}\033[0m: Baseline {fmt_b} ➔ Active \033[1;31m{fmt_e}\033[0m ({action}: \033[1;31m{pct_str}\033[0m)")
        else:
            print("    \033[1;32m🟢 HARDWARE METRIC HEALTH: Optimal (0.0% regression vs baseline)\033[0m")

        print("    " + "-" * 78)

        # Rank Top Performers vs Bottlenecks for this suite
        top_tests = [t for t in suite_res["tests"] if t[5] == "high"]
        stress_tests = [t for t in suite_res["tests"] if t[5] == "low"]

        print("    \033[1;32m🟢 TOP MICROBENCHMARKS (Highest Subsystem Efficiency):\033[0m")
        for idx, (t_name, t_desc, m_label, m_val, m_unit, _) in enumerate(top_tests[:2], 1):
            print(f"      {idx}. \033[1m{t_name}\033[0m: {m_label} = \033[1;32m{m_val}{m_unit}\033[0m ({t_desc})")

        print("    \033[1;31m🔴 CRITICAL BOTTLENECK & STRESS TESTS (Worst Performers / Highest Stalls):\033[0m")
        for idx, (t_name, t_desc, m_label, m_val, m_unit, _) in enumerate(stress_tests[:2], 1):
            print(f"      {idx}. \033[1m{t_name}\033[0m: {m_label} = \033[1;31m{m_val}{m_unit}\033[0m ({t_desc})")

    # =========================================================================
    # Global Sensitivity & Cross-Suite Performance Impact Summary
    # =========================================================================
    print("\n" + "=" * 88)
    print("  📊 CROSS-SUITE PERFORMANCE IMPACT & ATTRIBUTION RANKING:")
    print("=" * 88)

    if changed_knobs:
        # Rank which workload had best speedup and which had worst regression
        ranked_suites = []
        for s in all_results:
            base_f = os.path.join(DEFAULT_REPORTS_DIR, f"{s['suite_key']}.txt")
            b_data = parse_baseline_file(base_f).get('stats', {})
            c_base = b_data.get('cycles', 0)
            c_exp = s['sim_stats'].get('cycles', 0)
            if c_base > 0 and c_exp > 0:
                speedup = float(c_base) / float(c_exp)
                ranked_suites.append((s['name'], s['workload_elf'], speedup, c_base, c_exp))

        if ranked_suites:
            ranked_suites.sort(key=lambda x: x[2], reverse=True)
            best = ranked_suites[0]
            worst = ranked_suites[-1]

            has_gain = best[2] > 1.0001
            has_loss = worst[2] < 0.9999

            if has_gain:
                gain = (best[2] - 1.0) * 100.0
                print("  🏆 \033[1;32mGREATEST PERFORMANCE IMPROVEMENT:\033[0m")
                print(f"    • \033[1m{best[0]}\033[0m ({best[1]}): \033[1;32m+{gain:.1f}% FASTER (Speedup: {best[2]:.2f}x)\033[0m")
                print(f"      Baseline: {best[3]} cycles ➔ Active Run: {best[4]} cycles\n")

            if has_loss:
                loss = (1.0 - worst[2]) * 100.0
                print("  ⚠️  \033[1;31mGREATEST BOTTLENECK / PERFORMANCE REGRESSION:\033[0m")
                print(f"    • \033[1m{worst[0]}\033[0m ({worst[1]}): \033[1;31m{loss:.1f}% SLOWER (Slowdown: {(1.0/worst[2]):.2f}x)\033[0m")
                print(f"      Baseline: {worst[3]} cycles ➔ Active Run: {worst[4]} cycles\n")

            if not has_gain and not has_loss:
                print("  ℹ️  \033[1mNeutral Sensitivity\033[0m: All evaluated workloads ran within ±0.01% of baseline timing.")
    else:
        print("  (All hardware parameters match baseline default; no sensitivity delta detected)")

    # Save summary report file
    summary_path = os.path.join(REPORTS_DIR, f"summary_{timestamp}.txt")
    with open(summary_path, 'w') as fp:
        fp.write("================================================================================\n")
        fp.write("              TinyCpuSim Microbenchmark Performance Summary Report              \n")
        fp.write("================================================================================\n")
        fp.write(f"Generation Timestamp:  {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        fp.write(f"Applied Configuration: {active_cfg}\n")
        fp.write("Active Modifications (vs Baseline):\n")
        if changed_knobs:
            for sec, k, v_b, v_a in changed_knobs:
                fp.write(f"  • [{sec}] {k}: Baseline ({v_b}) ➔ Active ({v_a})\n")
        else:
            fp.write("  • (No modifications; matches default baseline)\n")
        fp.write("--------------------------------------------------------------------------------\n")
        fp.write("Evaluated Suite Reports:\n")
        for rep in generated_reports:
            fp.write(f"  • {rep}\n")
    generated_reports.append(summary_path)

    print("=" * 88)
    print("  📁 Generated Microbenchmark Reports in \033[1mreports/ubench/\033[0m:")
    for rep_file in generated_reports:
        print(f"    • \033[1;36m{rep_file}\033[0m")
    print("=" * 88)

if __name__ == '__main__':
    main()
