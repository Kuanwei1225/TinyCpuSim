#include <gtest/gtest.h>
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/uarch/ooo_core.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

TEST(OoOCoreTest, ExecutesLinearInstructionSequenceAndRetires) {
    MemoryBus bus(64 * 1024);

    // 0x1000: MOV R0, #10 (Thumb-16: 0x200A)
    bus.write16(0x1000, 0x200A);
    // 0x1002: MOV R1, #20 (Thumb-16: 0x2114)
    bus.write16(0x1002, 0x2114);
    // 0x1004: ADD R2, R0, R1 (Thumb-16: 0x1842)
    bus.write16(0x1004, 0x1842);
    // 0x1006: SVC #0 (Halt) (Thumb-16: 0xDF00)
    bus.write16(0x1006, 0xDF00);

    CoreConfig cfg;
    cfg.fetch_width = 4;
    cfg.commit_width = 4;
    cfg.branch_predictor.enabled = false;

    OoOCore core(0, cfg, bus, nullptr, nullptr, 0x1000);

    for (int cycle = 0; cycle < 30; ++cycle) {
        core.tick();
        if (core.is_halted()) break;
    }

    EXPECT_TRUE(core.is_halted());
    EXPECT_GE(core.get_committed_instructions(), 3);
    EXPECT_GT(core.get_cycles(), 0);

    CoreStats stats = core.get_stats();
    EXPECT_GT(stats.ipc(), 0.0);
}

TEST(OoOCoreTest, RawDataHazardWakeupThroughPhysicalRegisters) {
    MemoryBus bus(64 * 1024);

    // 0x1000: MOV R0, #100 (Thumb-16: 0x2064)
    bus.write16(0x1000, 0x2064);
    // 0x1002: ADD R1, R0, #5 (Thumb-16: 0x3005) -> RAW Hazard on R0!
    bus.write16(0x1002, 0x3005);
    // 0x1004: SVC #0 (Thumb-16: 0xDF00)
    bus.write16(0x1004, 0xDF00);

    CoreConfig cfg;
    OoOCore core(0, cfg, bus, nullptr, nullptr, 0x1000);

    for (int cycle = 0; cycle < 20; ++cycle) {
        core.tick();
        if (core.is_halted()) break;
    }

    EXPECT_TRUE(core.is_halted());
    EXPECT_GE(core.get_committed_instructions(), 2);
}
