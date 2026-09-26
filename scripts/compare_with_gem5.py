#!/usr/bin/env python3
"""
TinyArmSim vs gem5 Golden Reference Accuracy Comparator
Parses TinyArmSim performance report and gem5 stats.txt, calculating precision deltas.
"""

import sys
import re
import os

def parse_tinysim_stats(filename):
    stats = {}
    if not os.path.exists(filename):
        return stats
    with open(filename, 'r') as f:
        content = f.read()
    
    m = re.search(r'Simulated Total Cycles:\s+(\d+)', content)
    if m: stats['cycles'] = int(m.group(1))
    
    m = re.search(r'Total Committed Insts:\s+(\d+)', content)
    if m: stats['insts'] = int(m.group(1))
    
    m = re.search(r'Aggregate Throughput \(IPC\):\s*([\d\.]+)', content)
    if m: stats['ipc'] = float(m.group(1))
    
    return stats

def parse_gem5_stats(filename):
    stats = {}
    if not os.path.exists(filename):
        return stats
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'): continue
            parts = line.split()
            if len(parts) >= 2:
                key, val = parts[0], parts[1]
                if key == 'simTicks':
                    # Assuming 1GHz clock (1000 ticks = 1 cycle)
                    try: stats['cycles'] = int(val) // 1000
                    except: pass
                elif key == 'simInsts' or key == 'system.cpu.committedInsts':
                    try: stats['insts'] = int(val)
                    except: pass
                elif key == 'system.cpu.ipc':
                    try: stats['ipc'] = float(val)
                    except: pass
    return stats

def main():
    if len(sys.argv) < 3:
        print("Usage: compare_with_gem5.py <tinysim_stats.txt> <gem5_stats.txt>")
        sys.exit(1)
    
    ts_file = sys.argv[1]
    g5_file = sys.argv[2]
    
    ts = parse_tinysim_stats(ts_file)
    g5 = parse_gem5_stats(g5_file)
    
    print("=" * 65)
    print("     TinyArmSim vs gem5 Golden Precision Comparison Report     ")
    print("=" * 65)
    print(f"{'Metric':<25} | {'TinyArmSim':<12} | {'gem5 Golden':<12} | {'Delta (%)':<10}")
    print("-" * 65)
    
    metrics = ['cycles', 'insts', 'ipc']
    for m in metrics:
        v_ts = ts.get(m, 'N/A')
        v_g5 = g5.get(m, 'N/A')
        delta_str = 'N/A'
        if isinstance(v_ts, (int, float)) and isinstance(v_g5, (int, float)) and v_g5 > 0:
            delta = abs(v_ts - v_g5) / float(v_g5) * 100.0
            delta_str = f"{delta:.2f}%"
        
        print(f"{m:<25} | {str(v_ts):<12} | {str(v_g5):<12} | {delta_str:<10}")
    
    print("=" * 65)

if __name__ == '__main__':
    main()
