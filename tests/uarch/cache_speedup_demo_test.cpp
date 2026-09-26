#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/memory_hierarchy.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

TEST(Phase1DemoTest, CacheSpeedupOverUncachedDramAccess) {
    MemoryBus bus(4 * 1024 * 1024); // 4MB RAM

    // Initialize an array with data
    constexpr uint32_t ARRAY_BASE = 0x10000;
    constexpr size_t NUM_ELEMENTS = 2048; // 8KB data
    constexpr size_t NUM_ITERATIONS = 10;

    for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
        bus.write32(ARRAY_BASE + static_cast<uint32_t>(i * 4), static_cast<uint32_t>(i));
    }

    // 1. Run with Cache DISABLED (Direct DRAM access: 80 cycles per access)
    UArchConfig uncached_cfg = UArchConfig::make_in_order_simple();
    uncached_cfg.dram_latency_cycles = 80;
    uncached_cfg.default_core.l1d.type = CacheType::PASSTHROUGH;
    uncached_cfg.l2_shared.type = CacheType::PASSTHROUGH;
    uncached_cfg.coherence = CoherenceProtocol::NONE;

    CoherentMemoryHierarchy uncached_mem(bus, uncached_cfg);

    uint64_t uncached_cycles = 0;
    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            uint32_t addr = ARRAY_BASE + static_cast<uint32_t>(i * 4);
            auto resp = uncached_mem.read_data(0, addr, 4, uncached_cycles);
            uncached_cycles += resp.latency_cycles;
        }
    }

    // 2. Run with Cache ENABLED (L1D: 32KB, L2: 512KB)
    UArchConfig cached_cfg = UArchConfig::make_ooo_default();
    cached_cfg.dram_latency_cycles = 80;
    cached_cfg.default_core.l1d.type = CacheType::SET_ASSOCIATIVE;
    cached_cfg.default_core.l1d.hit_latency_cycles = 1;
    cached_cfg.l2_shared.type = CacheType::SET_ASSOCIATIVE;
    cached_cfg.l2_shared.hit_latency_cycles = 10;

    CoherentMemoryHierarchy cached_mem(bus, cached_cfg);

    uint64_t cached_cycles = 0;
    for (size_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            uint32_t addr = ARRAY_BASE + static_cast<uint32_t>(i * 4);
            auto resp = cached_mem.read_data(0, addr, 4, cached_cycles);
            cached_cycles += resp.latency_cycles;
        }
    }

    double speedup = static_cast<double>(uncached_cycles) / static_cast<double>(cached_cycles);
    double l1_hit_rate = cached_mem.get_l1d(0).get_stats().hit_rate() * 100.0;

    std::cout << "\n============================================================\n"
              << "       Phase 1 Demo: Cache Hierarchy Speedup Report         \n"
              << "============================================================\n"
              << "Uncached DRAM Access Cycles: " << uncached_cycles << " cycles\n"
              << "Cached Memory Hierarchy:     " << cached_cycles << " cycles\n"
              << "Measured Speedup:            " << speedup << "x faster!\n"
              << "L1 Data Cache Hit Rate:      " << l1_hit_rate << "%\n"
              << "============================================================\n";

    EXPECT_GT(l1_hit_rate, 85.0); // Iterative traversal yields high hit rate
    EXPECT_GT(speedup, 10.0);      // Cache provides >10x memory access speedup
}

TEST(Phase1DemoTest, MultiCoreMESICoherenceStateTransitions) {
    MemoryBus bus(4 * 1024 * 1024);
    UArchConfig cfg = UArchConfig::make_multicore_default(4); // 4 Cores
    CoherentMemoryHierarchy mem(bus, cfg);

    uint32_t shared_var_addr = 0x20000;

    // Phase A: Core 0 writes -> Core 0 becomes MODIFIED
    mem.write_data(0, shared_var_addr, 100, 4);
    EXPECT_EQ(mem.get_coherence().get_state(0, shared_var_addr), MESIState::MODIFIED);

    // Phase B: Core 1, Core 2, Core 3 read -> All transition to SHARED
    mem.read_data(1, shared_var_addr, 4);
    mem.read_data(2, shared_var_addr, 4);
    mem.read_data(3, shared_var_addr, 4);

    EXPECT_EQ(mem.get_coherence().get_state(0, shared_var_addr), MESIState::SHARED);
    EXPECT_EQ(mem.get_coherence().get_state(1, shared_var_addr), MESIState::SHARED);
    EXPECT_EQ(mem.get_coherence().get_state(2, shared_var_addr), MESIState::SHARED);
    EXPECT_EQ(mem.get_coherence().get_state(3, shared_var_addr), MESIState::SHARED);

    // Phase C: Core 2 writes -> Broadcasts BusUpgr, Core 2 becomes MODIFIED, others INVALID
    mem.write_data(2, shared_var_addr, 200, 4);
    EXPECT_EQ(mem.get_coherence().get_state(2, shared_var_addr), MESIState::MODIFIED);
    EXPECT_EQ(mem.get_coherence().get_state(0, shared_var_addr), MESIState::INVALID);
    EXPECT_EQ(mem.get_coherence().get_state(1, shared_var_addr), MESIState::INVALID);
    EXPECT_EQ(mem.get_coherence().get_state(3, shared_var_addr), MESIState::INVALID);

    UArchStats stats = mem.collect_stats();
    std::cout << "\n============================================================\n"
              << "       Phase 1 Demo: Multi-Core MESI Coherence Report       \n"
              << "============================================================\n"
              << "Active Cores:             4 Cores\n"
              << "MESI Snoop Transactions:  " << stats.mesi_snoop_requests << "\n"
              << "MESI Invalidation Events: " << stats.mesi_invalidations << "\n"
              << "============================================================\n";

    EXPECT_GE(stats.mesi_snoop_requests, 4);
    EXPECT_GE(stats.mesi_invalidations, 3);
}
