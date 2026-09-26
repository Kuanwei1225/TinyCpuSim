#include <gtest/gtest.h>
#include <sstream>
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/stats.hpp"

using namespace tinyarmsim::uarch;

TEST(UArchConfigTest, DefaultOooConfigValidatesSuccessfully) {
    UArchConfig cfg = UArchConfig::make_ooo_default();
    EXPECT_NO_THROW(cfg.validate());
    EXPECT_EQ(cfg.num_cores, 1);
    EXPECT_TRUE(cfg.default_core.enable_ooo);
    EXPECT_EQ(cfg.default_core.fetch_width, 4);
    EXPECT_EQ(cfg.default_core.rob_size, 64);
    EXPECT_EQ(cfg.default_core.l1i.num_sets(), 128); // 32KB / (64B * 4) = 128 sets
}

TEST(UArchConfigTest, InOrderSimpleConfigValidates) {
    UArchConfig cfg = UArchConfig::make_in_order_simple();
    EXPECT_NO_THROW(cfg.validate());
    EXPECT_FALSE(cfg.default_core.enable_ooo);
    EXPECT_EQ(cfg.default_core.rob_size, 1);
    EXPECT_FALSE(cfg.default_core.l1i.enabled);
    EXPECT_FALSE(cfg.enable_mesi_coherence);
}

TEST(UArchConfigTest, InvalidCacheGeometryThrowsException) {
    UArchConfig cfg;
    cfg.default_core.l1i.size_bytes = 1000; // Not a power of 2
    EXPECT_THROW(cfg.default_core.l1i.validate(), std::invalid_argument);

    cfg.default_core.l1i.size_bytes = 1024;
    cfg.default_core.l1i.line_size = 128;
    cfg.default_core.l1i.associativity = 16; // 128 * 16 = 2048 > 1024
    EXPECT_THROW(cfg.default_core.l1i.validate(), std::invalid_argument);
}

TEST(UArchConfigTest, ParseKvConfigurationWithCommentsAndSections) {
    std::string config_content = R"(
# Multi-Core Simulation Config
[global]
num_cores = 2
enable_mesi = true
dram_latency = 100

[core]
enable_ooo = true
fetch_width = 8
rob_size = 128
rs_size = 64
num_phys_regs = 256

[l1d]
enabled = true
size_bytes = 65536
associativity = 8
hit_latency = 2

[branch_predictor]
enabled = true
type = TAGE
table_size = 8192
)";

    std::istringstream iss(config_content);
    UArchConfig cfg = UArchConfig::parse_kv(iss);

    EXPECT_EQ(cfg.num_cores, 2);
    EXPECT_TRUE(cfg.enable_mesi_coherence);
    EXPECT_EQ(cfg.dram_latency_cycles, 100);
    EXPECT_EQ(cfg.default_core.fetch_width, 8);
    EXPECT_EQ(cfg.default_core.rob_size, 128);
    EXPECT_EQ(cfg.default_core.num_phys_regs, 256);
    EXPECT_EQ(cfg.default_core.l1d.size_bytes, 65536);
    EXPECT_EQ(cfg.default_core.l1d.associativity, 8);
    EXPECT_EQ(cfg.default_core.branch_predictor.type, PredictorType::TAGE);
    EXPECT_EQ(cfg.default_core.branch_predictor.table_size, 8192);
}

TEST(UArchStatsTest, CorrectlyCalculatesRatesAndFormatsReport) {
    UArchStats stats;
    stats.total_simulated_cycles = 1000;
    
    CoreStats c0;
    c0.cycles = 1000;
    c0.committed_instructions = 1500;
    c0.branch.record_prediction(true);
    c0.branch.record_prediction(true);
    c0.branch.record_prediction(false);
    c0.l1d.record_access(true);
    c0.l1d.record_access(false);
    stats.cores.push_back(c0);

    EXPECT_DOUBLE_EQ(stats.total_ipc(), 1.5);
    EXPECT_DOUBLE_EQ(c0.branch.accuracy(), 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(c0.l1d.hit_rate(), 0.5);

    std::string report = stats.format_text();
    EXPECT_NE(report.find("Aggregate Throughput (IPC):1.500"), std::string::npos);
    EXPECT_NE(report.find("[ Core 0"), std::string::npos);
}
