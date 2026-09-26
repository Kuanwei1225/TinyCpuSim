#include <gtest/gtest.h>
#include "tinyarmsim/uarch/rob_issue_queue.hpp"
#include "tinyarmsim/uarch/topdown_profiler.hpp"
#include "tinyarmsim/uarch/rat_prf.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

// =============================================================================
// Reorder Buffer Isolated Microbenchmarks (ROB_UBench)
// =============================================================================

// 1. Sustained retirement throughput at max commit width (4-way commit)
TEST(RobTopDownUBenchTest, ROB_UBench_SustainedRetireThroughput) {
    ReorderBuffer rob(32);

    // Allocate 8 uops
    for (uint64_t seq = 1; seq <= 8; ++seq) {
        UOp uop;
        uop.seq_num = seq;
        uop.type = UOpType::ALU;
        size_t idx = rob.allocate(uop);
        rob.mark_completed(idx); // All completed out of order
    }

    EXPECT_EQ(rob.size(), 8);

    // Commit first bundle of up to 4 uops
    size_t commit1 = 0;
    for (int i = 0; i < 4; ++i) {
        if (rob.can_commit_head()) {
            rob.commit_head();
            commit1++;
        }
    }
    EXPECT_EQ(commit1, 4);
    EXPECT_EQ(rob.size(), 4);

    // Commit second bundle of up to 4 uops
    size_t commit2 = 0;
    for (int i = 0; i < 4; ++i) {
        if (rob.can_commit_head()) {
            rob.commit_head();
            commit2++;
        }
    }
    EXPECT_EQ(commit2, 4);
    EXPECT_TRUE(rob.is_empty());
}

// 2. Head-of-ROB blocking: unready head blocks younger ready uops from committing
TEST(RobTopDownUBenchTest, ROB_UBench_HeadOfRobBlockingRetire) {
    ReorderBuffer rob(16);

    // uop 0 (head): Pending long-latency load/div (NOT completed)
    UOp u0; u0.seq_num = 1; u0.type = UOpType::LOAD;
    size_t idx0 = rob.allocate(u0);

    // uops 1..5: Fast ALU uops (COMPLETED out-of-order)
    std::vector<size_t> younger_indices;
    for (uint64_t seq = 2; seq <= 5; ++seq) {
        UOp u; u.seq_num = seq; u.type = UOpType::ALU;
        size_t idx = rob.allocate(u);
        rob.mark_completed(idx);
        younger_indices.push_back(idx);
    }

    EXPECT_EQ(rob.size(), 5);

    // Try to commit: Head is not ready -> NO uop can commit (strict in-order commit)
    EXPECT_FALSE(rob.can_commit_head());

    // Long-latency load finishes at idx0
    rob.mark_completed(idx0);
    EXPECT_TRUE(rob.can_commit_head());

    // Now all 5 uops can commit in-order
    size_t committed = 0;
    while (rob.can_commit_head()) {
        rob.commit_head();
        committed++;
    }
    EXPECT_EQ(committed, 5);
    EXPECT_TRUE(rob.is_empty());
}

// 3. Circular buffer wrap-around stress (256 operations over 32-entry capacity)
TEST(RobTopDownUBenchTest, ROB_UBench_CircularBufferWrapAroundStress) {
    ReorderBuffer rob(32);

    for (uint64_t cycle = 1; cycle <= 256; ++cycle) {
        UOp uop;
        uop.seq_num = cycle;
        uop.type = UOpType::ALU;
        size_t idx = rob.allocate(uop);
        rob.mark_completed(idx);

        EXPECT_TRUE(rob.can_commit_head());
        auto committed = rob.commit_head();
        EXPECT_EQ(committed.uop.seq_num, cycle);
    }

    EXPECT_TRUE(rob.is_empty());
    EXPECT_FALSE(rob.is_full());
}

// 4. Speculative store commits only when reaching ROB head
TEST(RobTopDownUBenchTest, ROB_UBench_SpeculativeStoreDrainOnRetire) {
    ReorderBuffer rob(16);

    UOp u_alu; u_alu.seq_num = 1; u_alu.type = UOpType::ALU;
    size_t idx_alu = rob.allocate(u_alu);

    UOp u_store; u_store.seq_num = 2; u_store.type = UOpType::STORE_DATA; u_store.lsu_queue_idx = 3;
    size_t idx_store = rob.allocate(u_store);

    // Store completes in SQ early
    rob.mark_completed(idx_store);

    // Store is NOT at head, so cannot commit yet
    EXPECT_FALSE(rob.can_commit_head());

    // ALU completes
    rob.mark_completed(idx_alu);
    EXPECT_TRUE(rob.can_commit_head());

    auto c1 = rob.commit_head();
    EXPECT_EQ(c1.uop.type, UOpType::ALU);

    // Now store is at head and can drain to cache/memory
    EXPECT_TRUE(rob.can_commit_head());
    auto c2 = rob.commit_head();
    EXPECT_EQ(c2.uop.type, UOpType::STORE_DATA);
    EXPECT_EQ(c2.uop.lsu_queue_idx, 3);
}

// 5. Branch misprediction flushes all younger uops and rolls back tail
TEST(RobTopDownUBenchTest, ROB_UBench_MultipleBranchMispredictFlushes) {
    ReorderBuffer rob(32);

    // Allocate uops 1..6
    std::vector<size_t> indices;
    for (uint64_t s = 1; s <= 6; ++s) {
        UOp u; u.seq_num = s;
        indices.push_back(rob.allocate(u));
    }
    EXPECT_EQ(rob.size(), 6);

    // uop 3 (index indices[2]) is a mispredicted branch -> flush younger uops (4, 5, 6)
    rob.flush_younger_than(indices[2]);

    EXPECT_EQ(rob.size(), 3); // Only uops 1, 2, 3 remain
}

// 6. Relative age distance in circular buffer
TEST(RobTopDownUBenchTest, ROB_UBench_IsYoungerCircularAgeDistance) {
    ReorderBuffer rob(16);

    // Allocate 10 uops
    std::vector<size_t> idxs;
    for (uint64_t i = 0; i < 10; ++i) {
        UOp u; u.seq_num = i;
        idxs.push_back(rob.allocate(u));
    }

    // idxs[5] is younger than idxs[2]
    EXPECT_TRUE(rob.is_younger(idxs[5], idxs[2]));
    EXPECT_FALSE(rob.is_younger(idxs[2], idxs[5]));
    EXPECT_FALSE(rob.is_younger(idxs[2], idxs[2]));

    // Commit 8 uops so head moves forward near tail
    for (size_t i = 0; i < 8; ++i) {
        rob.mark_completed(idxs[i]);
        rob.commit_head();
    }

    // Allocate across buffer wrap-around
    size_t wrap_idx1 = rob.allocate(UOp{});
    size_t wrap_idx2 = rob.allocate(UOp{});

    EXPECT_TRUE(rob.is_younger(wrap_idx2, wrap_idx1));
    EXPECT_TRUE(rob.is_younger(wrap_idx1, idxs[9]));
}

// =============================================================================
// Top-Down Profiler Isolated Microbenchmarks (TopDown_UBench)
// =============================================================================

// 1. Slot conservation invariant: TotalSlots == Retiring + BadSpec + FE + BE
TEST(RobTopDownUBenchTest, TopDown_UBench_SlotConservationInvariant) {
    TopDownProfiler profiler;
    profiler.init(1, 4);

    // Simulate 100 cycles of mixed slot events
    for (int cycle = 0; cycle < 100; ++cycle) {
        if (cycle % 4 == 0) {
            profiler.record_slot(0, SlotType::RetiringBaseAlu);
            profiler.record_slot(0, SlotType::RetiringMem);
            profiler.record_slot(0, SlotType::FrontEndFetchBubble);
            profiler.record_slot(0, SlotType::BackEndCoreRSFull);
        } else if (cycle % 4 == 1) {
            profiler.record_slot(0, SlotType::BadSpecMispredict);
            profiler.record_slot(0, SlotType::BadSpecMispredict);
            profiler.record_slot(0, SlotType::BackEndMemL1DMiss);
            profiler.record_slot(0, SlotType::BackEndMemL2Miss);
        } else if (cycle % 4 == 2) {
            profiler.record_slot(0, SlotType::FrontEndL1IMiss);
            profiler.record_slot(0, SlotType::FrontEndL1IMiss);
            profiler.record_slot(0, SlotType::FrontEndFetchBubble);
            profiler.record_slot(0, SlotType::RetiringBaseAlu);
        } else {
            profiler.record_slot(0, SlotType::BackEndCoreRobFull);
            profiler.record_slot(0, SlotType::BackEndCoreFreeListEmpty);
            profiler.record_slot(0, SlotType::BackEndMemMSHRFull);
            profiler.record_slot(0, SlotType::BackEndMemStoreBufFull);
        }
    }

    const auto& report = profiler.get_report(0);
    EXPECT_EQ(report.total_slots, 400);

    uint64_t sum_l1 = report.retiring_slots + report.bad_spec_slots + report.frontend_slots + report.backend_slots;
    EXPECT_EQ(sum_l1, report.total_slots);

    // Invariant holds for percentages as well (sum ~ 100.0%)
    double sum_pct = report.retiring_pct() + report.bad_spec_pct() + report.frontend_pct() + report.backend_pct();
    EXPECT_NEAR(sum_pct, 100.0, 0.001);
}

// 2. Frontend vs Backend bound classification breakdown
TEST(RobTopDownUBenchTest, TopDown_UBench_FrontendVsBackendBreakdown) {
    TopDownProfiler profiler;
    profiler.init(1, 2);

    // 10 cycles of Frontend L1I Miss
    for (int i = 0; i < 20; ++i) {
        profiler.record_slot(0, SlotType::FrontEndL1IMiss);
    }

    // 10 cycles of Backend L1D Miss
    for (int i = 0; i < 20; ++i) {
        profiler.record_slot(0, SlotType::BackEndMemL1DMiss);
    }

    const auto& report = profiler.get_report(0);
    EXPECT_EQ(report.frontend_slots, 20);
    EXPECT_EQ(report.fe_l1i_miss, 20);
    EXPECT_EQ(report.backend_slots, 20);
    EXPECT_EQ(report.be_mem_l1d_miss, 20);
    EXPECT_EQ(report.retiring_slots, 0);
    EXPECT_EQ(report.bad_spec_slots, 0);
}

// 3. Bad speculation slot accounting on branch mispredict
TEST(RobTopDownUBenchTest, TopDown_UBench_BadSpeculationAccounting) {
    TopDownProfiler profiler;
    profiler.init(1, 4);

    // Pipeline depth 14, 4-way issue = 56 flushed bad speculation slots
    for (int i = 0; i < 56; ++i) {
        profiler.record_slot(0, SlotType::BadSpecMispredict);
    }
    profiler.record_event(0, PerfEvent::BranchMispredict);

    const auto& report = profiler.get_report(0);
    EXPECT_EQ(report.bad_spec_slots, 56);
    EXPECT_EQ(report.event_branch_mispredicts, 1);
    EXPECT_DOUBLE_EQ(report.bad_spec_pct(), 100.0);
}

// 4. ROI boundary reset and dump trigger isolation via m5ops
TEST(RobTopDownUBenchTest, ROB_UBench_RoiBoundaryResetStats) {
    TopDownProfiler profiler;
    profiler.init(1, 2);

    // Phase 1: Startup initialization (outside ROI)
    for (int i = 0; i < 50; ++i) {
        profiler.record_slot(0, SlotType::FrontEndFetchBubble);
    }
    EXPECT_EQ(profiler.get_report(0).total_slots, 50);

    // m5op 0x50: Reset stats at start of ROI
    profiler.trigger_m5op(0x50);
    EXPECT_EQ(profiler.get_report(0).total_slots, 0);

    // Phase 2: ROI kernel execution
    for (int i = 0; i < 100; ++i) {
        profiler.record_slot(0, SlotType::RetiringBaseAlu);
    }
    EXPECT_EQ(profiler.get_report(0).total_slots, 100);
    EXPECT_EQ(profiler.get_report(0).retiring_slots, 100);

    // m5op 0x51: Dump stats
    profiler.trigger_m5op(0x51);

    // m5op 0x52: Exit ROI
    profiler.trigger_m5op(0x52);
    EXPECT_FALSE(profiler.is_roi_active());

    // Additional slots after ROI should be ignored
    profiler.record_slot(0, SlotType::RetiringBaseAlu);
    EXPECT_EQ(profiler.get_report(0).total_slots, 100);
}

// 5. Pareto bottleneck ranking accurately identifies the dominant limiter
TEST(RobTopDownUBenchTest, TopDown_UBench_BottleneckParetoRanking) {
    TopDownProfiler profiler;
    profiler.init(1, 4);

    // Dominant: L1D Cache Miss (60 slots)
    for (int i = 0; i < 60; ++i) profiler.record_slot(0, SlotType::BackEndMemL1DMiss);

    // Second: Branch Mispredict (30 slots)
    for (int i = 0; i < 30; ++i) profiler.record_slot(0, SlotType::BadSpecMispredict);

    // Third: Retiring (10 slots)
    for (int i = 0; i < 10; ++i) profiler.record_slot(0, SlotType::RetiringBaseAlu);

    auto bottlenecks = profiler.get_top_bottlenecks(0);
    ASSERT_GE(bottlenecks.size(), 2);
    EXPECT_EQ(bottlenecks[0].name, "Back-End Memory / L1D Miss");
    EXPECT_EQ(bottlenecks[0].slots, 60);
    EXPECT_DOUBLE_EQ(bottlenecks[0].percentage, 60.0);

    EXPECT_EQ(bottlenecks[1].name, "Bad Speculation / Branch Mispredict");
    EXPECT_EQ(bottlenecks[1].slots, 30);
    EXPECT_DOUBLE_EQ(bottlenecks[1].percentage, 30.0);
}
