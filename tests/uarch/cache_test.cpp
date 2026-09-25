#include <gtest/gtest.h>
#include "tinyarmsim/uarch/cache.hpp"

using namespace tinyarmsim::uarch;

TEST(CacheTest, AddressBitSplittingAndGeometry) {
    CacheConfig cfg;
    cfg.size_bytes = 32768; // 32KB
    cfg.line_size = 64;     // 64B -> offset = 6 bits
    cfg.associativity = 4;  // 4-way -> 128 sets -> index = 7 bits
    Cache cache(cfg, "L1D");

    EXPECT_EQ(cache.get_offset_bits(), 6);
    EXPECT_EQ(cache.get_index_bits(), 7);
    EXPECT_EQ(cache.get_num_sets(), 128);

    uint32_t addr = 0x12345678;
    // Offset: lowest 6 bits (0x78 & 0x3F = 0x38)
    EXPECT_EQ(cache.extract_offset(addr), 0x38);
    // Index: bits [12:6]
    EXPECT_EQ(cache.extract_index(addr), (0x12345678 >> 6) & 0x7F);
    // Tag: bits [31:13]
    EXPECT_EQ(cache.extract_tag(addr), 0x12345678 >> 13);
}

TEST(CacheTest, ColdMissThenHitOnSameLine) {
    CacheConfig cfg;
    cfg.size_bytes = 1024;
    cfg.line_size = 64;
    cfg.associativity = 2;
    Cache cache(cfg, "TestCache");

    // First access to 0x1000 is a cold miss
    auto res1 = cache.access(0x1000, false);
    EXPECT_FALSE(res1.hit);
    EXPECT_FALSE(res1.evicted);
    EXPECT_EQ(cache.get_stats().misses, 1);

    // Second access to 0x1000 is a hit
    auto res2 = cache.access(0x1000, false);
    EXPECT_TRUE(res2.hit);
    EXPECT_EQ(cache.get_stats().hits, 1);

    // Third access to 0x1020 (same 64B cache line) is also a hit!
    auto res3 = cache.access(0x1020, false);
    EXPECT_TRUE(res3.hit);
    EXPECT_EQ(cache.get_stats().hits, 2);
}

TEST(CacheTest, LruEvictionSequenceUnderSetPressure) {
    CacheConfig cfg;
    cfg.size_bytes = 256;  // 256B total
    cfg.line_size = 64;    // 64B line
    cfg.associativity = 2; // 2-way -> 2 sets
    cfg.replacement = ReplacementPolicy::LRU;
    Cache cache(cfg, "2WayLru");

    // Addresses mapping to Set 0: (index bits = 1, bit 6)
    // 0x000 (Set 0), 0x080 (Set 0), 0x100 (Set 0)
    uint32_t a = 0x000;
    uint32_t b = 0x080;
    uint32_t c = 0x100;

    EXPECT_EQ(cache.extract_index(a), 0);
    EXPECT_EQ(cache.extract_index(b), 0);
    EXPECT_EQ(cache.extract_index(c), 0);

    // Fill the 2 ways of Set 0 with A and B
    cache.access(a, false); // Miss, Way 0 = A
    cache.access(b, false); // Miss, Way 1 = B

    // Access A again to make B the Least Recently Used
    cache.access(a, false); // Hit A, timestamp(A) > timestamp(B)

    // Now access C -> Must evict B (not A)
    auto res_c = cache.access(c, false);
    EXPECT_FALSE(res_c.hit);
    EXPECT_TRUE(res_c.evicted);
    EXPECT_EQ(res_c.evicted_addr, b);

    // A should still be in cache!
    auto res_a = cache.access(a, false);
    EXPECT_TRUE(res_a.hit);
}

TEST(CacheTest, DirtyWriteBackTrackingAndFlush) {
    CacheConfig cfg;
    cfg.size_bytes = 128;
    cfg.line_size = 64;
    cfg.associativity = 1; // Direct mapped (2 sets)
    cfg.write_policy = WritePolicy::WRITE_BACK;
    Cache cache(cfg, "DirectMappedWB");

    uint32_t a = 0x000; // Set 0
    uint32_t b = 0x080; // Set 0 (conflicts with a)

    // Write to A -> Miss, allocates, sets dirty bit
    auto res1 = cache.access(a, true);
    EXPECT_FALSE(res1.hit);
    EXPECT_FALSE(res1.evicted);

    // Access B -> Evicts A (A is dirty!)
    auto res2 = cache.access(b, false);
    EXPECT_FALSE(res2.hit);
    EXPECT_TRUE(res2.evicted);
    EXPECT_TRUE(res2.evicted_dirty);
    EXPECT_EQ(res2.evicted_addr, a);
    EXPECT_EQ(cache.get_stats().writebacks, 1);

    // Write to B and then flush cache
    cache.access(b, true);
    auto flushed = cache.flush();
    EXPECT_EQ(flushed.size(), 1);
    EXPECT_EQ(flushed[0], b);
}

TEST(CacheTest, InvalidateLineRemovesEntry) {
    CacheConfig cfg;
    cfg.size_bytes = 512;
    cfg.line_size = 64;
    cfg.associativity = 4;
    Cache cache(cfg, "SnoopTest");

    cache.access(0x2000, false);
    EXPECT_TRUE(cache.probe(0x2000));

    // Invalidate 0x2000
    bool inv_ok = cache.invalidate_line(0x2000);
    EXPECT_TRUE(inv_ok);
    EXPECT_FALSE(cache.probe(0x2000));
    EXPECT_EQ(cache.get_stats().invalidations, 1);

    // Next access to 0x2000 must be a miss
    auto res = cache.access(0x2000, false);
    EXPECT_FALSE(res.hit);
}

TEST(CacheTest, BypassModeReturnsZeroLatencyHits) {
    CacheConfig cfg;
    cfg.enabled = false;
    Cache cache(cfg, "BypassedCache");

    auto res = cache.access(0x1234, false);
    EXPECT_TRUE(res.hit);
    EXPECT_EQ(res.latency_cycles, 0);
}
