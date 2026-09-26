#include <gtest/gtest.h>
#include "tinyarmsim/uarch/multicore_system.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

TEST(MultiCoreSystemTest, FourCoreConcurrentExecutionAndStatsAggregation) {
    UArchConfig cfg = UArchConfig::make_multicore_default(4);
    cfg.coherence = CoherenceProtocol::MESI;

    MultiCoreSystem system(cfg, 1024 * 1024);
    EXPECT_EQ(system.num_cores(), 4);

    MemoryBus& bus = system.get_bus();

    // Populate distinct code regions for Core 0..3
    for (size_t c = 0; c < 4; ++c) {
        uint32_t base_pc = 0x1000 + static_cast<uint32_t>(c * 0x100);
        // MOV R0, #(c + 1) (Thumb-16: 0x2000 | (c + 1))
        bus.write16(base_pc, static_cast<uint16_t>(0x2000 | (c + 1)));
        // ADD R1, R0, #10 (Thumb-16: 0x300A)
        bus.write16(base_pc + 2, 0x300A);
        // SVC #0 (Halt) (Thumb-16: 0xDF00)
        bus.write16(base_pc + 4, 0xDF00);

        system.set_entry_pc(c, base_pc);
    }

    uint64_t total_cycles = system.run(50);
    EXPECT_TRUE(system.all_halted());
    EXPECT_GT(total_cycles, 0);

    UArchStats stats = system.collect_stats();
    EXPECT_EQ(stats.cores.size(), 4);
    EXPECT_GE(stats.total_committed_instructions(), 8);
    EXPECT_GT(stats.total_ipc(), 0.0);

    std::string report = stats.format_text();
    EXPECT_FALSE(report.empty());
}
