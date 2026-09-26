#include <gtest/gtest.h>
#include "tinyarmsim/uarch/topdown_profiler.hpp"
#include <sstream>
#include <fstream>

using namespace tinyarmsim::uarch;

TEST(TopDownProfilerTest, SlotConservationInvariantHolds) {
    TopDownProfiler profiler;
    profiler.init(1, 2); // 1 core, 2-wide issue
    profiler.start_roi();

    // Record 100 slots across different categories
    for (int i = 0; i < 40; ++i) profiler.record_slot(0, SlotType::RetiringBaseAlu);
    for (int i = 0; i < 10; ++i) profiler.record_slot(0, SlotType::RetiringMem);
    for (int i = 0; i < 15; ++i) profiler.record_slot(0, SlotType::BadSpecMispredict);
    for (int i = 0; i < 15; ++i) profiler.record_slot(0, SlotType::FrontEndL1IMiss);
    for (int i = 0; i < 10; ++i) profiler.record_slot(0, SlotType::BackEndCoreRSFull);
    for (int i = 0; i < 10; ++i) profiler.record_slot(0, SlotType::BackEndMemL1DMiss);

    auto rep = profiler.get_report(0);
    EXPECT_EQ(rep.total_slots, 100);
    EXPECT_EQ(rep.retiring_slots, 50);
    EXPECT_EQ(rep.bad_spec_slots, 15);
    EXPECT_EQ(rep.frontend_slots, 15);
    EXPECT_EQ(rep.backend_slots, 20);

    // Strict 100% conservation invariant
    EXPECT_EQ(rep.retiring_slots + rep.bad_spec_slots + rep.frontend_slots + rep.backend_slots, rep.total_slots);
    EXPECT_DOUBLE_EQ(rep.retiring_pct(), 50.0);
    EXPECT_DOUBLE_EQ(rep.bad_spec_pct(), 15.0);
    EXPECT_DOUBLE_EQ(rep.frontend_pct(), 15.0);
    EXPECT_DOUBLE_EQ(rep.backend_pct(), 20.0);
}

TEST(TopDownProfilerTest, TopBottleneckParetoRanking) {
    TopDownProfiler profiler;
    profiler.init(1, 2);
    profiler.start_roi();

    // 100 slots: 20 Retiring, 40 Mem L1D Miss (dominant), 25 BadSpec (2nd), 15 Core RS Full (3rd)
    for (int i = 0; i < 20; ++i) profiler.record_slot(0, SlotType::RetiringBaseAlu);
    for (int i = 0; i < 40; ++i) profiler.record_slot(0, SlotType::BackEndMemL1DMiss);
    for (int i = 0; i < 25; ++i) profiler.record_slot(0, SlotType::BadSpecMispredict);
    for (int i = 0; i < 15; ++i) profiler.record_slot(0, SlotType::BackEndCoreRSFull);

    auto bottlenecks = profiler.get_top_bottlenecks(0);
    ASSERT_GE(bottlenecks.size(), 3);
    EXPECT_EQ(bottlenecks[0].name, "Back-End Memory / L1D Miss");
    EXPECT_DOUBLE_EQ(bottlenecks[0].percentage, 40.0);
    EXPECT_EQ(bottlenecks[1].name, "Bad Speculation / Branch Mispredict");
    EXPECT_DOUBLE_EQ(bottlenecks[1].percentage, 25.0);
    EXPECT_EQ(bottlenecks[2].name, "Back-End Core / RS Full");
    EXPECT_DOUBLE_EQ(bottlenecks[2].percentage, 15.0);
}

TEST(TopDownProfilerTest, MultiFormatExportGeneratesValidOutputs) {
    TopDownProfiler profiler;
    profiler.init(1, 2);
    profiler.start_roi();

    profiler.record_slot(0, SlotType::RetiringBaseAlu);
    profiler.record_slot(0, SlotType::BackEndMemL1DMiss);
    profiler.record_event(0, PerfEvent::StoreLoadForwardHit);

    // Text format check
    std::string text = profiler.format_text();
    EXPECT_NE(text.find("TinyArmSim Top-Down"), std::string::npos);
    EXPECT_NE(text.find("Retiring"), std::string::npos);

    // JSON format check
    std::string json = profiler.format_json();
    EXPECT_NE(json.find("\"total_slots\": 2"), std::string::npos);
    EXPECT_NE(json.find("\"retiring_slots\": 1"), std::string::npos);

    // CSV format check
    std::string csv = profiler.format_csv();
    EXPECT_NE(csv.find("core_id,total_slots,retiring,bad_spec,frontend,backend"), std::string::npos);
    EXPECT_NE(csv.find("0,2,1,0,0,1"), std::string::npos);
}
