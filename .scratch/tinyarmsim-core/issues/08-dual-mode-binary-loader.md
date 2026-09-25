# 08: Dual-Mode Binary Loader (ELF32 Static Binary & Flat Raw Binary)

**What to build:** The `Loader` component supporting dual-mode binary loading: parsing ELF32 static binaries (ELF header validation, iterating `PT_LOAD` segments, populating `MemoryBus`, and setting initial entry PC) and loading flat raw binary payloads to a configurable base address.

**Blocked by:** 02-architectural-state-and-memory-bus

**Status:** resolved

- [x] `Loader::load_elf(std::istream&, MemoryBus&, ArchitecturalState&)` parses ELF32 header, verifies magic numbers (`0x7F 'E' 'L' 'F'`), machine type (ARM), and copies `PT_LOAD` segments into memory at `p_vaddr`
- [x] Initial `ArchitecturalState.PC` is set to `e_entry`
- [x] `Loader::load_raw(std::istream&, MemoryBus&, uint32_t base_address)` copies raw stream bytes sequentially into `MemoryBus`
- [x] Corrupt ELF headers or invalid segment bounds throw descriptive `LoaderException`
- [x] Unit tests verify ELF loading with synthetic ELF byte streams and raw binaries
