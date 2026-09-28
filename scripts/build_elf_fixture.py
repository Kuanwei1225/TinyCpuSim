#!/usr/bin/env python3
"""
Custom ELF32 Thumb Assembler & Builder for TinyCpuSim Test Fixtures
"""

import os
import struct
import keystone

def build_elf(asm_text, output_path, load_addr=0x00010000, bss_size=64):
    ks = keystone.Ks(keystone.KS_ARCH_ARM, keystone.KS_MODE_THUMB)
    encoding, count = ks.asm(asm_text, load_addr)
    code_bytes = bytes(encoding)
    
    # Pad code to 4-byte alignment
    pad_len = (4 - (len(code_bytes) % 4)) % 4
    code_bytes += b'\x00' * pad_len
    
    file_size = 0x1000 + len(code_bytes)
    mem_size = len(code_bytes) + bss_size
    entry_point = load_addr + 1 # Thumb entry point (bit 0 = 1)
    
    # ELF Header (52 bytes)
    # e_ident: 16 bytes
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
    
    # Construct complete file
    header_block = elf_header + prog_header
    header_padding = b'\x00' * (0x1000 - len(header_block))
    
    with open(output_path, 'wb') as f:
        f.write(header_block)
        f.write(header_padding)
        f.write(code_bytes)
        
    print(f"[OK] Generated ELF: {output_path} (Code: {len(code_bytes)} bytes, Total: {file_size} bytes, Entry: {hex(entry_point)})")

if __name__ == "__main__":
    pass
