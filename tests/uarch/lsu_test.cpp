#include <gtest/gtest.h>
#include "tinyarmsim/uarch/lsu.hpp"

using namespace tinyarmsim::uarch;

TEST(LsuTest, LoadAndStoreQueueAllocationAndCapacity) {
    LsuConfig cfg;
    cfg.lq_size = 4;
    cfg.sq_size = 4;

    LoadStoreUnit lsu(cfg, nullptr);

    EXPECT_TRUE(lsu.can_allocate_load());
    EXPECT_TRUE(lsu.can_allocate_store());

    UOp load_uop; load_uop.seq_num = 1;
    size_t lq0 = lsu.allocate_load(load_uop);
    EXPECT_EQ(lq0, 0);

    UOp store_uop; store_uop.seq_num = 2;
    size_t sq0 = lsu.allocate_store(store_uop);
    EXPECT_EQ(sq0, 0);

    lsu.free_load(lq0);
    EXPECT_TRUE(lsu.can_allocate_load());
}

TEST(LsuTest, StoreToLoadForwardingExactAddressMatch) {
    LsuConfig cfg;
    cfg.type = LsuType::SPECULATIVE_OOO;
    cfg.store_forward_latency = 1;

    LoadStoreUnit lsu(cfg, nullptr);

    // 1. Older Store: STR R0, [0x1000] -> Store value 0x12345678
    UOp store_uop;
    store_uop.seq_num = 10;
    store_uop.rob_idx = 1;
    size_t sq_idx = lsu.allocate_store(store_uop);

    size_t violation_rob = 0;
    lsu.execute_store_address(sq_idx, 0x1000, 4, 10, violation_rob);
    lsu.execute_store_data(sq_idx, 0x12345678);

    // 2. Younger Load: LDR R1, [0x1000]
    UOp load_uop;
    load_uop.seq_num = 11;
    load_uop.rob_idx = 2;
    size_t lq_idx = lsu.allocate_load(load_uop);

    auto load_res = lsu.execute_load(lq_idx, 0x1000, 4, 11);
    EXPECT_TRUE(load_res.completed);
    EXPECT_TRUE(load_res.forwarded);
    EXPECT_EQ(load_res.data, 0x12345678);
    EXPECT_EQ(load_res.latency_cycles, 1);
}

TEST(LsuTest, MemoryOrderBufferViolationDetection) {
    LsuConfig cfg;
    LoadStoreUnit lsu(cfg, nullptr);

    // 1. Older Store allocated at seq_num = 20 (Address NOT resolved yet)
    UOp store_uop;
    store_uop.seq_num = 20;
    store_uop.rob_idx = 5;
    size_t sq_idx = lsu.allocate_store(store_uop);

    // 2. Younger Speculative Load executes at seq_num = 21 from 0x2000
    UOp load_uop;
    load_uop.seq_num = 21;
    load_uop.rob_idx = 6;
    size_t lq_idx = lsu.allocate_load(load_uop);
    auto load_res = lsu.execute_load(lq_idx, 0x2000, 4, 21);
    EXPECT_TRUE(load_res.completed);
    EXPECT_FALSE(load_res.forwarded);

    // 3. Older Store Address resolves to 0x2000! (RAW Collision Violation!)
    size_t violating_rob_idx = 0;
    bool violation = lsu.execute_store_address(sq_idx, 0x2000, 4, 20, violating_rob_idx);
    EXPECT_TRUE(violation);
    EXPECT_EQ(violating_rob_idx, 6); // Detected load at ROB 6 violated memory order!
}
