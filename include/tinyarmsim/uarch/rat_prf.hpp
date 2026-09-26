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
    explicit FreeList(size_t num_phys_regs = 128, size_t num_arch_regs = 16)
        : total_regs_(num_phys_regs) {
        if (num_phys_regs <= num_arch_regs) {
            throw std::invalid_argument("FreeList: num_phys_regs must exceed num_arch_regs");
        }
        // Initially registers 16..num_phys_regs-1 are free
        for (size_t i = num_arch_regs; i < num_phys_regs; ++i) {
            free_regs_.push_back(static_cast<uint16_t>(i));
        }
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
        return reg;
    }

    void free(uint16_t phys_reg) noexcept {
        free_regs_.push_back(phys_reg);
    }

    void reset(size_t num_arch_regs = 16) {
        free_regs_.clear();
        for (size_t i = num_arch_regs; i < total_regs_; ++i) {
            free_regs_.push_back(static_cast<uint16_t>(i));
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
    std::deque<uint16_t> free_regs_;
};

// -----------------------------------------------------------------------------
// Register Alias Table (RAT) & Branch Checkpointing
// -----------------------------------------------------------------------------
class RegisterAliasTable {
public:
    struct Checkpoint {
        std::array<uint16_t, 16> rat_map;
        std::deque<uint16_t> free_list_state;
    };

    RegisterAliasTable() {
        reset();
    }

    [[nodiscard]] uint16_t get(uint8_t arch_reg) const {
        if (arch_reg >= 16) {
            throw std::out_of_range("Architectural register index >= 16: " + std::to_string(arch_reg));
        }
        return spec_map_[arch_reg];
    }

    void set(uint8_t arch_reg, uint16_t phys_reg) {
        if (arch_reg >= 16) {
            throw std::out_of_range("Architectural register index >= 16: " + std::to_string(arch_reg));
        }
        spec_map_[arch_reg] = phys_reg;
    }

    [[nodiscard]] uint16_t get_commit(uint8_t arch_reg) const {
        if (arch_reg >= 16) {
            throw std::out_of_range("Architectural register index >= 16");
        }
        return commit_map_[arch_reg];
    }

    void commit(uint8_t arch_reg, uint16_t phys_reg) {
        if (arch_reg >= 16) return;
        commit_map_[arch_reg] = phys_reg;
    }

    [[nodiscard]] Checkpoint create_checkpoint(const FreeList& free_list) const {
        Checkpoint cp;
        cp.rat_map = spec_map_;
        cp.free_list_state = free_list.get_state();
        return cp;
    }

    void restore_checkpoint(const Checkpoint& cp, FreeList& free_list) noexcept {
        spec_map_ = cp.rat_map;
        free_list.restore_state(cp.free_list_state);
    }

    void rollback_to_commit(FreeList& free_list) noexcept {
        spec_map_ = commit_map_;
        free_list.reset(16);
    }

    void reset() {
        for (uint16_t i = 0; i < 16; ++i) {
            spec_map_[i] = i;
            commit_map_[i] = i;
        }
    }

private:
    std::array<uint16_t, 16> spec_map_{};
    std::array<uint16_t, 16> commit_map_{};
};

} // namespace tinyarmsim::uarch
