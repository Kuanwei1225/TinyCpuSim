#include <gtest/gtest.h>
#include "tinyarmsim/uarch/branch_predictor.hpp"

using namespace tinyarmsim::uarch;

TEST(BranchPredictorTest, SaturatingCounter2BitTransitionsAndSaturation) {
    SaturatingCounter2Bit ctr(2); // Weakly Taken
    EXPECT_TRUE(ctr.is_taken());
    EXPECT_EQ(ctr.get(), 2);

    ctr.update(true); // -> 3 (Strongly Taken)
    EXPECT_TRUE(ctr.is_taken());
    EXPECT_EQ(ctr.get(), 3);

    ctr.update(true); // Saturation at 3
    EXPECT_EQ(ctr.get(), 3);

    ctr.update(false); // -> 2 (Weakly Taken)
    EXPECT_TRUE(ctr.is_taken());
    EXPECT_EQ(ctr.get(), 2);

    ctr.update(false); // -> 1 (Weakly Not Taken)
    EXPECT_FALSE(ctr.is_taken());
    EXPECT_EQ(ctr.get(), 1);

    ctr.update(false); // -> 0 (Strongly Not Taken)
    EXPECT_FALSE(ctr.is_taken());
    EXPECT_EQ(ctr.get(), 0);

    ctr.update(false); // Saturation at 0
    EXPECT_EQ(ctr.get(), 0);
}

TEST(BranchPredictorTest, BimodalPredictorTrainingOnAlternatingBranches) {
    BimodalPredictor bimodal(512);
    uint32_t branch_pc = 0x1000;

    // Initially Weakly Taken (default = 2)
    EXPECT_TRUE(bimodal.predict(branch_pc));

    // Train heavily Not Taken
    bimodal.update(branch_pc, false);
    bimodal.update(branch_pc, false);
    EXPECT_FALSE(bimodal.predict(branch_pc));

    // Train back to Taken
    bimodal.update(branch_pc, true);
    bimodal.update(branch_pc, true);
    EXPECT_TRUE(bimodal.predict(branch_pc));
}

TEST(BranchPredictorTest, GSharePredictorCorrelatesGlobalHistory) {
    GSharePredictor gshare(1024, 8);
    uint32_t pc1 = 0x2000;

    // Pattern: T, N, T, N
    for (int i = 0; i < 10; ++i) {
        gshare.update(pc1, true);
        gshare.update(pc1, false);
    }

    // When history ends in 1 (after Taken), predictor predicts Not-Taken (false)
    gshare.restore_history(0b01010101);
    EXPECT_FALSE(gshare.predict(pc1));

    // When history ends in 0 (after Not-Taken), predictor predicts Taken (true)
    gshare.restore_history(0b01010100);
    EXPECT_TRUE(gshare.predict(pc1));
}

TEST(BranchPredictorTest, TagePredictorGeometricHistoryMatching) {
    TagePredictor tage(1024, 4, 512);
    uint32_t pc = 0x3000;

    // Train loop branch with 4-iteration pattern
    for (int loop = 0; loop < 20; ++loop) {
        for (int iter = 0; iter < 3; ++iter) {
            auto pred = tage.predict(pc);
            tage.update(pc, true, pred);
        }
        auto exit_pred = tage.predict(pc);
        tage.update(pc, false, exit_pred);
    }

    // Verify reset
    tage.reset();
    auto initial_pred = tage.predict(pc);
    EXPECT_EQ(initial_pred.provider_table, -1); // Bimodal fallback
}

TEST(BranchPredictorTest, BranchTargetBufferLookupAndUpdate) {
    BranchTargetBuffer btb(512);
    uint32_t pc = 0x4000;
    uint32_t target = 0x4080;
    uint32_t out_target = 0;
    BranchType out_type = BranchType::DIRECT_COND;

    EXPECT_FALSE(btb.lookup(pc, out_target, out_type));

    btb.update(pc, target, BranchType::DIRECT_CALL);
    EXPECT_TRUE(btb.lookup(pc, out_target, out_type));
    EXPECT_EQ(out_target, target);
    EXPECT_EQ(out_type, BranchType::DIRECT_CALL);

    btb.reset();
    EXPECT_FALSE(btb.lookup(pc, out_target, out_type));
}

TEST(BranchPredictorTest, ReturnAddressStackPushPopAndWrapAround) {
    ReturnAddressStack ras(4);
    uint32_t out_pc = 0;

    EXPECT_FALSE(ras.pop(out_pc));
    EXPECT_EQ(ras.size(), 0);

    // Push 3 return PCs
    ras.push(0x1004);
    ras.push(0x2004);
    ras.push(0x3004);
    EXPECT_EQ(ras.size(), 3);

    EXPECT_TRUE(ras.peek(out_pc));
    EXPECT_EQ(out_pc, 0x3004);

    // Pop in LIFO order
    EXPECT_TRUE(ras.pop(out_pc));
    EXPECT_EQ(out_pc, 0x3004);
    EXPECT_TRUE(ras.pop(out_pc));
    EXPECT_EQ(out_pc, 0x2004);

    // Overflow wrap-around test
    ras.push(0x4004);
    ras.push(0x5004);
    ras.push(0x6004);
    ras.push(0x7004);
    ras.push(0x8004); // Overflows capacity=4
    EXPECT_EQ(ras.size(), 4);

    EXPECT_TRUE(ras.pop(out_pc));
    EXPECT_EQ(out_pc, 0x8004);
}

TEST(BranchPredictorTest, CompositeBranchPredictorCallAndReturnFlow) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::BIMODAL;
    cfg.btb_size = 512;
    cfg.ras_size = 16;

    CompositeBranchPredictor bp(cfg);

    uint32_t call_pc = 0x1000;
    uint32_t func_entry = 0x2000;
    uint32_t ret_pc = 0x2050;

    // Initial query before training: BTB miss
    auto pred1 = bp.predict(call_pc);
    EXPECT_FALSE(pred1.is_branch);

    // Train Call instruction
    bp.update(call_pc, true, func_entry, BranchType::DIRECT_CALL, pred1);

    // Query Call again -> BTB hit + RAS push
    auto pred2 = bp.predict(call_pc);
    EXPECT_TRUE(pred2.is_branch);
    EXPECT_TRUE(pred2.taken);
    EXPECT_EQ(pred2.target_pc, func_entry);

    // Train Return instruction at ret_pc
    BranchPrediction empty_pred;
    bp.update(ret_pc, true, call_pc + 4, BranchType::RETURN, empty_pred);

    // Query Return -> predicted by RAS to return to call_pc + 4
    auto ret_pred = bp.predict(ret_pc);
    EXPECT_TRUE(ret_pred.is_branch);
    EXPECT_TRUE(ret_pred.taken);
    EXPECT_EQ(ret_pred.target_pc, call_pc + 4);
}
