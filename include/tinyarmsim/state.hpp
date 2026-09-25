#pragma once

#include <array>
#include <cstdint>
#include "tinyarmsim/faults.hpp"

namespace tinyarmsim {

// CPSR / APSR flag bit positions
constexpr uint32_t FLAG_N_BIT = 31; // Negative
constexpr uint32_t FLAG_Z_BIT = 30; // Zero
constexpr uint32_t FLAG_C_BIT = 29; // Carry
constexpr uint32_t FLAG_V_BIT = 28; // Overflow
constexpr uint32_t FLAG_T_BIT = 5;  // Thumb state

class ArchitecturalState {
public:
    ArchitecturalState() noexcept {
        reset();
    }

    void reset() noexcept {
        registers_.fill(0);
        cpsr_ = 0;
    }

    [[nodiscard]] uint32_t get_reg(size_t reg_num) const {
        if (reg_num >= 16) {
            throw CpuFaultException(FaultType::IllegalRegister, "Register index out of bounds: " + std::to_string(reg_num));
        }
        return registers_[reg_num];
    }

    void set_reg(size_t reg_num, uint32_t val) {
        if (reg_num >= 16) {
            throw CpuFaultException(FaultType::IllegalRegister, "Register index out of bounds: " + std::to_string(reg_num));
        }
        registers_[reg_num] = val;
    }

    // Convenience register accessors
    [[nodiscard]] uint32_t get_sp() const noexcept { return registers_[13]; }
    void set_sp(uint32_t val) noexcept { registers_[13] = val; }

    [[nodiscard]] uint32_t get_lr() const noexcept { return registers_[14]; }
    void set_lr(uint32_t val) noexcept { registers_[14] = val; }

    [[nodiscard]] uint32_t get_pc() const noexcept { return registers_[15]; }
    void set_pc(uint32_t val) noexcept { registers_[15] = val; }
    void advance_pc(uint32_t bytes) noexcept { registers_[15] += bytes; }

    // Status register and NZCV flags
    [[nodiscard]] uint32_t get_cpsr() const noexcept { return cpsr_; }
    void set_cpsr(uint32_t val) noexcept { cpsr_ = val; }

    [[nodiscard]] bool get_flag_n() const noexcept { return (cpsr_ & (1u << FLAG_N_BIT)) != 0; }
    [[nodiscard]] bool get_flag_z() const noexcept { return (cpsr_ & (1u << FLAG_Z_BIT)) != 0; }
    [[nodiscard]] bool get_flag_c() const noexcept { return (cpsr_ & (1u << FLAG_C_BIT)) != 0; }
    [[nodiscard]] bool get_flag_v() const noexcept { return (cpsr_ & (1u << FLAG_V_BIT)) != 0; }
    [[nodiscard]] bool get_flag_t() const noexcept { return (cpsr_ & (1u << FLAG_T_BIT)) != 0; }

    void set_flag_n(bool val) noexcept { set_flag_bit(FLAG_N_BIT, val); }
    void set_flag_z(bool val) noexcept { set_flag_bit(FLAG_Z_BIT, val); }
    void set_flag_c(bool val) noexcept { set_flag_bit(FLAG_C_BIT, val); }
    void set_flag_v(bool val) noexcept { set_flag_bit(FLAG_V_BIT, val); }
    void set_flag_t(bool val) noexcept { set_flag_bit(FLAG_T_BIT, val); }

    void set_flags(bool n, bool z, bool c, bool v) noexcept {
        set_flag_n(n);
        set_flag_z(z);
        set_flag_c(c);
        set_flag_v(v);
    }

private:
    void set_flag_bit(uint32_t bit, bool val) noexcept {
        if (val) {
            cpsr_ |= (1u << bit);
        } else {
            cpsr_ &= ~(1u << bit);
        }
    }

    std::array<uint32_t, 16> registers_{};
    uint32_t cpsr_{0};
};

} // namespace tinyarmsim
