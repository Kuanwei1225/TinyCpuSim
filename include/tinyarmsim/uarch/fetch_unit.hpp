#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <memory>
#include "tinyarmsim/common.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/decoder.hpp"
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/stats.hpp"
#include "tinyarmsim/uarch/uop.hpp"
#include "tinyarmsim/uarch/branch_predictor.hpp"
#include "tinyarmsim/uarch/uop_decoder.hpp"
#include "tinyarmsim/uarch/cache.hpp"

namespace tinyarmsim::uarch {

class FetchUnit {
public:
    FetchUnit(uint32_t start_pc,
              MemoryBus& bus,
              Cache* l1i_cache,
              const CoreConfig& core_cfg,
              const BranchPredictorConfig& bp_cfg)
        : pc_(start_pc & ~1u),
          bus_(bus),
          l1i_(l1i_cache),
          core_cfg_(core_cfg),
          branch_pred_(bp_cfg),
          fetch_width_(core_cfg.fetch_width > 0 ? core_cfg.fetch_width : 4),
          max_queue_size_(16),
          seq_counter_(0) {}

    void tick() {
        if (stalled_ || is_halted_) return;

        // Fetch up to fetch_width instructions per cycle
        for (uint32_t i = 0; i < fetch_width_; ++i) {
            if (uop_queue_.size() >= max_queue_size_) {
                // Fetch queue is full, stall front-end
                break;
            }

            // Fetch halfword from memory / L1I
            if (pc_ + 2 > bus_.size()) {
                is_halted_ = true;
                break;
            }

            // Access L1I cache if active
            if (l1i_ && l1i_->get_config().is_active()) {
                uint32_t lat = 0;
                l1i_->access(pc_, false, lat);
            }

            uint16_t w1 = bus_.read16(pc_);
            uint32_t current_inst_pc = pc_;
            DecodedInstruction dec_inst;
            uint32_t inst_size = 2;

            if (Decoder::is_32bit_thumb(w1)) {
                if (pc_ + 4 > bus_.size()) {
                    is_halted_ = true;
                    break;
                }
                uint16_t w2 = bus_.read16(pc_ + 2);
                dec_inst = Decoder::decode32(w1, w2, current_inst_pc);
                inst_size = 4;
            } else {
                dec_inst = Decoder::decode16(w1, current_inst_pc);
                inst_size = 2;
            }

            // Advance sequential PC
            pc_ += inst_size;

            // Expand to micro-ops
            seq_counter_++;
            std::vector<UOp> uops = UOpDecoder::decode(dec_inst, current_inst_pc, seq_counter_);

            // Branch prediction evaluation
            bool redirect = false;
            uint32_t redirect_target = 0;
            bool is_svc = false;

            for (auto& uop : uops) {
                if (uop.type == UOpType::SVC || uop.type == UOpType::HALT) {
                    is_svc = true;
                }
                if (uop.is_branch) {
                    BranchPrediction pred = branch_pred_.predict(current_inst_pc);
                    uop.pred_taken = pred.taken;
                    uop.pred_target = pred.target_pc & ~1u;
                    uop.branch_pred = pred;

                    // If branch is unconditional (B / BL), default taken if not in BTB
                    if (uop.opcode == Opcode::B && uop.cond == ConditionCode::AL && !pred.is_branch) {
                        uop.pred_taken = true;
                        uop.pred_target = uop.actual_target & ~1u;
                    } else if (uop.opcode == Opcode::BL && !pred.is_branch) {
                        uop.pred_taken = true;
                        uop.pred_target = uop.actual_target & ~1u;
                    }

                    if (uop.pred_taken) {
                        redirect = true;
                        redirect_target = uop.pred_target & ~1u;
                    }
                }
                uop_queue_.push_back(uop);
            }

            // If SVC/HALT, stall front-end from fetching beyond it
            if (is_svc) {
                stalled_ = true;
                break;
            }

            // If a branch is predicted taken, redirect Fetch PC and stop fetching for this cycle
            if (redirect) {
                pc_ = redirect_target & ~1u;
                break;
            }
        }
    }

    // Flush front-end on branch misprediction or exception recovery
    void flush(uint32_t target_pc) noexcept {
        uop_queue_.clear();
        pc_ = target_pc & ~1u;
        stalled_ = false;
        is_halted_ = false;
    }

    [[nodiscard]] bool has_uops() const noexcept {
        return !uop_queue_.empty();
    }

    [[nodiscard]] size_t queue_size() const noexcept {
        return uop_queue_.size();
    }

    UOp pop_uop() {
        if (uop_queue_.empty()) {
            throw std::runtime_error("Attempted to pop from empty fetch UOp queue");
        }
        UOp uop = uop_queue_.front();
        uop_queue_.pop_front();
        return uop;
    }

    [[nodiscard]] const UOp& peek_uop() const {
        if (uop_queue_.empty()) {
            throw std::runtime_error("Attempted to peek empty fetch UOp queue");
        }
        return uop_queue_.front();
    }

    [[nodiscard]] uint32_t get_pc() const noexcept {
        return pc_;
    }

    void set_pc(uint32_t pc) noexcept {
        pc_ = pc;
    }

    void set_stalled(bool stalled) noexcept {
        stalled_ = stalled;
    }

    [[nodiscard]] bool is_halted() const noexcept {
        return is_halted_;
    }

    [[nodiscard]] CompositeBranchPredictor& get_branch_predictor() noexcept {
        return branch_pred_;
    }

    [[nodiscard]] const CompositeBranchPredictor& get_branch_predictor() const noexcept {
        return branch_pred_;
    }

private:
    uint32_t pc_{0};
    MemoryBus& bus_;
    Cache* l1i_{nullptr};
    CoreConfig core_cfg_;
    CompositeBranchPredictor branch_pred_;
    uint32_t fetch_width_{4};
    size_t max_queue_size_{16};
    uint64_t seq_counter_{0};

    std::deque<UOp> uop_queue_;
    bool stalled_{false};
    bool is_halted_{false};
};

} // namespace tinyarmsim::uarch
