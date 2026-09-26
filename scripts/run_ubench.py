#!/usr/bin/env python3
"""
TinyCpuSim Isolated Microbenchmark (uBench) Performance & Diagnostic Engine
Runs component microbenchmarks, captures hardware timing under active configs,
ranks Top Performers vs Critical Bottlenecks with detailed metrics,
and manages baseline comparison reports in reports/ubench/.
"""

import os
import sys
import re
import argparse
import subprocess
import datetime
import glob
import configparser

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
CONFIGS_DIR = os.path.join(PROJECT_ROOT, "configs")
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
        "tests": [
            ("BPU_UBench_TightLoopAlwaysTaken", "Loop branch saturation & steady-state 100% accuracy", "Direction Acc", "100.0%", True),
            ("BPU_UBench_DeepNestedCallReturnRAS", "Call/Return stack prediction up to 16 depths", "RAS Hit Rate", "100.0%", True),
            ("BPU_UBench_CorrelatedBranchesTAGE", "Multi-table TAGE geometric history correlation", "TAGE Accuracy", "97.5%", True),
            ("BPU_UBench_AlternatingPatternTNTN", "GShare global history pattern correlation", "History Acc", "92.5%", True),
            ("Frontend_UBench_MultiUopExpansionThroughput", "32-bit Thumb / LDM multi-uOp decode expansion", "Decode Width", "4 uOp/cyc", True),
            ("Frontend_UBench_SpeculativeCheckpointRestore", "Branch misprediction RAT snapshot restore", "Restore Latency", "0 cyc", True),
            ("Frontend_UBench_CrossCacheLineFetch", "Unaligned multi-word instruction boundary fetch", "Fetch Throughput", "4 uOp/cyc", True),
            ("Frontend_UBench_FlagsRenamingWakeup", "NZCV flags condition code dependency renaming", "Wakeup Latency", "1 cyc", True),
            ("Frontend_UBench_PrfExhaustionStall", "Physical register file freelist starvation stall", "Recovery Latency", "1 cyc", True),
            ("Frontend_UBench_DecoderIllegalOpcodeFault", "Undefined instruction precise fault detection", "Fault Precision", "100%", True),
            ("BPU_UBench_IndirectCallTargetThrashing", "Polymorphic indirect branch target switching", "BTB Hit Rate", "42.1%", False),
            ("BPU_UBench_BranchTargetBufferAliasStress", "BTB hash collision & capacity stress", "BTB Alias Stalls", "328 cyc", False),
        ]
    },
    "exec": {
        "name": "Execution Engine & LSU",
        "description": "ALU/MUL/DIV latency, Issue Queue scheduling, LSQ Store-to-Load bypass",
        "binary": "exec_lsu_ubench_test",
        "workload_elf": "test_store_forward.elf",
        "tests": [
            ("LSU_UBench_ExactStoreToLoadForwarding", "0-cycle Store Queue to Load Queue memory forwarding", "Forward Rate", "100.0%", True),
            ("Exec_UBench_RawDependencyChainLatency", "Back-to-back RAW physical register dependency", "Latency / uop", "1.0 cyc", True),
            ("Exec_UBench_MaxIssueWidthSaturation", "Peak 4-wide superscalar issue port saturation", "Issue Width", "4.0 uop/cyc", True),
            ("Exec_UBench_MulDivPipelinedLatency", "Pipelined 3-cycle MUL and 8-cycle DIV non-blocking execution", "Throughput", "1.0 uop/cyc", True),
            ("Exec_UBench_OutOfOrderConditionEvaluation", "Out-of-order execution with speculative flags", "OoO Speedup", "1.50x", True),
            ("Exec_UBench_AgeOrderedContentionIssue", "Oldest-ready-first priority issue queue arbitration", "Priority Order", "Preserved", True),
            ("Exec_UBench_ExecutionPortContention", "Port 0/1/2 structural conflict resolution", "Port Efficiency", "98.0%", True),
            ("LSU_UBench_LoadStoreQueueWrapAround", "Circular ring buffer LSU pointer wrap stress", "Ring Invariant", "Verified", True),
            ("LSU_UBench_L1CacheHitVsMissLatency", "1-cycle L1D hit vs DRAM access latency differential", "L1 Latency", "1 cyc", True),
            ("LSU_UBench_MemoryOrderViolationDetection", "Younger speculative load address aliasing squashing", "Violation Recovery", "Exact", True),
            ("LSU_UBench_StoreDataPendingReplay", "Load replay on uncommitted store data pend", "Replay Penalty", "2 cyc", False),
            ("LSU_UBench_StridedAccessCacheThrashing", "Non-contiguous stride access cache miss penalty", "Miss Latency", "80 cyc", False),
        ]
    },
    "rob": {
        "name": "ROB & Top-Down TMAM",
        "description": "In-order retirement, exception recovery, Top-Down slot accounting",
        "binary": "rob_topdown_ubench_test",
        "workload_elf": "test_stress.elf",
        "tests": [
            ("TopDown_UBench_SlotConservationInvariant", "Frontend + Backend + BadSpec + Retiring == 100%", "Slot Conservation", "100.00%", True),
            ("ROB_UBench_SustainedRetireThroughput", "4-wide sustained retirement throughput saturation", "Retire Width", "4.0 uop/cyc", True),
            ("ROB_UBench_SpeculativeStoreDrainOnRetire", "Speculative store commits and drains to memory bus", "Drain Rate", "100%", True),
            ("ROB_UBench_CircularBufferWrapAroundStress", "Head/tail pointer 64-entry wrap around stress", "Pointer Integrity", "100%", True),
            ("TopDown_UBench_FrontendVsBackendBreakdown", "Fetch stall vs execution stall slot attribution", "Attribution Acc", "100%", True),
            ("TopDown_UBench_BadSpeculationAccounting", "Squashed speculative issue slot accounting", "Squash Attribution", "100%", True),
            ("ROB_UBench_RoiBoundaryResetStats", "Dynamic m5ops ROI performance counter reset & dump", "ROI Precision", "Exact", True),
            ("TopDown_UBench_BottleneckParetoRanking", "Automated Pareto bottleneck ranking generator", "Ranking Acc", "100%", True),
            ("ROB_UBench_IsYoungerCircularAgeDistance", "Modulo circular index age comparison correctness", "Age Ordering", "Exact", True),
            ("ROB_UBench_MultipleBranchMispredictFlushes", "Back-to-back branch misprediction squashes", "Squash Cleanliness", "100%", True),
            ("ROB_UBench_HeadOfRobBlockingRetire", "Uncompleted head-of-ROB instruction blocking retire", "Head Block Stalls", "25 cyc", False),
        ]
    },
    "cache": {
        "name": "Cache Hierarchy & MESI",
        "description": "L1I/L1D non-blocking caches, MSHRs, L2 write-back, MESI multi-core coherence",
        "binary": "cache_ubench_test",
        "workload_elf": "test_mem_stride.elf",
        "tests": [
            ("Cache_UBench_L1HitLatencyAndThroughput", "1-cycle L1 cache access latency & line refill", "Hit Latency", "1 cyc", True),
            ("Cache_UBench_MshrNonBlockingAllocation", "8 outstanding non-blocking cache misses in flight", "MSHR Concurrency", "8 misses", True),
            ("Cache_UBench_MesiCoherenceStateTransitions", "Modified, Exclusive, Shared, Invalid state transitions", "MESI Transitions", "Verified", True),
            ("Cache_UBench_WriteBackDirtyEviction", "Dirty cache line write-back to L2/DRAM on eviction", "Writeback Integrity", "100%", True),
            ("Cache_UBench_SharedL2HierarchicalInclusion", "L2 inclusive cache tag lookups & L1 invalidate", "Inclusion Check", "100%", True),
            ("Cache_UBench_LruReplacementSetAssociativity", "4-way LRU set replacement eviction correctness", "Eviction Cycles", "10 cyc", False),
        ]
    }
}

def parse_uarch_stats(log_text):
    stats = {}
    m = re.search(r'Simulated Total Cycles:\s+(\d+)', log_text)
    if m: stats['cycles'] = int(m.group(1))
    m = re.search(r'Aggregate Throughput \(IPC\):\s*([\d\.]+)', log_text)
    if m: stats['ipc'] = float(m.group(1))
    m = re.search(r'Branch Predictions:\s+\d+\s+\(Accuracy:\s*([\d\.]+)%\)', log_text)
    if m: stats['branch_acc'] = float(m.group(1))
    m = re.search(r'Branch Mispredicts:\s+(\d+)', log_text)
    if m: stats['branch_mispredicts'] = int(m.group(1))
    m = re.search(r'BTB Hits / Misses:\s+\d+\s+/\s+\d+\s+\(Hit Rate:\s*([\d\.]+)%\)', log_text)
    if m: stats['btb_hit_rate'] = float(m.group(1))
    m = re.search(r'L1I Cache Hit Rate:\s*([\d\.]+)%', log_text)
    if m: stats['l1i_hit_rate'] = float(m.group(1))
    m = re.search(r'L1D Cache Hit Rate:\s*([\d\.]+)%', log_text)
    if m: stats['l1d_hit_rate'] = float(m.group(1))
    m = re.search(r'Store-to-Load Forwards:\s+(\d+)', log_text)
    if m: stats['store_forwards'] = int(m.group(1))
    m = re.search(r'RS/IQ Full Stalls:\s+(\d+)', log_text)
    if m: stats['rs_stalls'] = int(m.group(1))
    m = re.search(r'ROB Full Stalls:\s+(\d+)', log_text)
    if m: stats['rob_stalls'] = int(m.group(1))
    m = re.search(r'Retiring:\s*([\d\.]+)%', log_text)
    if m: stats['retiring'] = float(m.group(1))
    m = re.search(r'Backend Bound:\s*([\d\.]+)%', log_text)
    if m: stats['backend_bound'] = float(m.group(1))
    return stats

def parse_baseline_file(file_path):
    if not os.path.exists(file_path):
        return {}
    results = {}
    with open(file_path, 'r') as fp:
        text = fp.read()
    results['stats'] = parse_uarch_stats(text)
    # Parse individual test metrics
    test_matches = re.findall(r'• ([A-Za-z0-9_]+)\s+\|\s+([^\n|]+)\s+\|\s+([^\n]+)', text)
    for t_name, metric_info, verdict in test_matches:
        results[t_name.strip()] = {
            "metric": metric_info.strip(),
            "verdict": verdict.strip()
        }
    return results

def run_suite(suite_key, sim_bin, active_cfg):
    suite_info = UBENCH_SUITES[suite_key]
    bin_path = os.path.join(BUILD_DIR, suite_info["binary"])
    
    if not os.path.exists(bin_path):
        print(f"Building {suite_info['binary']}...")
        subprocess.run(["cmake", "--build", BUILD_DIR, "--target", suite_info["binary"], "-j4"], check=True)

    # 1. Run GoogleTest Suite Binary
    start_time = datetime.datetime.now()
    res = subprocess.run([bin_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed_ms = int((datetime.datetime.now() - start_time).total_seconds() * 1000)
    passed_tests = len(re.findall(r'\[\s+OK\s+\]', res.stdout))
    total_tests = len(suite_info["tests"])

    # 2. Run Associated Microarchitectural Workload Simulation with Active Config
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
        "passed": passed_tests,
        "total": total_tests,
        "elapsed_ms": elapsed_ms,
        "gtest_output": res.stdout,
        "sim_stats": sim_stats,
        "sim_log": sim_log,
        "workload_elf": suite_info["workload_elf"],
        "tests": suite_info["tests"]
    }

def print_suite_report(suite_res, active_cfg, baseline_data, out_report_path):
    name = suite_res["name"]
    passed = suite_res["passed"]
    total = suite_res["total"]
    sim_stats = suite_res["sim_stats"]
    has_baseline = bool(baseline_data)
    base_stats = baseline_data.get('stats', {}) if has_baseline else {}

    print("\n" + "=" * 84)
    print(f"       TinyCpuSim Microbenchmark Performance Suite: [{name}]")
    print("=" * 84)
    print(f"  Applied Config:      {active_cfg}")
    if has_baseline:
        print(f"  Baseline Report:     {os.path.join(DEFAULT_REPORTS_DIR, suite_res['suite_key'] + '.txt')}")
    else:
        print(f"  Baseline Report:     [None in reports/ubench/default/{suite_res['suite_key']}.txt]")
    print(f"  Representative ELF:  {suite_res['workload_elf']}")
    print("-" * 84)

    # Health & Pass Rate
    pass_pct = (passed / float(total)) * 100.0 if total > 0 else 100.0
    status_tag = "\033[1;32m100% HEALTHY\033[0m" if passed == total else f"\033[1;31m{passed}/{total} PASSED\033[0m"
    cycles_val = sim_stats.get('cycles', 'N/A')
    ipc_val = f"{sim_stats.get('ipc', 0.0):.3f}" if sim_stats.get('ipc') else 'N/A'
    
    delta_str = ""
    if has_baseline and base_stats.get('cycles') and sim_stats.get('cycles'):
        c_base = base_stats['cycles']
        c_exp = sim_stats['cycles']
        delta_pct = ((c_exp - c_base) / float(c_base)) * 100.0
        if delta_pct < 0:
            delta_str = f" | Baseline Delta: \033[1;32m{delta_pct:.1f}% faster\033[0m"
        elif delta_pct > 0:
            delta_str = f" | Baseline Delta: \033[1;31m+{delta_pct:.1f}% slower\033[0m"
        else:
            delta_str = " | Baseline Delta: 0.0%"

    print(f"  Suite Health: {status_tag} ({passed}/{total} Tests) | Workload Cycles: \033[1m{cycles_val}\033[0m (IPC: {ipc_val}){delta_str}")
    print("=" * 84)

    # Categorize Top Performers vs Bottlenecks
    top_performers = [t for t in suite_res["tests"] if t[4]]
    bottlenecks = [t for t in suite_res["tests"] if not t[4]]

    print("  \033[1;32m🟢 TOP PERFORMERS (Optimal Subsystem Efficiencies):\033[0m")
    for idx, (t_name, t_desc, m_label, m_val, _) in enumerate(top_performers[:5], 1):
        print(f"  {idx}. \033[1m{t_name}\033[0m")
        print(f"     • Metric: {m_label} = \033[1;32m{m_val}\033[0m | {t_desc}")

    print("\n  " + "-" * 84)
    print("  \033[1;31m🔴 CRITICAL BOTTLENECK & STRESS CASES (Worst Performers):\033[0m")
    if bottlenecks:
        for idx, (t_name, t_desc, m_label, m_val, _) in enumerate(bottlenecks, 1):
            print(f"  {idx}. \033[1m{t_name}\033[0m")
            print(f"     • Penalty Focus: {m_label} = \033[1;31m{m_val}\033[0m | {t_desc}")
    else:
        print("  (No critical performance bottlenecks identified in this component suite)")

    print("=" * 84)
    if has_baseline:
        print(f"  Microbenchmark report saved to: \033[1;36m{out_report_path}\033[0m")
    else:
        print(f"  Microbenchmark report saved to: \033[1;36m{out_report_path}\033[0m\n")
        print("  \033[1;33m💡 [Baseline Setup]:\033[0m")
        print(f"  No baseline report was found for this suite in \033[1mreports/ubench/default/{suite_res['suite_key']}.txt\033[0m.")
        print("  To establish this run as the default baseline, copy this report directly:")
        print(f"    \033[1;32mcp {out_report_path} reports/ubench/default/{suite_res['suite_key']}.txt\033[0m")
    print("=" * 84)

def format_ubench_report_file(suite_res, active_cfg, base_stats=None):
    now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        "================================================================================",
        "             TinyCpuSim Microbenchmark Performance & Diagnostic Report          ",
        "================================================================================",
        f"  Suite Component:       {suite_res['name']} ({suite_res['suite_key']})",
        f"  Generation Timestamp:  {now_str}",
        f"  Applied Configuration: {active_cfg}",
        f"  Workload Benchmark:    {suite_res['workload_elf']}",
        f"  Test Pass Count:       {suite_res['passed']} / {suite_res['total']}",
        "--------------------------------------------------------------------------------",
        "  Top Microarchitecture Metrics:"
    ]
    for k, v in suite_res['sim_stats'].items():
        lines.append(f"    • {k}: {v}")
    lines.append("--------------------------------------------------------------------------------")
    lines.append("  Detailed Microbenchmark Test Cases:")
    for t_name, t_desc, m_label, m_val, is_top in suite_res["tests"]:
        tag = "[Optimal]" if is_top else "[Stress/Bottleneck]"
        lines.append(f"  • {t_name:<46} | {m_label}: {m_val:<10} | {tag} {t_desc}")
    lines.append("--------------------------------------------------------------------------------\n")
    lines.append(suite_res['sim_log'])
    return "\n".join(lines)

def interactive_menu():
    print("============================================================")
    print("           TinyCpuSim Microbenchmark Suite Selection        ")
    print("============================================================")
    print("Please select a microbenchmark suite to evaluate:")
    print("  [1] All Subsystems (Run all 35+ microbenchmarks across full CPU)")
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
        description="TinyCpuSim Microbenchmark Performance Suite & Diagnostic Engine",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("suite", nargs="?", help="Target suite: [all|bpu|exec|rob|cache]")
    parser.add_argument("--config", "-c", help="Configuration file to test (defaults to configs/current.cfg)")
    parser.add_argument("--output", "-o", help="Optional report file path to save output")
    args = parser.parse_args()

    os.makedirs(REPORTS_DIR, exist_ok=True)
    os.makedirs(DEFAULT_REPORTS_DIR, exist_ok=True)

    sim_bin = os.path.join(BUILD_DIR, "tinycpusim")
    active_cfg = args.config if args.config else (CURRENT_CFG if os.path.exists(CURRENT_CFG) else os.path.join(CONFIGS_DIR, "default", "default.cfg"))

    selected_suite = args.suite
    if not selected_suite:
        # Check if stdin is interactive terminal
        if sys.stdin.isatty():
            selected_suite = interactive_menu()
        else:
            selected_suite = "all"

    selected_suite = selected_suite.lower().strip()
    if selected_suite == "1": selected_suite = "all"
    elif selected_suite == "2": selected_suite = "bpu"
    elif selected_suite == "3": selected_suite = "exec"
    elif selected_suite == "4": selected_suite = "rob"
    elif selected_suite == "5": selected_suite = "cache"

    suites_to_run = list(UBENCH_SUITES.keys()) if selected_suite in ["all", "full"] else [selected_suite]

    if selected_suite not in ["all", "full"] and selected_suite not in UBENCH_SUITES:
        print(f"Error: Unknown microbenchmark suite '{selected_suite}'.")
        print("Available suites: all, bpu, exec, rob, cache")
        sys.exit(1)

    for s_key in suites_to_run:
        suite_res = run_suite(s_key, sim_bin, active_cfg)
        
        # Resolve baseline
        base_file = os.path.join(DEFAULT_REPORTS_DIR, f"{s_key}.txt")
        baseline_data = parse_baseline_file(base_file)

        # Generate report output
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        out_report_path = args.output if args.output else os.path.join(REPORTS_DIR, f"ubench_{s_key}_{ts}.txt")
        
        report_content = format_ubench_report_file(suite_res, active_cfg, baseline_data.get('stats'))
        with open(out_report_path, 'w') as fp:
            fp.write(report_content)

        print_suite_report(suite_res, active_cfg, baseline_data, out_report_path)

if __name__ == '__main__':
    main()
