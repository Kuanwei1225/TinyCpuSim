#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/stats.hpp"

namespace tinyarmsim::uarch {

enum class BranchType : uint8_t {
    DIRECT_COND,
    DIRECT_UNCOND,
    INDIRECT_BRANCH,
    DIRECT_CALL,
    INDIRECT_CALL,
    RETURN
};

struct BranchPrediction {
    bool is_branch{false};
    bool taken{false};
    uint32_t target_pc{0};
    BranchType type{BranchType::DIRECT_COND};
    
    // Metadata for predictor update/training at execution stage
    uint32_t predictor_meta{0};
    int8_t provider_table{-1};
    bool alt_used{false};
};

// Two-bit saturating counter helper (0=Strongly Not Taken, 1=Weakly Not Taken, 2=Weakly Taken, 3=Strongly Taken)
class SaturatingCounter2Bit {
public:
    constexpr SaturatingCounter2Bit() : state_(2) {} // Default: Weakly Taken
    explicit constexpr SaturatingCounter2Bit(uint8_t initial_state) : state_(initial_state & 0x3) {}

    [[nodiscard]] bool is_taken() const noexcept {
        return state_ >= 2;
    }

    [[nodiscard]] uint8_t get() const noexcept {
        return state_;
    }

    void update(bool taken) noexcept {
        if (taken) {
            if (state_ < 3) state_++;
        } else {
            if (state_ > 0) state_--;
        }
    }

private:
    uint8_t state_{2};
};

// -----------------------------------------------------------------------------
// Bimodal Predictor
// -----------------------------------------------------------------------------
class BimodalPredictor {
public:
    explicit BimodalPredictor(size_t table_size = 2048)
        : table_(table_size, SaturatingCounter2Bit(2)), mask_(table_size - 1) {
        if (table_size == 0 || (table_size & (table_size - 1)) != 0) {
            throw std::invalid_argument("Bimodal table_size must be a power of 2");
        }
    }

    [[nodiscard]] bool predict(uint32_t pc) const noexcept {
        size_t idx = (pc >> 2) & mask_;
        return table_[idx].is_taken();
    }

    void update(uint32_t pc, bool taken) noexcept {
        size_t idx = (pc >> 2) & mask_;
        table_[idx].update(taken);
    }

    void reset() {
        std::fill(table_.begin(), table_.end(), SaturatingCounter2Bit(2));
    }

private:
    std::vector<SaturatingCounter2Bit> table_;
    size_t mask_;
};

// -----------------------------------------------------------------------------
// GShare Predictor
// -----------------------------------------------------------------------------
class GSharePredictor {
public:
    explicit GSharePredictor(size_t table_size = 4096, size_t history_bits = 12)
        : table_(table_size, SaturatingCounter2Bit(2)),
          mask_(table_size - 1),
          history_mask_((1ULL << history_bits) - 1),
          global_history_(0) {
        if (table_size == 0 || (table_size & (table_size - 1)) != 0) {
            throw std::invalid_argument("GShare table_size must be a power of 2");
        }
    }

    [[nodiscard]] bool predict(uint32_t pc) const noexcept {
        size_t idx = compute_index(pc, global_history_);
        return table_[idx].is_taken();
    }

    void update(uint32_t pc, bool taken) noexcept {
        size_t idx = compute_index(pc, global_history_);
        table_[idx].update(taken);
        // Shift global history
        global_history_ = ((global_history_ << 1) | (taken ? 1 : 0)) & history_mask_;
    }

    void restore_history(uint64_t history) noexcept {
        global_history_ = history & history_mask_;
    }

    [[nodiscard]] uint64_t get_history() const noexcept {
        return global_history_;
    }

    void reset() {
        std::fill(table_.begin(), table_.end(), SaturatingCounter2Bit(2));
        global_history_ = 0;
    }

private:
    [[nodiscard]] size_t compute_index(uint32_t pc, uint64_t history) const noexcept {
        size_t pc_hash = (pc >> 2);
        return (pc_hash ^ (history & history_mask_)) & mask_;
    }

    std::vector<SaturatingCounter2Bit> table_;
    size_t mask_;
    uint64_t history_mask_;
    uint64_t global_history_;
};

// -----------------------------------------------------------------------------
// TAGE (TAgged GEometric) Branch Predictor
// -----------------------------------------------------------------------------
class TagePredictor {
public:
    struct TaggedEntry {
        SaturatingCounter2Bit ctr{2};
        uint16_t tag{0};
        uint8_t u{0}; // Useful bit (0 or 1)
    };

    struct TaggedTable {
        std::vector<TaggedEntry> entries;
        size_t history_length{0};
        size_t mask{0};

        TaggedTable(size_t num_entries, size_t hist_len)
            : entries(num_entries), history_length(hist_len), mask(num_entries - 1) {}
    };

    explicit TagePredictor(size_t bimodal_size = 2048, size_t num_tables = 4, size_t table_entries = 1024)
        : bimodal_(bimodal_size), global_history_(0) {
        
        // Geometric progression for history lengths: e.g. 4, 10, 24, 60
        std::vector<size_t> hist_lens = {4, 10, 24, 60};
        for (size_t i = 0; i < num_tables; ++i) {
            size_t h_len = (i < hist_lens.size()) ? hist_lens[i] : (4 * (1ULL << i));
            tables_.emplace_back(table_entries, h_len);
        }
    }

    struct PredictionResult {
        bool taken{false};
        int provider_table{-1}; // -1 = Bimodal (Base)
        int alt_table{-1};
        bool alt_taken{false};
        uint16_t provider_tag{0};
    };

    [[nodiscard]] PredictionResult predict(uint32_t pc) const noexcept {
        PredictionResult res;
        bool base_pred = bimodal_.predict(pc);
        res.taken = base_pred;
        res.alt_taken = base_pred;

        // Search tagged tables from longest history to shortest
        for (int i = static_cast<int>(tables_.size()) - 1; i >= 0; --i) {
            size_t idx = get_table_index(i, pc);
            uint16_t tag = get_table_tag(i, pc);
            const auto& entry = tables_[static_cast<size_t>(i)].entries[idx];

            if (entry.tag == tag) {
                if (res.provider_table == -1) {
                    res.provider_table = i;
                    res.provider_tag = tag;
                    res.taken = entry.ctr.is_taken();
                } else if (res.alt_table == -1) {
                    res.alt_table = i;
                    res.alt_taken = entry.ctr.is_taken();
                    break;
                }
            }
        }

        return res;
    }

    void update(uint32_t pc, bool taken, const PredictionResult& pred) noexcept {
        // Update provider or bimodal
        if (pred.provider_table == -1) {
            bimodal_.update(pc, taken);
        } else {
            size_t p_idx = get_table_index(pred.provider_table, pc);
            auto& entry = tables_[static_cast<size_t>(pred.provider_table)].entries[p_idx];
            entry.ctr.update(taken);

            // Update useful bit
            if (pred.taken != pred.alt_taken) {
                if (pred.taken == taken) {
                    entry.u = 1;
                } else {
                    entry.u = 0;
                }
            }
        }

        // On misprediction, allocate entry in a table with longer history than provider
        if (pred.taken != taken) {
            int start_alloc = pred.provider_table + 1;
            bool allocated = false;
            for (size_t i = static_cast<size_t>(std::max(0, start_alloc)); i < tables_.size(); ++i) {
                size_t a_idx = get_table_index(static_cast<int>(i), pc);
                auto& a_entry = tables_[i].entries[a_idx];
                if (a_entry.u == 0) {
                    a_entry.tag = get_table_tag(static_cast<int>(i), pc);
                    a_entry.ctr = SaturatingCounter2Bit(taken ? 2 : 1);
                    a_entry.u = 0;
                    allocated = true;
                    break;
                }
            }
            // If all entries useful, decay useful bits
            if (!allocated && start_alloc < static_cast<int>(tables_.size())) {
                for (auto& tbl : tables_) {
                    for (auto& ent : tbl.entries) {
                        ent.u = 0;
                    }
                }
            }
        }

        // Shift global history
        global_history_ = ((global_history_ << 1) | (taken ? 1 : 0));
    }

    void reset() {
        bimodal_.reset();
        for (auto& tbl : tables_) {
            for (auto& ent : tbl.entries) {
                ent.tag = 0;
                ent.u = 0;
                ent.ctr = SaturatingCounter2Bit(2);
            }
        }
        global_history_ = 0;
    }

private:
    [[nodiscard]] size_t get_table_index(int table_idx, uint32_t pc) const noexcept {
        const auto& tbl = tables_[static_cast<size_t>(table_idx)];
        uint64_t hist = global_history_ & ((1ULL << tbl.history_length) - 1);
        return ((pc >> 2) ^ hist ^ (hist >> 5)) & tbl.mask;
    }

    [[nodiscard]] uint16_t get_table_tag(int table_idx, uint32_t pc) const noexcept {
        const auto& tbl = tables_[static_cast<size_t>(table_idx)];
        uint64_t hist = global_history_ & ((1ULL << tbl.history_length) - 1);
        return static_cast<uint16_t>(((pc >> 4) ^ (hist << 1) ^ (hist >> 7)) & 0xFFFF);
    }

    BimodalPredictor bimodal_;
    std::vector<TaggedTable> tables_;
    uint64_t global_history_;
};

// -----------------------------------------------------------------------------
// Branch Target Buffer (BTB)
// -----------------------------------------------------------------------------
class BranchTargetBuffer {
public:
    struct BTBEntry {
        uint32_t tag{0};
        uint32_t target{0};
        BranchType type{BranchType::DIRECT_COND};
        bool valid{false};
    };

    explicit BranchTargetBuffer(size_t num_entries = 1024)
        : entries_(num_entries), mask_(num_entries - 1) {
        if (num_entries == 0 || (num_entries & (num_entries - 1)) != 0) {
            throw std::invalid_argument("BTB entries must be a power of 2");
        }
    }

    [[nodiscard]] bool lookup(uint32_t pc, uint32_t& out_target, BranchType& out_type) const noexcept {
        size_t idx = (pc >> 2) & mask_;
        const auto& entry = entries_[idx];
        if (entry.valid && entry.tag == (pc >> 2)) {
            out_target = entry.target;
            out_type = entry.type;
            return true;
        }
        return false;
    }

    void update(uint32_t pc, uint32_t target, BranchType type) noexcept {
        size_t idx = (pc >> 2) & mask_;
        entries_[idx].tag = (pc >> 2);
        entries_[idx].target = target;
        entries_[idx].type = type;
        entries_[idx].valid = true;
    }

    void reset() {
        for (auto& entry : entries_) {
            entry.valid = false;
        }
    }

private:
    std::vector<BTBEntry> entries_;
    size_t mask_;
};

// -----------------------------------------------------------------------------
// Return Address Stack (RAS)
// -----------------------------------------------------------------------------
class ReturnAddressStack {
public:
    explicit ReturnAddressStack(size_t capacity = 16)
        : stack_(capacity, 0), capacity_(capacity), top_(0), size_(0) {
        if (capacity == 0) {
            throw std::invalid_argument("RAS capacity must be greater than 0");
        }
    }

    void push(uint32_t ret_pc) noexcept {
        stack_[top_] = ret_pc;
        top_ = (top_ + 1) % capacity_;
        if (size_ < capacity_) size_++;
    }

    [[nodiscard]] bool pop(uint32_t& out_pc) noexcept {
        if (size_ == 0) return false;
        top_ = (top_ + capacity_ - 1) % capacity_;
        out_pc = stack_[top_];
        size_--;
        return true;
    }

    [[nodiscard]] bool peek(uint32_t& out_pc) const noexcept {
        if (size_ == 0) return false;
        size_t peek_idx = (top_ + capacity_ - 1) % capacity_;
        out_pc = stack_[peek_idx];
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    void reset() noexcept {
        top_ = 0;
        size_ = 0;
        std::fill(stack_.begin(), stack_.end(), 0);
    }

private:
    std::vector<uint32_t> stack_;
    size_t capacity_;
    size_t top_;
    size_t size_;
};

// -----------------------------------------------------------------------------
// Unified Composite Branch Predictor (Frontend Integration)
// -----------------------------------------------------------------------------
class CompositeBranchPredictor {
public:
    explicit CompositeBranchPredictor(const BranchPredictorConfig& cfg)
        : config_(cfg),
          bimodal_(cfg.table_size > 0 ? cfg.table_size : 2048),
          gshare_(cfg.table_size > 0 ? cfg.table_size : 4096, 12),
          tage_(2048, cfg.tage_tables > 0 ? cfg.tage_tables : 4, 1024),
          btb_(cfg.btb_size > 0 ? cfg.btb_size : 1024),
          ras_(cfg.ras_size > 0 ? cfg.ras_size : 16) {}

    [[nodiscard]] BranchPrediction predict(uint32_t pc) noexcept {
        BranchPrediction pred;
        if (!config_.enabled) {
            return pred;
        }

        uint32_t btb_target = 0;
        BranchType btb_type = BranchType::DIRECT_COND;
        bool btb_hit = btb_.lookup(pc, btb_target, btb_type);

        if (!btb_hit) {
            // Not a known branch in BTB
            pred.is_branch = false;
            pred.taken = false;
            pred.target_pc = pc + 4;
            return pred;
        }

        pred.is_branch = true;
        pred.type = btb_type;

        // Function Return handling via RAS
        if (btb_type == BranchType::RETURN) {
            uint32_t ras_target = 0;
            if (ras_.peek(ras_target)) {
                pred.taken = true;
                pred.target_pc = ras_target;
                return pred;
            }
        }

        // Unconditional Direct / Call branches are always taken
        if (btb_type == BranchType::DIRECT_UNCOND || btb_type == BranchType::DIRECT_CALL) {
            pred.taken = true;
            pred.target_pc = btb_target;
            return pred;
        }

        // Conditional Branch direction prediction
        bool dir_taken = false;
        switch (config_.type) {
            case PredictorType::BIMODAL:
                dir_taken = bimodal_.predict(pc);
                break;
            case PredictorType::GSHARE:
                dir_taken = gshare_.predict(pc);
                break;
            case PredictorType::TAGE: {
                auto tage_res = tage_.predict(pc);
                dir_taken = tage_res.taken;
                pred.provider_table = static_cast<int8_t>(tage_res.provider_table);
                pred.alt_used = (tage_res.provider_table != -1 && tage_res.taken != tage_res.alt_taken);
                break;
            }
            case PredictorType::IDEAL:
            default:
                dir_taken = true;
                break;
        }

        pred.taken = dir_taken;
        pred.target_pc = dir_taken ? btb_target : (pc + 4);
        return pred;
    }

    void update(uint32_t pc, bool taken, uint32_t actual_target, BranchType type, const BranchPrediction& pred) noexcept {
        if (!config_.enabled) return;

        // Update BTB
        btb_.update(pc, actual_target, type);

        // Update Direction Predictor
        switch (config_.type) {
            case PredictorType::BIMODAL:
                bimodal_.update(pc, taken);
                break;
            case PredictorType::GSHARE:
                gshare_.update(pc, taken);
                break;
            case PredictorType::TAGE: {
                TagePredictor::PredictionResult tage_res;
                tage_res.taken = pred.taken;
                tage_res.provider_table = pred.provider_table;
                tage_.update(pc, taken, tage_res);
                break;
            }
            case PredictorType::IDEAL:
            default:
                break;
        }

        // RAS updates on call/return
        if (type == BranchType::DIRECT_CALL || type == BranchType::INDIRECT_CALL) {
            ras_.push(pc + 4);
        } else if (type == BranchType::RETURN) {
            uint32_t popped_pc = 0;
            static_cast<void>(ras_.pop(popped_pc));
        }
    }

    void reset() {
        bimodal_.reset();
        gshare_.reset();
        tage_.reset();
        btb_.reset();
        ras_.reset();
    }

    [[nodiscard]] ReturnAddressStack& get_ras() noexcept { return ras_; }
    [[nodiscard]] BranchTargetBuffer& get_btb() noexcept { return btb_; }

private:
    BranchPredictorConfig config_;
    BimodalPredictor bimodal_;
    GSharePredictor gshare_;
    TagePredictor tage_;
    BranchTargetBuffer btb_;
    ReturnAddressStack ras_;
};

} // namespace tinyarmsim::uarch
