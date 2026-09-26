#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <array>
#include <stdexcept>
#include <algorithm>

namespace tinyarmsim::uarch {

// -----------------------------------------------------------------------------
// Physical Register File (PRF)
// -----------------------------------------------------------------------------
class PhysicalRegisterFile {
public:
    explicit PhysicalRegisterFile(size_t num_phys_regs = 128)
        : num_regs_(num_phys_regs),
          data_(num_phys_regs, 0),
          ready_(num_phys_regs, true) {
        if (num_phys_regs < 32) {
            throw std::invalid_argument("PRF must have at least 32 registers");
        }
    }

    [[nodiscard]] uint32_t read(uint16_t phys_reg) const {
        if (phys_reg >= num_regs_) {
            throw std::out_of_range("Physical register read out of range: " + std::to_string(phys_reg));
        }
        return data_[phys_reg];
    }

    void write(uint16_t phys_reg, uint32_t value) {
        if (phys_reg >= num_regs_) {
            throw std::out_of_range("Physical register write out of range: " + std::to_string(phys_reg));
        }
        data_[phys_reg] = value;
        ready_[phys_reg] = true;
    }

    [[nodiscard]] bool is_ready(uint16_t phys_reg) const noexcept {
        if (phys_reg >= num_regs_) return false;
        return ready_[phys_reg];
    }

    void set_ready(uint16_t phys_reg, bool ready) {
        if (phys_reg >= num_regs_) {
            throw std::out_of_range("Physical register set_ready out of range");
        }
        ready_[phys_reg] = ready;
    }

    [[nodiscard]] size_t size() const noexcept {
        return num_regs_;
    }

    void reset() {
        std::fill(data_.begin(), data_.end(), 0);
        std::fill(ready_.begin(), ready_.end(), true);
    }

private:
    size_t num_regs_;
    std::vector<uint32_t> data_;
    std::vector<bool> ready_;
};

// -----------------------------------------------------------------------------
// Free List
// -----------------------------------------------------------------------------
class FreeList {
public:
    explicit FreeList(size_t num_phys_regs = 128, size_t num_arch_regs = 17)
        : total_regs_(num_phys_regs), num_arch_regs_(num_arch_regs), is_free_(num_phys_regs, false) {
        if (num_phys_regs <= num_arch_regs) {
            throw std::invalid_argument("FreeList: num_phys_regs must exceed num_arch_regs");
        }
        reset(num_arch_regs);
    }

    [[nodiscard]] bool has_free() const noexcept {
        return !free_regs_.empty();
    }

    [[nodiscard]] size_t free_count() const noexcept {
        return free_regs_.size();
    }

    [[nodiscard]] uint16_t allocate() {
        if (free_regs_.empty()) {
            throw std::runtime_error("FreeList exhaustion: no available physical registers");
        }
        uint16_t reg = free_regs_.front();
        free_regs_.pop_front();
        is_free_[reg] = false;
        return reg;
    }

    void free(uint16_t phys_reg) noexcept {
        if (phys_reg >= total_regs_ || phys_reg < num_arch_regs_) return;
        if (!is_free_[phys_reg]) {
            is_free_[phys_reg] = true;
            free_regs_.push_back(phys_reg);
        }
    }

    void reset(size_t num_arch_regs = 17) {
        num_arch_regs_ = num_arch_regs;
        free_regs_.clear();
        std::fill(is_free_.begin(), is_free_.end(), false);
        for (size_t i = num_arch_regs_; i < total_regs_; ++i) {
            is_free_[i] = true;
            free_regs_.push_back(static_cast<uint16_t>(i));
        }
    }

    template <size_t N>
    void rebuild_from_committed(const std::array<uint16_t, N>& commit_map) {
        free_regs_.clear();
        std::fill(is_free_.begin(), is_free_.end(), false);
        std::vector<bool> in_use(total_regs_, false);
        for (size_t i = 0; i < N; ++i) {
            if (commit_map[i] < total_regs_) {
                in_use[commit_map[i]] = true;
            }
        }
        for (size_t p = num_arch_regs_; p < total_regs_; ++p) {
            if (!in_use[p]) {
                is_free_[p] = true;
                free_regs_.push_back(static_cast<uint16_t>(p));
            }
        }
    }

    [[nodiscard]] const std::deque<uint16_t>& get_state() const noexcept {
        return free_regs_;
    }

    void restore_state(const std::deque<uint16_t>& saved) noexcept {
        free_regs_ = saved;
    }

private:
    size_t total_regs_;
    size_t num_arch_regs_{17};
    std::deque<uint16_t> free_regs_;
    std::vector<bool> is_free_;
};

// -----------------------------------------------------------------------------
// Register Alias Table (RAT) & Branch Checkpointing
// -----------------------------------------------------------------------------
class RegisterAliasTable {
public:
    static constexpr size_t NUM_ARCH_REGS = 17;

    struct Checkpoint {
        std::array<uint16_t, NUM_ARCH_REGS> rat_map;
    };

    RegisterAliasTable() {
        reset();
    }

    [[nodiscard]] uint16_t get(uint8_t arch_reg) const {
        if (arch_reg >= NUM_ARCH_REGS) {
            throw std::out_of_range("Architectural register index >= " + std::to_string(NUM_ARCH_REGS) + ": " + std::to_string(arch_reg));
        }
        return spec_map_[arch_reg];
    }

    void set(uint8_t arch_reg, uint16_t phys_reg) {
        if (arch_reg >= NUM_ARCH_REGS) {
            throw std::out_of_range("Architectural register index >= " + std::to_string(NUM_ARCH_REGS) + ": " + std::to_string(arch_reg));
        }
        spec_map_[arch_reg] = phys_reg;
    }

    [[nodiscard]] uint16_t get_commit(uint8_t arch_reg) const {
        if (arch_reg >= NUM_ARCH_REGS) {
            throw std::out_of_range("Architectural register index >= " + std::to_string(NUM_ARCH_REGS));
        }
        return commit_map_[arch_reg];
    }

    void commit(uint8_t arch_reg, uint16_t phys_reg) {
        if (arch_reg >= NUM_ARCH_REGS) return;
        commit_map_[arch_reg] = phys_reg;
    }

    [[nodiscard]] Checkpoint create_checkpoint() const {
        Checkpoint cp;
        cp.rat_map = spec_map_;
        return cp;
    }

    void restore_checkpoint(const Checkpoint& cp) noexcept {
        spec_map_ = cp.rat_map;
    }

    void restore_from_commit() noexcept {
        spec_map_ = commit_map_;
    }

    void rollback_to_commit(FreeList& free_list) noexcept {
        spec_map_ = commit_map_;
        free_list.rebuild_from_committed(commit_map_);
    }

    void reset() {
        for (uint16_t i = 0; i < NUM_ARCH_REGS; ++i) {
            spec_map_[i] = i;
            commit_map_[i] = i;
        }
    }

private:
    std::array<uint16_t, NUM_ARCH_REGS> spec_map_{};
    std::array<uint16_t, NUM_ARCH_REGS> commit_map_{};
};

} // namespace tinyarmsim::uarch
