#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace tinyarmsim::uarch {

enum class ReplacementPolicy {
    LRU,
    FIFO,
    RANDOM
};

enum class WritePolicy {
    WRITE_BACK,
    WRITE_THROUGH
};

enum class PredictorType {
    IDEAL,
    BIMODAL,
    GSHARE,
    TAGE
};

struct CacheConfig {
    bool enabled{true};
    size_t size_bytes{32768};        // Default: 32KB
    size_t line_size{64};            // Default: 64B
    size_t associativity{4};         // Default: 4-way
    uint32_t hit_latency_cycles{1};  // Default: 1 cycle
    ReplacementPolicy replacement{ReplacementPolicy::LRU};
    WritePolicy write_policy{WritePolicy::WRITE_BACK};
    size_t mshr_entries{8};          // Non-blocking MSHR capacity

    [[nodiscard]] size_t num_sets() const noexcept {
        if (line_size == 0 || associativity == 0) return 0;
        return size_bytes / (line_size * associativity);
    }

    void validate() const {
        if (!enabled) return;
        if (size_bytes == 0 || (size_bytes & (size_bytes - 1)) != 0) {
            throw std::invalid_argument("Cache size_bytes must be a non-zero power of 2");
        }
        if (line_size == 0 || (line_size & (line_size - 1)) != 0) {
            throw std::invalid_argument("Cache line_size must be a non-zero power of 2");
        }
        if (associativity == 0 || (associativity & (associativity - 1)) != 0) {
            throw std::invalid_argument("Cache associativity must be a non-zero power of 2");
        }
        if (size_bytes < line_size * associativity) {
            throw std::invalid_argument("Cache size_bytes must be >= line_size * associativity");
        }
    }
};

struct BranchPredictorConfig {
    bool enabled{true};
    PredictorType type{PredictorType::TAGE};
    size_t table_size{4096};
    size_t btb_size{4096};
    size_t ras_size{32};
    size_t tage_tables{4};

    void validate() const {
        if (!enabled) return;
        if (table_size == 0 || (table_size & (table_size - 1)) != 0) {
            throw std::invalid_argument("Predictor table_size must be a power of 2");
        }
        if (btb_size == 0 || (btb_size & (btb_size - 1)) != 0) {
            throw std::invalid_argument("BTB size must be a power of 2");
        }
    }
};

struct LsuConfig {
    bool enabled{true};
    size_t lq_size{16};
    size_t sq_size{16};
    bool enable_speculative_load{true};
    bool enable_store_forwarding{true};
    uint32_t store_forward_latency{1};
    uint32_t num_load_ports{2};
    uint32_t num_store_ports{1};

    void validate() const {
        if (!enabled) return;
        if (lq_size == 0 || sq_size == 0) {
            throw std::invalid_argument("LQ and SQ sizes must be > 0");
        }
    }
};

struct CoreConfig {
    bool enable_ooo{true};
    uint32_t fetch_width{4};
    uint32_t decode_width{4};
    uint32_t rename_width{4};
    uint32_t issue_width{4};
    uint32_t commit_width{4};
    size_t rob_size{64};
    size_t rs_size{32};
    size_t num_phys_regs{128};

    CacheConfig l1i{};
    CacheConfig l1d{};
    BranchPredictorConfig branch_predictor{};
    LsuConfig lsu{};

    void validate() const {
        if (fetch_width == 0 || decode_width == 0 || rename_width == 0 ||
            issue_width == 0 || commit_width == 0) {
            throw std::invalid_argument("Pipeline stage widths must be > 0");
        }
        if (enable_ooo) {
            if (rob_size == 0 || rs_size == 0) {
                throw std::invalid_argument("ROB and RS sizes must be > 0 when OoO is enabled");
            }
            if (num_phys_regs <= 16) {
                throw std::invalid_argument("num_phys_regs must be > 16 (architectural registers count)");
            }
        }
        l1i.validate();
        l1d.validate();
        branch_predictor.validate();
        lsu.validate();
    }
};

struct UArchConfig {
    size_t num_cores{1};
    CoreConfig default_core{};
    std::vector<CoreConfig> cores{};
    CacheConfig l2_shared{
        true,
        524288,   // 512KB
        64,       // 64B line
        8,        // 8-way
        10,       // 10 cycles hit latency
        ReplacementPolicy::LRU,
        WritePolicy::WRITE_BACK,
        16        // 16 MSHR entries
    };
    bool enable_mesi_coherence{true};
    uint32_t dram_latency_cycles{80};

    [[nodiscard]] const CoreConfig& get_core_config(size_t core_id) const noexcept {
        if (core_id < cores.size()) {
            return cores[core_id];
        }
        return default_core;
    }

    void validate() {
        if (num_cores == 0) {
            throw std::invalid_argument("num_cores must be >= 1");
        }
        default_core.validate();
        for (auto& c : cores) {
            c.validate();
        }
        l2_shared.validate();
    }

    static UArchConfig make_in_order_simple() {
        UArchConfig cfg;
        cfg.num_cores = 1;
        cfg.default_core.enable_ooo = false;
        cfg.default_core.fetch_width = 1;
        cfg.default_core.decode_width = 1;
        cfg.default_core.rename_width = 1;
        cfg.default_core.issue_width = 1;
        cfg.default_core.commit_width = 1;
        cfg.default_core.rob_size = 1;
        cfg.default_core.rs_size = 1;
        cfg.default_core.branch_predictor.type = PredictorType::BIMODAL;
        cfg.default_core.lsu.enabled = false;
        cfg.default_core.l1i.enabled = false;
        cfg.default_core.l1d.enabled = false;
        cfg.l2_shared.enabled = false;
        cfg.enable_mesi_coherence = false;
        return cfg;
    }

    static UArchConfig make_ooo_default() {
        UArchConfig cfg;
        cfg.num_cores = 1;
        cfg.validate();
        return cfg;
    }

    static UArchConfig make_multicore_default(size_t cores = 4) {
        UArchConfig cfg;
        cfg.num_cores = cores;
        cfg.cores.resize(cores, cfg.default_core);
        cfg.l2_shared.size_bytes = 1024 * 1024 * 2; // 2MB
        cfg.validate();
        return cfg;
    }

    // Key-Value style configuration parser (supports comments #, section headers [core], key=value)
    static UArchConfig parse_kv(std::istream& in) {
        UArchConfig cfg;
        std::string line;
        std::string current_section = "global";

        while (std::getline(in, line)) {
            // Strip comments
            auto hash_pos = line.find('#');
            if (hash_pos != std::string::npos) {
                line = line.substr(0, hash_pos);
            }
            // Trim whitespace
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            auto end = line.find_last_not_of(" \t\r\n");
            line = line.substr(start, end - start + 1);

            if (line.empty()) continue;

            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            auto eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1);

            auto trim_str = [](std::string& s) {
                auto s_start = s.find_first_not_of(" \t\r\n");
                if (s_start == std::string::npos) { s.clear(); return; }
                auto s_end = s.find_last_not_of(" \t\r\n");
                s = s.substr(s_start, s_end - s_start + 1);
            };
            trim_str(key);
            trim_str(val);

            auto parse_bool = [](const std::string& v) -> bool {
                return (v == "1" || v == "true" || v == "TRUE" || v == "yes");
            };

            if (current_section == "global") {
                if (key == "num_cores") cfg.num_cores = std::stoul(val);
                else if (key == "enable_mesi") cfg.enable_mesi_coherence = parse_bool(val);
                else if (key == "dram_latency") cfg.dram_latency_cycles = std::stoul(val);
            } else if (current_section == "core") {
                if (key == "enable_ooo") cfg.default_core.enable_ooo = parse_bool(val);
                else if (key == "fetch_width") cfg.default_core.fetch_width = std::stoul(val);
                else if (key == "decode_width") cfg.default_core.decode_width = std::stoul(val);
                else if (key == "issue_width") cfg.default_core.issue_width = std::stoul(val);
                else if (key == "commit_width") cfg.default_core.commit_width = std::stoul(val);
                else if (key == "rob_size") cfg.default_core.rob_size = std::stoul(val);
                else if (key == "rs_size") cfg.default_core.rs_size = std::stoul(val);
                else if (key == "num_phys_regs") cfg.default_core.num_phys_regs = std::stoul(val);
            } else if (current_section == "l1i") {
                if (key == "enabled") cfg.default_core.l1i.enabled = parse_bool(val);
                else if (key == "size_bytes") cfg.default_core.l1i.size_bytes = std::stoul(val);
                else if (key == "line_size") cfg.default_core.l1i.line_size = std::stoul(val);
                else if (key == "associativity") cfg.default_core.l1i.associativity = std::stoul(val);
                else if (key == "hit_latency") cfg.default_core.l1i.hit_latency_cycles = std::stoul(val);
            } else if (current_section == "l1d") {
                if (key == "enabled") cfg.default_core.l1d.enabled = parse_bool(val);
                else if (key == "size_bytes") cfg.default_core.l1d.size_bytes = std::stoul(val);
                else if (key == "line_size") cfg.default_core.l1d.line_size = std::stoul(val);
                else if (key == "associativity") cfg.default_core.l1d.associativity = std::stoul(val);
                else if (key == "hit_latency") cfg.default_core.l1d.hit_latency_cycles = std::stoul(val);
            } else if (current_section == "l2") {
                if (key == "enabled") cfg.l2_shared.enabled = parse_bool(val);
                else if (key == "size_bytes") cfg.l2_shared.size_bytes = std::stoul(val);
                else if (key == "line_size") cfg.l2_shared.line_size = std::stoul(val);
                else if (key == "associativity") cfg.l2_shared.associativity = std::stoul(val);
                else if (key == "hit_latency") cfg.l2_shared.hit_latency_cycles = std::stoul(val);
            } else if (current_section == "branch_predictor") {
                if (key == "enabled") cfg.default_core.branch_predictor.enabled = parse_bool(val);
                else if (key == "type") {
                    if (val == "IDEAL" || val == "ideal") cfg.default_core.branch_predictor.type = PredictorType::IDEAL;
                    else if (val == "BIMODAL" || val == "bimodal") cfg.default_core.branch_predictor.type = PredictorType::BIMODAL;
                    else if (val == "GSHARE" || val == "gshare") cfg.default_core.branch_predictor.type = PredictorType::GSHARE;
                    else if (val == "TAGE" || val == "tage") cfg.default_core.branch_predictor.type = PredictorType::TAGE;
                } else if (key == "table_size") cfg.default_core.branch_predictor.table_size = std::stoul(val);
            } else if (current_section == "lsu") {
                if (key == "enabled") cfg.default_core.lsu.enabled = parse_bool(val);
                else if (key == "lq_size") cfg.default_core.lsu.lq_size = std::stoul(val);
                else if (key == "sq_size") cfg.default_core.lsu.sq_size = std::stoul(val);
                else if (key == "enable_forwarding") cfg.default_core.lsu.enable_store_forwarding = parse_bool(val);
            }
        }

        cfg.validate();
        return cfg;
    }
};

} // namespace tinyarmsim::uarch
