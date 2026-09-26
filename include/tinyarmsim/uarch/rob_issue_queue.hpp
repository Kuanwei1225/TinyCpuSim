#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include "tinyarmsim/uarch/uop.hpp"
#include "tinyarmsim/uarch/rat_prf.hpp"

namespace tinyarmsim::uarch {

// -----------------------------------------------------------------------------
// ROB Entry
// -----------------------------------------------------------------------------
struct ROBEntry {
    size_t rob_idx{0};
    UOp uop{};
    bool ready{false};
    bool valid{false};
    bool has_exception{false};
    uint32_t exception_code{0};
};

// -----------------------------------------------------------------------------
// Reorder Buffer (ROB)
// -----------------------------------------------------------------------------
class ReorderBuffer {
public:
    explicit ReorderBuffer(size_t capacity = 64)
        : entries_(capacity), capacity_(capacity), head_(0), tail_(0), count_(0) {
        if (capacity == 0) {
            throw std::invalid_argument("ROB capacity must be > 0");
        }
    }

    [[nodiscard]] bool is_full() const noexcept {
        return count_ == capacity_;
    }

    [[nodiscard]] bool is_empty() const noexcept {
        return count_ == 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return count_;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

    size_t allocate(const UOp& uop) {
        if (is_full()) {
            throw std::runtime_error("ROB is full cannot allocate");
        }
        size_t idx = tail_;
        entries_[idx].rob_idx = idx;
        entries_[idx].uop = uop;
        entries_[idx].uop.rob_idx = idx;
        entries_[idx].ready = false;
        entries_[idx].valid = true;
        entries_[idx].has_exception = false;

        tail_ = (tail_ + 1) % capacity_;
        count_++;
        return idx;
    }

    void mark_completed(size_t rob_idx, bool has_exception = false, uint32_t exception_code = 0) {
        if (rob_idx >= capacity_ || !entries_[rob_idx].valid) {
            throw std::out_of_range("Invalid ROB entry completion index");
        }
        entries_[rob_idx].ready = true;
        entries_[rob_idx].has_exception = has_exception;
        entries_[rob_idx].exception_code = exception_code;
        entries_[rob_idx].uop.executed = true;
    }

    [[nodiscard]] bool can_commit_head() const noexcept {
        if (is_empty()) return false;
        return entries_[head_].valid && entries_[head_].ready;
    }

    [[nodiscard]] const ROBEntry& peek_head() const {
        if (is_empty()) {
            throw std::runtime_error("Attempted to peek empty ROB head");
        }
        return entries_[head_];
    }

    ROBEntry commit_head() {
        if (!can_commit_head()) {
            throw std::runtime_error("ROB head cannot commit");
        }
        ROBEntry committed = entries_[head_];
        entries_[head_].valid = false;
        entries_[head_].ready = false;

        head_ = (head_ + 1) % capacity_;
        count_--;
        return committed;
    }

    // Flush all entries younger than the given rob_idx (exclusive)
    void flush_younger_than(size_t rob_idx) noexcept {
        if (is_empty()) return;
        size_t curr = (rob_idx + 1) % capacity_;
        while (curr != tail_) {
            entries_[curr].valid = false;
            entries_[curr].ready = false;
            curr = (curr + 1) % capacity_;
            if (count_ > 0) count_--;
        }
        tail_ = (rob_idx + 1) % capacity_;
    }

    void reset() noexcept {
        head_ = 0;
        tail_ = 0;
        count_ = 0;
        for (auto& entry : entries_) {
            entry.valid = false;
            entry.ready = false;
        }
    }

    [[nodiscard]] ROBEntry& get_entry(size_t rob_idx) {
        if (rob_idx >= capacity_) throw std::out_of_range("ROB index out of range");
        return entries_[rob_idx];
    }

private:
    std::vector<ROBEntry> entries_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    size_t count_;
};

// -----------------------------------------------------------------------------
// Issue Queue / Reservation Station (RS)
// -----------------------------------------------------------------------------
struct IssueEntry {
    UOp uop{};
    bool valid{false};
};

class IssueQueue {
public:
    explicit IssueQueue(size_t capacity = 32)
        : entries_(capacity), capacity_(capacity), count_(0) {
        if (capacity == 0) {
            throw std::invalid_argument("IssueQueue capacity must be > 0");
        }
    }

    [[nodiscard]] bool is_full() const noexcept {
        return count_ == capacity_;
    }

    [[nodiscard]] bool is_empty() const noexcept {
        return count_ == 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return count_;
    }

    void insert(const UOp& uop) {
        if (is_full()) {
            throw std::runtime_error("IssueQueue is full");
        }
        for (auto& entry : entries_) {
            if (!entry.valid) {
                entry.uop = uop;
                entry.valid = true;
                count_++;
                return;
            }
        }
        throw std::runtime_error("IssueQueue inconsistent state");
    }

    // Broadcast tag writeback from execution units
    void wakeup(uint16_t completed_phys_reg) noexcept {
        for (auto& entry : entries_) {
            if (entry.valid) {
                auto& uop = entry.uop;
                if (uop.phys_src1 == completed_phys_reg) uop.src1_ready = true;
                if (uop.phys_src2 == completed_phys_reg) uop.src2_ready = true;
                if (uop.phys_src3 == completed_phys_reg) uop.src3_ready = true;
            }
        }
    }

    // Select up to max_issue ready uOps
    [[nodiscard]] std::vector<UOp> select_and_issue(uint32_t max_issue) {
        std::vector<UOp> issued;
        for (auto& entry : entries_) {
            if (entry.valid) {
                const auto& uop = entry.uop;
                if (uop.src1_ready && uop.src2_ready && uop.src3_ready) {
                    issued.push_back(uop);
                    entry.valid = false;
                    count_--;
                    if (issued.size() >= max_issue) break;
                }
            }
        }
        return issued;
    }

    // Flush all squashed uOps matching sequence condition
    void flush_squashed() noexcept {
        for (auto& entry : entries_) {
            if (entry.valid && entry.uop.is_squashed) {
                entry.valid = false;
                if (count_ > 0) count_--;
            }
        }
    }

    void reset() noexcept {
        for (auto& entry : entries_) {
            entry.valid = false;
        }
        count_ = 0;
    }

private:
    std::vector<IssueEntry> entries_;
    size_t capacity_;
    size_t count_;
};

} // namespace tinyarmsim::uarch
