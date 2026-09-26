#include <gtest/gtest.h>
#include "tinyarmsim/uarch/branch_predictor.hpp"
#include "tinyarmsim/uarch/fetch_unit.hpp"
#include "tinyarmsim/uarch/uop_decoder.hpp"
#include "tinyarmsim/uarch/rat_prf.hpp"
#include "tinyarmsim/uarch/rob_issue_queue.hpp"
#include "tinyarmsim/memory_bus.hpp"

using namespace tinyarmsim;
using namespace tinyarmsim::uarch;

// =============================================================================
// BPU Isolated Microbenchmarks (BPU_UBench)
// =============================================================================

// 1. Tight loop always taken branch prediction saturation and steady-state accuracy
TEST(BpuFrontendUBenchTest, BPU_UBench_TightLoopAlwaysTaken) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::GSHARE;
    cfg.table_size = 1024;
    cfg.btb_size = 512;
    cfg.ras_size = 16;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t loop_branch_pc = 0x1000;
    const uint32_t loop_target_pc = 0x0FE0;

    // Warm up BTB and GShare
    for (int i = 0; i < 5; ++i) {
        auto pred = bpu.predict(loop_branch_pc);
        bpu.update(loop_branch_pc, true, loop_target_pc, BranchType::DIRECT_COND, pred);
    }

    // Steady state: 100 consecutive predictions must be 100% taken with exact target
    for (int i = 0; i < 100; ++i) {
        auto pred = bpu.predict(loop_branch_pc);
        EXPECT_TRUE(pred.is_branch);
        EXPECT_TRUE(pred.taken);
        EXPECT_EQ(pred.target_pc, loop_target_pc);
        bpu.update(loop_branch_pc, true, loop_target_pc, BranchType::DIRECT_COND, pred);
    }
}

// 2. Alternating (TNTN) pattern to test GShare global history correlation
TEST(BpuFrontendUBenchTest, BPU_UBench_AlternatingPatternTNTN) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::GSHARE;
    cfg.table_size = 2048;
    cfg.btb_size = 512;
    cfg.ras_size = 16;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t branch_pc = 0x2000;
    const uint32_t target_pc = 0x2040;

    // Train on alternating T, NT, T, NT pattern
    for (int i = 0; i < 40; ++i) {
        bool actual_taken = (i % 2 == 0);
        auto pred = bpu.predict(branch_pc);
        bpu.update(branch_pc, actual_taken, actual_taken ? target_pc : branch_pc + 4, BranchType::DIRECT_COND, pred);
    }

    // GShare should learn history-dependent alternating outcomes
    int correct_predictions = 0;
    for (int i = 0; i < 20; ++i) {
        bool actual_taken = (i % 2 == 0);
        auto pred = bpu.predict(branch_pc);
        if (pred.taken == actual_taken) {
            correct_predictions++;
        }
        bpu.update(branch_pc, actual_taken, actual_taken ? target_pc : branch_pc + 4, BranchType::DIRECT_COND, pred);
    }
    EXPECT_GE(correct_predictions, 15);
}

// 3. Deeply nested Call/Return sequence testing Return Address Stack (RAS) wrap-around
TEST(BpuFrontendUBenchTest, BPU_UBench_DeepNestedCallReturnRAS) {
    ReturnAddressStack ras(16);
    std::vector<uint32_t> return_stack;
    const size_t num_calls = 32; // Exceeds 16-entry RAS depth

    // Push 32 nested calls
    for (size_t i = 0; i < num_calls; ++i) {
        uint32_t call_pc = 0x4000 + static_cast<uint32_t>(i * 0x20);
        uint32_t ret_pc = call_pc + 4;
        ras.push(ret_pc);
        return_stack.push_back(ret_pc);
    }

    EXPECT_EQ(ras.size(), 16);

    // Pop returns (most recent 16 must match exactly in LIFO order)
    for (size_t i = 0; i < 16; ++i) {
        uint32_t expected_ret = return_stack.back();
        return_stack.pop_back();

        uint32_t popped_pc = 0;
        bool success = ras.pop(popped_pc);
        EXPECT_TRUE(success);
        EXPECT_EQ(popped_pc, expected_ret);
    }
    EXPECT_EQ(ras.size(), 0);
}

// 4. Polymorphic indirect branch target switching stress on BTB
TEST(BpuFrontendUBenchTest, BPU_UBench_IndirectCallTargetThrashing) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::BIMODAL;
    cfg.btb_size = 512;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t indirect_branch_pc = 0x5000;
    const uint32_t target_A = 0x6000;
    const uint32_t target_B = 0x7000;

    // Train on Target A
    for (int i = 0; i < 5; ++i) {
        auto pred = bpu.predict(indirect_branch_pc);
        bpu.update(indirect_branch_pc, true, target_A, BranchType::INDIRECT_CALL, pred);
    }
    auto predA = bpu.predict(indirect_branch_pc);
    EXPECT_TRUE(predA.taken);
    EXPECT_EQ(predA.target_pc, target_A);

    // Switch to Target B
    for (int i = 0; i < 5; ++i) {
        auto pred = bpu.predict(indirect_branch_pc);
        bpu.update(indirect_branch_pc, true, target_B, BranchType::INDIRECT_CALL, pred);
    }
    auto predB = bpu.predict(indirect_branch_pc);
    EXPECT_TRUE(predB.taken);
    EXPECT_EQ(predB.target_pc, target_B);
}

// 5. Correlated branch patterns on TAGE multi-table geometric history
TEST(BpuFrontendUBenchTest, BPU_UBench_CorrelatedBranchesTAGE) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::TAGE;
    cfg.tage_tables = 4;
    cfg.btb_size = 512;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t br1 = 0x3000;
    const uint32_t br2 = 0x3010;

    // br2 outcome is correlated with br1: if br1 Taken -> br2 NotTaken; if br1 NotTaken -> br2 Taken
    for (int i = 0; i < 60; ++i) {
        bool t1 = (i % 3 == 0);
        bool t2 = !t1;

        auto p1 = bpu.predict(br1);
        bpu.update(br1, t1, t1 ? 0x3040 : br1 + 4, BranchType::DIRECT_COND, p1);

        auto p2 = bpu.predict(br2);
        bpu.update(br2, t2, t2 ? 0x3080 : br2 + 4, BranchType::DIRECT_COND, p2);
    }

    int accurate_count = 0;
    for (int i = 0; i < 20; ++i) {
        bool t1 = (i % 3 == 0);
        bool t2 = !t1;

        auto p1 = bpu.predict(br1);
        bpu.update(br1, t1, t1 ? 0x3040 : br1 + 4, BranchType::DIRECT_COND, p1);

        auto p2 = bpu.predict(br2);
        if (p2.taken == t2) accurate_count++;
        bpu.update(br2, t2, t2 ? 0x3080 : br2 + 4, BranchType::DIRECT_COND, p2);
    }
    EXPECT_GE(accurate_count, 15);
}

// 6. BTB hash index aliasing stress
TEST(BpuFrontendUBenchTest, BPU_UBench_BranchTargetBufferAliasStress) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::GSHARE;
    cfg.btb_size = 128;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t pc1 = 0x1000;
    const uint32_t pc2 = 0x1200;
    const uint32_t target1 = 0x2000;
    const uint32_t target2 = 0x3000;

    auto p1 = bpu.predict(pc1);
    bpu.update(pc1, true, target1, BranchType::DIRECT_UNCOND, p1);

    auto p2 = bpu.predict(pc2);
    bpu.update(pc2, true, target2, BranchType::DIRECT_UNCOND, p2);

    // Tag matching should distinguish pc2 from pc1
    auto pred2 = bpu.predict(pc2);
    EXPECT_TRUE(pred2.is_branch);
    EXPECT_TRUE(pred2.taken);
    EXPECT_EQ(pred2.target_pc, target2);
}

// =============================================================================
// Frontend Isolated Microbenchmarks (Frontend_UBench)
// =============================================================================

// 1. Fetch cross 64-byte cache line boundary with mixed 16/32-bit Thumb instructions
TEST(BpuFrontendUBenchTest, Frontend_UBench_CrossCacheLineFetch) {
    MemoryBus bus(4096);
    bus.write16(0x3C, 0xbf00);     // 16-bit NOP
    bus.write16(0x3E, 0xf240);     // 32-bit MOVW r0, #42 (0xF240 0x002A spans 0x3E -> 0x40 cacheline boundary)
    bus.write16(0x40, 0x002a);
    bus.write16(0x42, 0xbf00);     // 16-bit NOP

    CoreConfig core_cfg;
    core_cfg.fetch_width = 4;
    BranchPredictorConfig bp_cfg;
    bp_cfg.type = PredictorType::NONE;

    FetchUnit fetch_unit(0x3C, bus, nullptr, core_cfg, bp_cfg);

    fetch_unit.tick();
    EXPECT_TRUE(fetch_unit.has_uops());

    auto uop1 = fetch_unit.pop_uop();
    EXPECT_EQ(uop1.pc, 0x3C);
    EXPECT_FALSE(uop1.is_thumb32);

    auto uop2 = fetch_unit.pop_uop();
    EXPECT_EQ(uop2.pc, 0x3E);
    EXPECT_TRUE(uop2.is_thumb32);
}

// 2. RAT / FreeList allocation burst, exhaustion stall, and commit recovery
TEST(BpuFrontendUBenchTest, Frontend_UBench_PrfExhaustionStall) {
    const size_t num_arch_regs = 17;
    const size_t num_phys_regs = 36; // 36 - 17 = 19 speculative physical registers available
    RegisterAliasTable rat;
    PhysicalRegisterFile prf(num_phys_regs);
    FreeList free_list(num_phys_regs, num_arch_regs);

    EXPECT_EQ(free_list.free_count(), 19);

    std::vector<uint16_t> allocated_phys;
    // Allocate all 19 physical registers
    for (int i = 0; i < 19; ++i) {
        EXPECT_TRUE(free_list.has_free());
        uint16_t p = free_list.allocate();
        allocated_phys.push_back(p);
        rat.set(static_cast<uint8_t>(i % 16), p);
    }

    // Now FreeList is completely exhausted
    EXPECT_FALSE(free_list.has_free());
    EXPECT_THROW(static_cast<void>(free_list.allocate()), std::runtime_error);

    // Commit and free 5 registers
    for (size_t i = 0; i < 5; ++i) {
        free_list.free(allocated_phys[i]);
    }
    EXPECT_EQ(free_list.free_count(), 5);
    EXPECT_NO_THROW(static_cast<void>(free_list.allocate()));
}

// 3. Flags register renaming and dependency tracking (CMP -> BNE)
TEST(BpuFrontendUBenchTest, Frontend_UBench_FlagsRenamingWakeup) {
    RegisterAliasTable rat;
    PhysicalRegisterFile prf(32);
    FreeList free_list(32, 17);

    // Instruction 1: CMP r0, #0 -> sets flags
    uint16_t flags_p1 = free_list.allocate();
    rat.set(UOp::ARCH_REG_FLAGS, flags_p1);
    prf.set_ready(flags_p1, false);

    // Instruction 2: BNE label -> reads flags
    uint16_t bne_flags_src = rat.get(UOp::ARCH_REG_FLAGS);
    EXPECT_EQ(bne_flags_src, flags_p1);
    EXPECT_FALSE(prf.is_ready(bne_flags_src));

    // Instruction 1 completes and writes flags
    prf.write(flags_p1, 0x40000000); // Z flag
    EXPECT_TRUE(prf.is_ready(bne_flags_src));
}

// 4. Multi-uop expansion: PUSH {r4, r5, lr} expands into SP decrement and store uops
TEST(BpuFrontendUBenchTest, Frontend_UBench_MultiUopExpansionThroughput) {
    DecodedInstruction instr;
    instr.op = Opcode::PUSH;
    instr.register_list = (1 << 4) | (1 << 5) | (1 << 14); // R4, R5, LR
    instr.instr_size = 2;

    auto uops = UOpDecoder::decode(instr, 0x1000, 1);
    // 1 SUB SP uop + 3 registers * 2 uops (STA + STD) = 7 uops
    ASSERT_EQ(uops.size(), 7);
    EXPECT_EQ(uops[0].type, UOpType::ALU);
    EXPECT_EQ(uops[0].opcode, Opcode::SUB);
    EXPECT_EQ(uops[0].arch_dest, 13); // SP

    for (size_t i = 1; i < uops.size(); ++i) {
        EXPECT_EQ(uops[i].pc, 0x1000);
        EXPECT_TRUE(uops[i].type == UOpType::STORE_ADDR || uops[i].type == UOpType::STORE_DATA);
    }
}

// 5. Speculative RAT checkpoint restore on branch misprediction
TEST(BpuFrontendUBenchTest, Frontend_UBench_SpeculativeCheckpointRestore) {
    RegisterAliasTable rat;
    FreeList free_list(48, 17);

    // Initial architectural mapping: r0 -> p0, r1 -> p1
    EXPECT_EQ(rat.get(0), 0);
    EXPECT_EQ(rat.get(1), 1);

    // Take checkpoint before speculative branch
    auto checkpoint = rat.create_checkpoint();

    // Speculative branch path: renames r0 -> p17, r1 -> p18
    uint16_t p17 = free_list.allocate();
    uint16_t p18 = free_list.allocate();
    rat.set(0, p17);
    rat.set(1, p18);

    EXPECT_EQ(rat.get(0), p17);
    EXPECT_EQ(rat.get(1), p18);

    // Branch mispredicted! Restore checkpoint
    rat.restore_checkpoint(checkpoint);

    // Must be perfectly restored to p0 and p1
    EXPECT_EQ(rat.get(0), 0);
    EXPECT_EQ(rat.get(1), 1);
}

// 6. Decoder unknown / illegal opcode handling
TEST(BpuFrontendUBenchTest, Frontend_UBench_DecoderIllegalOpcodeFault) {
    DecodedInstruction instr;
    instr.op = Opcode::UNKNOWN;
    instr.instr_size = 2;

    auto uops = UOpDecoder::decode(instr, 0xDEAD, 42);
    ASSERT_EQ(uops.size(), 1);
    EXPECT_EQ(uops[0].type, UOpType::HALT);
    EXPECT_EQ(uops[0].pc, 0xDEAD);
}

// 7. Isolation Test: Non-conditional branches (BL calls and POP PC returns) must NOT pollute GHR
TEST(BpuFrontendUBenchTest, BPU_UBench_CallReturnPreservesConditionalGHR) {
    BranchPredictorConfig cfg;
    cfg.type = PredictorType::BIMODAL; // BiModeBP
    cfg.table_size = 2048;
    cfg.btb_size = 512;
    cfg.ras_size = 16;
    CompositeBranchPredictor bpu(cfg);

    const uint32_t cond_pc = 0x1000;
    const uint32_t cond_target = 0x1040;
    const uint32_t call_pc = 0x2004; // Index 1 in 512-entry BTB (cond_pc is Index 0)
    const uint32_t ret_pc = 0x3008;  // Index 2 in 512-entry BTB

    // 1. Train conditional branch on a standard loop pattern (8 Taken, 1 Not-Taken)
    for (int rep = 0; rep < 20; ++rep) {
        for (int i = 0; i < 8; ++i) {
            auto pred = bpu.predict(cond_pc, BranchType::DIRECT_COND, true);
            bpu.update(cond_pc, true, cond_target, BranchType::DIRECT_COND, pred);
            if (!pred.taken) {
                bpu.squash(pred, true);
            }
        }
        auto pred = bpu.predict(cond_pc, BranchType::DIRECT_COND, true);
        bpu.update(cond_pc, false, cond_pc + 4, BranchType::DIRECT_COND, pred);
        if (pred.taken) {
            bpu.squash(pred, false);
        }
    }

    // 2. Interleave 50 BL calls and 50 RET returns
    for (int i = 0; i < 50; ++i) {
        auto call_pred = bpu.predict(call_pc, BranchType::DIRECT_CALL, false);
        EXPECT_TRUE(call_pred.taken);
        bpu.update(call_pc, true, 0x2100, BranchType::DIRECT_CALL, call_pred);

        auto ret_pred = bpu.predict(ret_pc, BranchType::RETURN, false);
        bpu.update(ret_pc, true, 0x2004, BranchType::RETURN, ret_pred);
    }

    // 3. Conditional branch prediction must remain strongly TAKEN (accuracy > 99%)
    auto post_call_pred = bpu.predict(cond_pc, BranchType::DIRECT_COND, true);
    EXPECT_TRUE(post_call_pred.taken);
    EXPECT_EQ(post_call_pred.target_pc, cond_target);
}

