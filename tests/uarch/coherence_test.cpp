#include <gtest/gtest.h>
#include "tinyarmsim/uarch/coherence.hpp"
#include "tinyarmsim/uarch/memory_hierarchy.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

TEST(MESICoherenceTest, SingleCoreTransitions) {
    MESICoherenceEngine mesi(1, 64);
    uint32_t addr = 0x1000;

    EXPECT_EQ(mesi.get_state(0, addr), MESIState::INVALID);

    // Read on I -> transitions to EXCLUSIVE (since no other core has it)
    auto act_rd = mesi.handle_cpu_read(0, addr);
    EXPECT_FALSE(act_rd.is_hit);
    EXPECT_EQ(act_rd.new_state, MESIState::EXCLUSIVE);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::EXCLUSIVE);

    // Silent write on E -> transitions to MODIFIED (0 bus tx)
    auto act_wr = mesi.handle_cpu_write(0, addr);
    EXPECT_TRUE(act_wr.is_hit);
    EXPECT_EQ(act_wr.bus_tx, BusTransactionType::NONE);
    EXPECT_EQ(act_wr.new_state, MESIState::MODIFIED);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::MODIFIED);
}

TEST(MESICoherenceTest, DualCoreSharedReadThenUpgrade) {
    MESICoherenceEngine mesi(2, 64);
    uint32_t addr = 0x2000;

    // 1. Core 0 reads -> EXCLUSIVE
    mesi.handle_cpu_read(0, addr);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::EXCLUSIVE);

    // 2. Core 1 reads -> Core 0 demotes to SHARED, Core 1 becomes SHARED
    auto act_c1_rd = mesi.handle_cpu_read(1, addr);
    EXPECT_EQ(act_c1_rd.new_state, MESIState::SHARED);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::SHARED);
    EXPECT_EQ(mesi.get_state(1, addr), MESIState::SHARED);

    // 3. Core 0 writes to SHARED line -> broadcasts BusUpgr, Core 1 invalidated to (I)
    auto act_c0_wr = mesi.handle_cpu_write(0, addr);
    EXPECT_EQ(act_c0_wr.bus_tx, BusTransactionType::BUS_UPGR);
    EXPECT_EQ(act_c0_wr.new_state, MESIState::MODIFIED);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::MODIFIED);
    EXPECT_EQ(mesi.get_state(1, addr), MESIState::INVALID);
    EXPECT_EQ(act_c0_wr.invalidations, 1);
}

TEST(MESICoherenceTest, RemoteReadFromModifiedTriggersFlush) {
    MESICoherenceEngine mesi(2, 64);
    uint32_t addr = 0x3000;

    // Core 0 writes directly -> MODIFIED
    mesi.handle_cpu_write(0, addr);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::MODIFIED);

    // Core 1 reads -> Core 0 must flush Modified line, both become SHARED
    auto act = mesi.handle_cpu_read(1, addr);
    EXPECT_EQ(act.bus_tx, BusTransactionType::BUS_RD);
    EXPECT_EQ(act.new_state, MESIState::SHARED);
    EXPECT_EQ(mesi.get_state(0, addr), MESIState::SHARED);
    EXPECT_EQ(mesi.get_state(1, addr), MESIState::SHARED);
}

TEST(MESICoherenceTest, ExclusiveMonitorLdrexStrex) {
    MESICoherenceEngine mesi(2, 64);
    uint32_t addr = 0x4000;

    // Core 0 executes LDREX on addr
    mesi.set_exclusive_monitor(0, addr);

    // Without any intervening peer write, STREX succeeds
    bool ok = mesi.check_and_clear_exclusive_monitor(0, addr);
    EXPECT_TRUE(ok);

    // Core 0 sets monitor again
    mesi.set_exclusive_monitor(0, addr);

    // Core 1 writes to the same line -> invalidates Core 0's exclusive monitor
    mesi.handle_cpu_write(1, addr);

    // Core 0's STREX must fail!
    bool ok_after_peer_write = mesi.check_and_clear_exclusive_monitor(0, addr);
    EXPECT_FALSE(ok_after_peer_write);
}

TEST(MemoryHierarchyTest, CoherentMemoryLatencyAndHitHierarchy) {
    MemoryBus bus(1024 * 1024); // 1MB physical RAM
    UArchConfig cfg = UArchConfig::make_multicore_default(2);
    cfg.default_core.l1d.hit_latency_cycles = 1;
    cfg.l2_shared.hit_latency_cycles = 10;
    cfg.dram_latency_cycles = 80;

    CoherentMemoryHierarchy mem(bus, cfg);

    uint32_t addr = 0x5000;

    // 1. First Write on Core 0 -> Cold Miss, writes to RAM and allocates L1/L2
    auto resp1 = mem.write_data(0, addr, 0xCAFEBABE, 4);
    EXPECT_TRUE(resp1.is_dram_access);
    EXPECT_GE(resp1.latency_cycles, 80);

    // 2. Second Read on Core 0 -> L1D Hit (1 cycle latency)
    auto resp2 = mem.read_data(0, addr, 4);
    EXPECT_TRUE(resp2.is_l1_hit);
    EXPECT_EQ(resp2.latency_cycles, 1);
    EXPECT_EQ(resp2.data, 0xCAFEBABE);

    // 3. Core 1 reads the same address -> Fetches from Shared L2 / Peer Flush
    auto resp3 = mem.read_data(1, addr, 4);
    EXPECT_EQ(resp3.data, 0xCAFEBABE);
    EXPECT_GE(resp3.latency_cycles, 10); // L2 / Coherence latency
}
