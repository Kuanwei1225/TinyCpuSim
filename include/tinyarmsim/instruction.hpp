#pragma once

#include <cstdint>
#include <string_view>

namespace tinyarmsim {

enum class Opcode {
    UNKNOWN = 0,
    // Data processing
    MOV,
    MVN,
    MOVT,
    MOVW,
    ADD,
    ADC,
    SUB,
    SBC,
    RSB,
    MUL,
    MLA,
    AND,
    ORR,
    EOR,
    BIC,
    CMP,
    CMN,
    TST,
    TEQ,
    ASR,
    LSL,
    LSR,
    ROR,
    // Branching
    B,
    BL,
    BX,
    BLX,
    CBZ,
    CBNZ,
    // Memory access
    LDR,
    LDRB,
    LDRH,
    LDRSB,
    LDRSH,
    STR,
    STRB,
    STRH,
    LDM,
    STM,
    PUSH,
    POP,
    // System
    MRS,
    MSR,
    SVC,
    NOP
};

enum class ConditionCode : uint8_t {
    EQ = 0b0000, // Equal (Z == 1)
    NE = 0b0001, // Not equal (Z == 0)
    CS = 0b0010, // Carry set / Unsigned higher or same (C == 1)
    CC = 0b0011, // Carry clear / Unsigned lower (C == 0)
    MI = 0b0100, // Minus / Negative (N == 1)
    PL = 0b0101, // Plus / Positive or zero (N == 0)
    VS = 0b0110, // Overflow (V == 1)
    VC = 0b0111, // No overflow (V == 0)
    HI = 0b1000, // Unsigned higher (C == 1 and Z == 0)
    LS = 0b1001, // Unsigned lower or same (C == 0 or Z == 1)
    GE = 0b1010, // Signed greater than or equal (N == V)
    LT = 0b1011, // Signed less than (N != V)
    GT = 0b1100, // Signed greater than (Z == 0 and N == V)
    LE = 0b1101, // Signed less than or equal (Z == 1 or N != V)
    AL = 0b1110, // Always (unconditional)
    NV = 0b1111  // Never / Unpredictable
};

enum class ShiftType : uint8_t {
    LSL = 0,
    LSR = 1,
    ASR = 2,
    ROR = 3,
    RRX = 4
};

struct DecodedInstruction {
    Opcode op{Opcode::UNKNOWN};
    ConditionCode cond{ConditionCode::AL};
    uint8_t rd{0};
    uint8_t rn{0};
    uint8_t rm{0};
    uint8_t rs{0};

    bool is_imm{false};
    uint32_t imm{0};

    ShiftType shift_type{ShiftType::LSL};
    uint8_t shift_amount{0};
    bool shift_by_reg{false};

    bool set_flags{false};
    uint8_t instr_size{2}; // 2 bytes for 16-bit Thumb, 4 bytes for 32-bit Thumb/ARM

    // Memory access parameters
    bool is_load{false};
    uint8_t mem_size{4}; // 1 = byte, 2 = halfword, 4 = word
    bool is_signed_mem{false};
    bool writeback{false};
    bool pre_indexed{true};
    uint16_t register_list{0}; // Bitmask for LDM/STM/PUSH/POP
};

} // namespace tinyarmsim
