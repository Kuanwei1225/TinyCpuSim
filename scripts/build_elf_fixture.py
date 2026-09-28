#!/usr/bin/env python3
"""
Custom ELF32 Thumb Assembler & Builder for TinyCpuSim Test Fixtures
"""

import os
import sys
import glob
import struct
import re

extra_paths = [
    "/home/kw/snap/antigravity-cli/common/local/lib/python3.14/dist-packages",
    "/home/kw/snap/antigravity-cli/common/local/lib/python3.12/site-packages",
]
for p in extra_paths:
    if os.path.exists(p) and p not in sys.path:
        sys.path.insert(0, p)

import keystone

def clean_asm(text, base_addr=0x10000):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    lines = []
    in_bss = False
    bss_labels = {}
    current_bss_offset = 0x2000
    for line in text.splitlines():
        line = line.split('//')[0].split('@')[0].strip()
        if not line:
            continue
        if '.bss' in line:
            in_bss = True
            continue
        if in_bss:
            if line.endswith(':'):
                lbl = line[:-1].strip()
                bss_labels[lbl] = base_addr + current_bss_offset
            elif '.space' in line:
                parts = line.split('.space')
                sz = int(parts[1].strip())
                current_bss_offset += sz
            continue
        if line.startswith('.'):
            continue
        if '.word' in line:
            continue
        lines.append(line)
    
    bss_labels.setdefault('fwd_buf', 0x12000)
    bss_labels.setdefault('scratch_buf', 0x12000)
    bss_labels.setdefault('stack_mem', 0x12000)
    bss_labels.setdefault('stack_top', 0x13000)
    bss_labels.setdefault('sort_array', 0x12000)
    bss_labels.setdefault('stride_buf', 0x12000)
    
    asm_code = '\n'.join(lines)
    for lbl, addr in bss_labels.items():
        asm_code = re.sub(rf'={lbl}\b', f'={hex(addr)}', asm_code)
    return asm_code

def build_elf(asm_text, output_path, load_addr=0x00010000, bss_size=0x10000):
    cleaned_asm = clean_asm(asm_text, load_addr)
    ks = keystone.Ks(keystone.KS_ARCH_ARM, keystone.KS_MODE_THUMB)
    encoding, count = ks.asm(cleaned_asm, load_addr)
    code_bytes = bytes(encoding)
    
    # Pad code to 4-byte alignment
    pad_len = (4 - (len(code_bytes) % 4)) % 4
    code_bytes += b'\x00' * pad_len
    
    file_size = 0x1000 + len(code_bytes)
    mem_size = 0x20000 # 128KB address space for code + BSS
    entry_point = load_addr + 1 # Thumb entry point (bit 0 = 1)
    
    # ELF Header (52 bytes)
    e_ident = b'\x7fELF\x01\x01\x01\x00' + b'\x00' * 8
    e_type = 2          # ET_EXEC
    e_machine = 40      # EM_ARM
    e_version = 1       # EV_CURRENT
    e_entry = entry_point
    e_phoff = 52        # Program header offset
    e_shoff = 0         # No section headers needed for loader
    e_flags = 0x05000200 # EABI version 5
    e_ehsize = 52
    e_phentsize = 32
    e_phnum = 1
    e_shentsize = 40
    e_shnum = 0
    e_shstrndx = 0
    
    elf_header = struct.pack(
        '<16sHHIIIIIHHHHHH',
        e_ident, e_type, e_machine, e_version, e_entry,
        e_phoff, e_shoff, e_flags, e_ehsize, e_phentsize,
        e_phnum, e_shentsize, e_shnum, e_shstrndx
    )
    
    # Program Header (32 bytes)
    p_type = 1          # PT_LOAD
    p_offset = 0x1000   # Offset in file
    p_vaddr = load_addr
    p_paddr = load_addr
    p_filesz = len(code_bytes)
    p_memsz = mem_size
    p_flags = 7         # PF_R | PF_W | PF_X
    p_align = 0x1000
    
    prog_header = struct.pack(
        '<IIIIIIII',
        p_type, p_offset, p_vaddr, p_paddr,
        p_filesz, p_memsz, p_flags, p_align
    )
    
    header_block = elf_header + prog_header
    header_padding = b'\x00' * (0x1000 - len(header_block))
    
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(header_block)
        f.write(header_padding)
        f.write(code_bytes)
        
    print(f"[OK] Generated ELF: {os.path.basename(output_path):<24} (Code: {len(code_bytes)} bytes, Entry: {hex(entry_point)})")

def build_from_source(asm_path, output_path=None):
    if output_path is None:
        root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        base_name = os.path.splitext(os.path.basename(asm_path))[0]
        output_path = os.path.join(root_dir, "tests", "fixtures", f"{base_name}.elf")
        
    with open(asm_path, "r") as f:
        asm_text = f.read()
        
    build_elf(asm_text, output_path)

import concurrent.futures
import argparse

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    asm_dir = os.path.join(root_dir, "tests", "asm")
    default_jobs = max(1, (os.cpu_count() or 4) // 2)
    
    parser = argparse.ArgumentParser(description="ELF32 Thumb Assembler & Builder for TinyCpuSim Test Fixtures")
    parser.add_argument("targets", nargs="*", help="Specific .s targets to build")
    parser.add_argument("-j", "--jobs", type=int, default=default_jobs, help=f"Number of parallel workers (default: {default_jobs}, half of CPU cores)")
    parser.add_argument("-a", "--all", action="store_true", help="Build all assembly fixtures")
    args = parser.parse_args()
    
    target_files = []
    if args.targets and not args.all:
        for target in args.targets:
            if os.path.exists(target):
                target_files.append(target)
            else:
                candidate = os.path.join(asm_dir, target if target.endswith(".s") else f"{target}.s")
                if os.path.exists(candidate):
                    target_files.append(candidate)
                else:
                    print(f"Error: Target {target} not found.")
    else:
        target_files = sorted(glob.glob(os.path.join(asm_dir, "*.s")))
        
    if not target_files:
        print("No target assembly files to build.")
        return

    print(f"Building {len(target_files)} ELF fixtures from {asm_dir} using {args.jobs} parallel workers (half of CPU cores)...")
    if args.jobs <= 1 or len(target_files) == 1:
        for asm_file in target_files:
            build_from_source(asm_file)
    else:
        with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as executor:
            futures = [executor.submit(build_from_source, f) for f in target_files]
            for future in concurrent.futures.as_completed(futures):
                future.result()

    print("All fixtures built successfully.")

if __name__ == "__main__":
    main()
