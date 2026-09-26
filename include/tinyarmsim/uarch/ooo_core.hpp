#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <memory>
#include <iostream>
#include "tinyarmsim/common.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/interpreter.hpp"
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/stats.hpp"
#include "tinyarmsim/uarch/uop.hpp"
#include "tinyarmsim/uarch/fetch_unit.hpp"
#include "tinyarmsim/uarch/rat_prf.hpp"
#include "tinyarmsim/uarch/rob_issue_queue.hpp"
#include "tinyarmsim/uarch/lsu.hpp"
#include "tinyarmsim/uarch/cache.hpp"

namespace tinyarmsim::uarch {

class OoOCore {
public:
    OoOCore(size_t core_id,
            const CoreConfig& config,
            MemoryBus& bus,
            Cache* l1i,
            Cache* l1d,
            uint32_t entry_pc = 0)
        : core_id_(core_id),
          config_(config),
          bus_(bus),
          l1i_(l1i),
          l1d_(l1d),
          fetch_unit_(entry_pc, bus, l1i, config, config.branch_predictor),
          prf_(config.num_phys_regs > 0 ? config.num_phys_regs : 128),
          free_list_(config.num_phys_regs > 0 ? config.num_phys_regs : 128, 16),
          rob_(config.rob_size > 0 ? config.rob_size : 64),
          iq_(config.rs_size > 0 ? config.rs_size : 32),
          lsu_(config.lsu, l1d) {}

    // Advance core by 1 clock cycle using Reverse Stage Traversal
    void tick() {
        if (halted_) return;
        cycles_++;

        // -------------------------------------------------------------
        // Stage 8: Commit / Retire
        // -------------------------------------------------------------
        stage_commit();

        // -------------------------------------------------------------
        // Stage 7: Writeback & Wakeup
        // -------------------------------------------------------------
        stage_writeback();

        // -------------------------------------------------------------
        // Stage 6: Execute (Multi-port ALUs, AGUs, Branches)
        // -------------------------------------------------------------
        stage_execute();

        // -------------------------------------------------------------
        // Stage 5: Issue / Select from Issue Queue
        // -------------------------------------------------------------
        stage_issue();

        // -------------------------------------------------------------
        // Stage 4: Dispatch & ROB / LSU Allocation
        // -------------------------------------------------------------
        stage_dispatch();

        // -------------------------------------------------------------
        // Stage 3: Rename & RAT / Checkpointing
        // -------------------------------------------------------------
        stage_rename();

        // -------------------------------------------------------------
        // Stage 1 & 2: Fetch & Pre-decode
        // -------------------------------------------------------------
        fetch_unit_.tick();
        if (fetch_unit_.is_halted() && rob_.is_empty() && rename_queue_.empty()) {
            halted_ = true;
        }
    }

    [[nodiscard]] size_t get_core_id() const noexcept {
        return core_id_;
    }

    [[nodiscard]] MemoryBus& get_bus() noexcept {
        return bus_;
    }

    [[nodiscard]] bool is_halted() const noexcept {
        return halted_;
    }

    [[nodiscard]] uint64_t get_cycles() const noexcept {
        return cycles_;
    }

    [[nodiscard]] uint64_t get_committed_instructions() const noexcept {
        return committed_insts_;
    }

    [[nodiscard]] CoreStats get_stats() const noexcept {
        CoreStats s;
        s.cycles = cycles_;
        s.committed_instructions = committed_insts_;
        s.committed_uops = committed_uops_;
        s.rob_full_stalls = rob_full_stalls_;
        s.rs_full_stalls = rs_full_stalls_;
        s.rename_reg_exhaustion_stalls = rename_stalls_;
        s.branch_mispredict_flushes = branch_flushes_;
        if (l1i_) s.l1i = l1i_->get_stats();
        if (l1d_) s.l1d = l1d_->get_stats();
        s.lsu = lsu_.get_stats();
        return s;
    }

private:
    void stage_commit() {
        uint32_t commit_count = 0;
        uint32_t max_commit = config_.commit_width > 0 ? config_.commit_width : 4;

        while (commit_count < max_commit && rob_.can_commit_head()) {
            ROBEntry entry = rob_.commit_head();
            const auto& uop = entry.uop;

            if (uop.type == UOpType::SVC || uop.type == UOpType::HALT) {
                halted_ = true;
            }

            // Commit architectural destination register
            if (uop.arch_dest != UOp::INVALID_REG && uop.arch_dest < 16) {
                rat_.commit(uop.arch_dest, uop.phys_dest);
                // Free superseded old physical register
                if (uop.old_phys_dest >= 16) {
                    free_list_.free(uop.old_phys_dest);
                }
            }

            // Drain committed store to L1D
            if (uop.type == UOpType::STORE_DATA) {
                lsu_.commit_store(uop.lsu_queue_idx);
            }

            committed_uops_++;
            if (uop.type != UOpType::STORE_DATA) {
                committed_insts_++;
            }
            commit_count++;
        }
    }

    void stage_writeback() {
        for (const auto& uop : exec_to_wb_buffer_) {
            if (uop.phys_dest > 0 && uop.phys_dest < prf_.size()) {
                prf_.write(uop.phys_dest, uop.mem_data);
                iq_.wakeup(uop.phys_dest);
            }
            rob_.mark_completed(uop.rob_idx);
        }
        exec_to_wb_buffer_.clear();
    }

    void stage_execute() {
        for (auto& uop : issued_uops_) {
            if (uop.type == UOpType::BRANCH || uop.type == UOpType::CALL || uop.type == UOpType::RET) {
                // Branch resolution
                branch_pred_count_++;
                if (uop.branch_mispredicted) {
                    branch_flushes_++;
                    recover_from_mispredict(uop);
                    break;
                }
            } else if (uop.type == UOpType::STORE_ADDR) {
                size_t violating_rob = 0;
                bool violation = lsu_.execute_store_address(uop.lsu_queue_idx, uop.mem_addr, uop.mem_size_bytes, uop.seq_num, violating_rob);
                if (violation) {
                    recover_from_memory_violation(violating_rob);
                    break;
                }
            } else if (uop.type == UOpType::STORE_DATA) {
                lsu_.execute_store_data(uop.lsu_queue_idx, uop.mem_data);
            } else if (uop.type == UOpType::LOAD) {
                auto l_res = lsu_.execute_load(uop.lsu_queue_idx, uop.mem_addr, uop.mem_size_bytes, uop.seq_num);
                uop.mem_data = l_res.data;
            }

            uop.executed = true;
            exec_to_wb_buffer_.push_back(uop);
        }
        issued_uops_.clear();
    }

    void stage_issue() {
        uint32_t issue_width = config_.issue_width > 0 ? config_.issue_width : 4;
        issued_uops_ = iq_.select_and_issue(issue_width);
    }

    void stage_dispatch() {
        while (!rename_queue_.empty()) {
            const auto& uop = rename_queue_.front();
            if (rob_.is_full()) {
                rob_full_stalls_++;
                break;
            }
            if (iq_.is_full()) {
                rs_full_stalls_++;
                break;
            }
            if (uop.type == UOpType::LOAD && !lsu_.can_allocate_load()) break;
            if ((uop.type == UOpType::STORE_ADDR || uop.type == UOpType::STORE_DATA) && !lsu_.can_allocate_store()) break;

            UOp disp_uop = uop;
            size_t r_idx = rob_.allocate(disp_uop);
            disp_uop.rob_idx = r_idx;

            if (disp_uop.type == UOpType::LOAD) {
                disp_uop.lsu_queue_idx = lsu_.allocate_load(disp_uop);
            } else if (disp_uop.type == UOpType::STORE_ADDR || disp_uop.type == UOpType::STORE_DATA) {
                disp_uop.lsu_queue_idx = lsu_.allocate_store(disp_uop);
            }

            iq_.insert(disp_uop);
            rename_queue_.pop_front();
        }
    }

    void stage_rename() {
        uint32_t rename_count = 0;
        uint32_t rename_width = config_.rename_width > 0 ? config_.rename_width : 4;

        while (rename_count < rename_width && fetch_unit_.has_uops()) {
            if (rename_queue_.size() >= 8) break;

            UOp uop = fetch_unit_.pop_uop();

            // Source operand renaming & dependency check
            if (uop.arch_src1 != UOp::INVALID_REG && uop.arch_src1 < 16) {
                uop.phys_src1 = rat_.get(uop.arch_src1);
                uop.src1_ready = prf_.is_ready(uop.phys_src1);
            }
            if (uop.arch_src2 != UOp::INVALID_REG && uop.arch_src2 < 16) {
                uop.phys_src2 = rat_.get(uop.arch_src2);
                uop.src2_ready = prf_.is_ready(uop.phys_src2);
            }
            if (uop.arch_src3 != UOp::INVALID_REG && uop.arch_src3 < 16) {
                uop.phys_src3 = rat_.get(uop.arch_src3);
                uop.src3_ready = prf_.is_ready(uop.phys_src3);
            }

            // Destination operand renaming
            if (uop.arch_dest != UOp::INVALID_REG && uop.arch_dest < 16) {
                if (!free_list_.has_free()) {
                    rename_stalls_++;
                    break;
                }
                uop.old_phys_dest = rat_.get(uop.arch_dest);
                uop.phys_dest = free_list_.allocate();
                rat_.set(uop.arch_dest, uop.phys_dest);
                prf_.set_ready(uop.phys_dest, false); // Pending execution
            }

            rename_queue_.push_back(uop);
            rename_count++;
        }
    }

    void recover_from_mispredict(const UOp& branch_uop) {
        rob_.flush_younger_than(branch_uop.rob_idx);
        iq_.reset();
        rename_queue_.clear();
        exec_to_wb_buffer_.clear();
        rat_.rollback_to_commit(free_list_);
        fetch_unit_.flush(branch_uop.actual_target);
    }

    void recover_from_memory_violation(size_t violating_rob_idx) {
        rob_.flush_younger_than(violating_rob_idx);
        iq_.reset();
        rename_queue_.clear();
        exec_to_wb_buffer_.clear();
        rat_.rollback_to_commit(free_list_);
    }

    size_t core_id_{0};
    CoreConfig config_;
    MemoryBus& bus_;
    Cache* l1i_{nullptr};
    Cache* l1d_{nullptr};

    FetchUnit fetch_unit_;
    PhysicalRegisterFile prf_;
    FreeList free_list_;
    RegisterAliasTable rat_;
    ReorderBuffer rob_;
    IssueQueue iq_;
    LoadStoreUnit lsu_;

    std::deque<UOp> rename_queue_;
    std::vector<UOp> issued_uops_;
    std::vector<UOp> exec_to_wb_buffer_;

    uint64_t cycles_{0};
    uint64_t committed_insts_{0};
    uint64_t committed_uops_{0};
    uint64_t rob_full_stalls_{0};
    uint64_t rs_full_stalls_{0};
    uint64_t rename_stalls_{0};
    uint64_t branch_flushes_{0};
    uint64_t branch_pred_count_{0};
    bool halted_{false};
};

} // namespace tinyarmsim::uarch
