#include <gtest/gtest.h>
#include "tinyarmsim/uarch/rob_issue_queue.hpp"

using namespace tinyarmsim::uarch;

TEST(RobIssueQueueTest, ReorderBufferInOrderAllocOutOrderCompleteInOrderCommit) {
    ReorderBuffer rob(8);
    EXPECT_TRUE(rob.is_empty());
    EXPECT_FALSE(rob.is_full());

    UOp uop1; uop1.uop_id = 1; uop1.seq_num = 1;
    UOp uop2; uop2.uop_id = 2; uop2.seq_num = 2;

    size_t rob_idx1 = rob.allocate(uop1);
    size_t rob_idx2 = rob.allocate(uop2);
    EXPECT_EQ(rob_idx1, 0);
    EXPECT_EQ(rob_idx2, 1);
    EXPECT_EQ(rob.size(), 2);

    // uop2 completes execution BEFORE uop1 (Out-of-Order Execution)
    rob.mark_completed(rob_idx2);
    EXPECT_FALSE(rob.can_commit_head()); // Cannot commit head because uop1 is still pending

    // uop1 completes execution
    rob.mark_completed(rob_idx1);
    EXPECT_TRUE(rob.can_commit_head());

    // In-order commit
    ROBEntry c1 = rob.commit_head();
    EXPECT_EQ(c1.uop.uop_id, 1);
    EXPECT_TRUE(rob.can_commit_head());

    ROBEntry c2 = rob.commit_head();
    EXPECT_EQ(c2.uop.uop_id, 2);
    EXPECT_TRUE(rob.is_empty());
}

TEST(RobIssueQueueTest, ReorderBufferFlushYoungerOnBranchMisprediction) {
    ReorderBuffer rob(8);

    UOp b_uop; b_uop.uop_id = 10;
    size_t branch_idx = rob.allocate(b_uop); // rob_idx 0

    UOp spec_uop1; spec_uop1.uop_id = 11;
    rob.allocate(spec_uop1); // rob_idx 1

    UOp spec_uop2; spec_uop2.uop_id = 12;
    rob.allocate(spec_uop2); // rob_idx 2

    EXPECT_EQ(rob.size(), 3);

    // Branch resolves and flushes younger speculative entries
    rob.flush_younger_than(branch_idx);
    EXPECT_EQ(rob.size(), 1);

    rob.mark_completed(branch_idx);
    EXPECT_TRUE(rob.can_commit_head());
    ROBEntry committed = rob.commit_head();
    EXPECT_EQ(committed.uop.uop_id, 10);
    EXPECT_TRUE(rob.is_empty());
}

TEST(RobIssueQueueTest, IssueQueueWakeupAndOutOfOrderSelect) {
    IssueQueue iq(8);

    // UOp A: depends on P16 (not ready)
    UOp uopA;
    uopA.uop_id = 100;
    uopA.phys_src1 = 16;
    uopA.src1_ready = false;

    // UOp B: no dependencies (ready)
    UOp uopB;
    uopB.uop_id = 101;
    uopB.src1_ready = true;
    uopB.src2_ready = true;
    uopB.src3_ready = true;

    iq.insert(uopA);
    iq.insert(uopB);
    EXPECT_EQ(iq.size(), 2);

    // Select: Only uopB is ready
    auto issued1 = iq.select_and_issue(2);
    ASSERT_EQ(issued1.size(), 1);
    EXPECT_EQ(issued1[0].uop_id, 101);
    EXPECT_EQ(iq.size(), 1);

    // Wakeup P16
    iq.wakeup(16);

    // Now uopA becomes ready!
    auto issued2 = iq.select_and_issue(2);
    ASSERT_EQ(issued2.size(), 1);
    EXPECT_EQ(issued2[0].uop_id, 100);
    EXPECT_TRUE(iq.is_empty());
}
