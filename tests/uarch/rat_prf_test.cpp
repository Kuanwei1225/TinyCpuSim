#include <gtest/gtest.h>
#include "tinyarmsim/uarch/rat_prf.hpp"

using namespace tinyarmsim::uarch;

TEST(RatPrfTest, PhysicalRegisterFileReadWriteAndReady) {
    PhysicalRegisterFile prf(64);
    EXPECT_EQ(prf.size(), 64);

    // Initial state: ready
    EXPECT_TRUE(prf.is_ready(10));
    EXPECT_EQ(prf.read(10), 0);

    // Write value
    prf.write(10, 0xCAFEBABE);
    EXPECT_EQ(prf.read(10), 0xCAFEBABE);
    EXPECT_TRUE(prf.is_ready(10));

    // Clear ready
    prf.set_ready(10, false);
    EXPECT_FALSE(prf.is_ready(10));

    // Out of range check
    EXPECT_THROW(static_cast<void>(prf.read(100)), std::out_of_range);
    EXPECT_THROW(prf.write(100, 42), std::out_of_range);
}

TEST(RatPrfTest, FreeListAllocationAndExhaustion) {
    FreeList free_list(32, 16); // 16 free physical registers (16..31)
    EXPECT_TRUE(free_list.has_free());
    EXPECT_EQ(free_list.free_count(), 16);

    uint16_t p1 = free_list.allocate();
    EXPECT_EQ(p1, 16);
    EXPECT_EQ(free_list.free_count(), 15);

    // Free p1 back
    free_list.free(p1);
    EXPECT_EQ(free_list.free_count(), 16);

    // Drain all
    for (int i = 0; i < 16; ++i) {
        static_cast<void>(free_list.allocate());
    }
    EXPECT_FALSE(free_list.has_free());
    EXPECT_THROW(static_cast<void>(free_list.allocate()), std::runtime_error);
}

TEST(RatPrfTest, RegisterAliasTableSpeculativeCommitAndBranchCheckpoint) {
    RegisterAliasTable rat;
    FreeList free_list(64, 16);

    // Initially R0..R15 map to P0..P15
    EXPECT_EQ(rat.get(0), 0);
    EXPECT_EQ(rat.get_commit(0), 0);

    // Speculatively rename R0 -> P16
    uint16_t p16 = free_list.allocate();
    rat.set(0, p16);
    EXPECT_EQ(rat.get(0), 16);
    EXPECT_EQ(rat.get_commit(0), 0); // Commit RAT untouched

    // Take checkpoint before a branch
    auto checkpoint = rat.create_checkpoint();

    // Speculatively rename R0 -> P17 after branch
    uint16_t p17 = free_list.allocate();
    rat.set(0, p17);
    EXPECT_EQ(rat.get(0), 17);

    // Branch Misprediction: Restore checkpoint!
    rat.restore_checkpoint(checkpoint);
    free_list.free(p17);
    EXPECT_EQ(rat.get(0), 16); // Restored to pre-branch state!

    // Commit R0 -> P16
    rat.commit(0, 16);
    EXPECT_EQ(rat.get_commit(0), 16);
}
