#!/usr/bin/python3
"""
gem5 Configuration Extractor & TinySim Configuration Generator (scripts/gem5_to_tinyconfig.py)
Parses gem5 config.ini / config.json from a simulated golden run and generates
a fully matched TinySim microarchitecture configuration (.cfg file).
"""

import os
import sys
import argparse
import configparser

def parse_gem5_config_ini(config_ini_path):
    if not os.path.exists(config_ini_path):
        raise FileNotFoundError(f"gem5 config.ini not found at: {config_ini_path}")

    config = configparser.ConfigParser()
    config.read(config_ini_path)

    extracted = {}

    # 1. CPU Core Parameters
    cpu_sec = 'system.cpu_cluster.cpus'
    if cpu_sec in config:
        extracted['fetch_width'] = int(config[cpu_sec].get('fetchWidth', 3))
        extracted['decode_width'] = int(config[cpu_sec].get('decodeWidth', 3))
        extracted['rename_width'] = int(config[cpu_sec].get('renameWidth', 3))
        extracted['dispatch_width'] = int(config[cpu_sec].get('dispatchWidth', 6))
        extracted['issue_width'] = int(config[cpu_sec].get('issueWidth', 8))
        extracted['commit_width'] = int(config[cpu_sec].get('commitWidth', 8))
        extracted['rob_size'] = int(config[cpu_sec].get('numROBEntries', 40))
        extracted['num_phys_regs'] = int(config[cpu_sec].get('numPhysIntRegs', 128))
        extracted['lq_size'] = int(config[cpu_sec].get('LQEntries', 16))
        extracted['sq_size'] = int(config[cpu_sec].get('SQEntries', 16))
    else:
        # Fallbacks
        extracted['fetch_width'] = 3
        extracted['decode_width'] = 3
        extracted['rename_width'] = 3
        extracted['issue_width'] = 8
        extracted['commit_width'] = 8
        extracted['rob_size'] = 40
        extracted['num_phys_regs'] = 128
        extracted['lq_size'] = 16
        extracted['sq_size'] = 16

    # 2. Instruction Queue (RS) Size
    iq_sec = 'system.cpu_cluster.cpus.instQueues'
    if iq_sec in config:
        extracted['rs_size'] = int(config[iq_sec].get('numEntries', 32))
    else:
        extracted['rs_size'] = 32

    # 3. Branch Predictor
    bp_sec = 'system.cpu_cluster.cpus.branchPred'
    cond_sec = 'system.cpu_cluster.cpus.branchPred.conditionalBranchPred'
    btb_sec = 'system.cpu_cluster.cpus.branchPred.btb'
    ras_sec = 'system.cpu_cluster.cpus.branchPred.ras'

    extracted['bp_type'] = 'BIMODE' if (cond_sec in config and 'BiModeBP' in config[cond_sec].get('type', '')) else 'TAGE'
    if cond_sec in config:
        extracted['bp_table_size'] = int(config[cond_sec].get('globalPredictorSize', 8192))
    else:
        extracted['bp_table_size'] = 8192

    if btb_sec in config:
        extracted['btb_size'] = int(config[btb_sec].get('numEntries', 2048))
    else:
        extracted['btb_size'] = 2048

    if ras_sec in config:
        extracted['ras_size'] = int(config[ras_sec].get('numEntries', 16))
    else:
        extracted['ras_size'] = 16

    # 4. L1 Instruction Cache
    icache_sec = 'system.cpu_cluster.cpus.icache'
    if icache_sec in config:
        extracted['l1i_size'] = int(config[icache_sec].get('size', 32768))
        extracted['l1i_assoc'] = int(config[icache_sec].get('assoc', 2))
        extracted['l1i_latency'] = int(config[icache_sec].get('tag_latency', 1))
        extracted['l1i_mshrs'] = int(config[icache_sec].get('mshrs', 2))
    else:
        extracted['l1i_size'] = 32768
        extracted['l1i_assoc'] = 2
        extracted['l1i_latency'] = 1
        extracted['l1i_mshrs'] = 2

    # 5. L1 Data Cache
    dcache_sec = 'system.cpu_cluster.cpus.dcache'
    if dcache_sec in config:
        extracted['l1d_size'] = int(config[dcache_sec].get('size', 32768))
        extracted['l1d_assoc'] = int(config[dcache_sec].get('assoc', 2))
        extracted['l1d_latency'] = int(config[dcache_sec].get('tag_latency', 2))
        extracted['l1d_mshrs'] = int(config[dcache_sec].get('mshrs', 6))
    else:
        extracted['l1d_size'] = 32768
        extracted['l1d_assoc'] = 2
        extracted['l1d_latency'] = 2
        extracted['l1d_mshrs'] = 6

    # 6. L2 Cache
    l2_sec = 'system.cpu_cluster.l2'
    if l2_sec in config:
        extracted['l2_size'] = int(config[l2_sec].get('size', 1048576))
        extracted['l2_assoc'] = int(config[l2_sec].get('assoc', 16))
        extracted['l2_latency'] = int(config[l2_sec].get('tag_latency', 12))
        extracted['l2_mshrs'] = int(config[l2_sec].get('mshrs', 16))
    else:
        extracted['l2_size'] = 1048576
        extracted['l2_assoc'] = 16
        extracted['l2_latency'] = 12
        extracted['l2_mshrs'] = 16

    return extracted

def generate_tinysim_cfg(extracted_params, output_cfg_path, elf_path=None):
    os.makedirs(os.path.dirname(os.path.abspath(output_cfg_path)), exist_ok=True)
    target_elf = elf_path if elf_path else "tests/fixtures/test_branch_pred.elf"

    cfg_content = f"""# ==============================================================================
# TinyCpuSim gem5 O3 Calibrated Microarchitecture Configuration
# Auto-generated from gem5 config.ini
# ==============================================================================

[simulation]
elf_path = {target_elf}
mode = uarch
max_steps = 1000000000
all_perf = true
enable_topdown = true

[system]
num_cores = 1
dram_latency_cycles = 80
coherence = MESI

[core]
type = OOO_DYNAMIC
fetch_width = {extracted_params['fetch_width']}
decode_width = {extracted_params['decode_width']}
rename_width = {extracted_params['rename_width']}
issue_width = {extracted_params['issue_width']}
commit_width = {extracted_params['commit_width']}
rob_size = {extracted_params['rob_size']}
rs_size = {extracted_params['rs_size']}
num_phys_regs = {extracted_params['num_phys_regs']}

[branch_predictor]
type = {extracted_params['bp_type']}
table_size = {extracted_params['bp_table_size']}
btb_size = {extracted_params['btb_size']}
ras_size = {extracted_params['ras_size']}
tage_tables = 4

[lsu]
type = SPECULATIVE_OOO
lq_size = {extracted_params['lq_size']}
sq_size = {extracted_params['sq_size']}
store_forward_latency = 1

[l1i]
type = SET_ASSOCIATIVE
size_bytes = {extracted_params['l1i_size']}
line_size = 64
associativity = {extracted_params['l1i_assoc']}
hit_latency_cycles = {extracted_params['l1i_latency']}
mshr_entries = {extracted_params['l1i_mshrs']}

[l1d]
type = SET_ASSOCIATIVE
size_bytes = {extracted_params['l1d_size']}
line_size = 64
associativity = {extracted_params['l1d_assoc']}
hit_latency_cycles = {extracted_params['l1d_latency']}
mshr_entries = {extracted_params['l1d_mshrs']}

[l2]
type = SET_ASSOCIATIVE
size_bytes = {extracted_params['l2_size']}
line_size = 64
associativity = {extracted_params['l2_assoc']}
hit_latency_cycles = {extracted_params['l2_latency']}
mshr_entries = {extracted_params['l2_mshrs']}
"""
    with open(output_cfg_path, 'w') as fp:
        fp.write(cfg_content)
    print(f"TinySim config successfully written to: {output_cfg_path}")

def main():
    parser = argparse.ArgumentParser(description="Extract gem5 config.ini into TinySim .cfg format")
    parser.add_argument("--gem5-ini", default="/tmp/gem5_golden_test_branch_pred/config.ini", help="Path to gem5 config.ini")
    parser.add_argument("--out-cfg", default="configs/default/gem5_o3.cfg", help="Destination path for generated TinySim .cfg")
    parser.add_argument("--elf", default="tests/fixtures/test_branch_pred.elf", help="Target ELF path")
    args = parser.parse_args()

    print(f"Reading gem5 config from: {args.gem5_ini}")
    extracted = parse_gem5_config_ini(args.gem5_ini)
    generate_tinysim_cfg(extracted, args.out_cfg, args.elf)

if __name__ == "__main__":
    main()
