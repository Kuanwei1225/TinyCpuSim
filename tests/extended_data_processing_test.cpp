#include <gtest/gtest.h>
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/instruction.hpp"
#include "tinyarmsim/decoder.hpp"
#include "tinyarmsim/interpreter.hpp"

using namespace tinyarmsim;

class ExtendedDataProcessingTest : public ::testing::Test {
protected:
    ArchitecturalState state;
    MemoryBus bus{1024 * 1024};
    IsaInterpreter interpreter{state, bus};
};

// === Logical & Bitwise Tests ===

TEST_F(ExtendedDataProcessingTest, AndOrrEorBic) {
    state.set_reg(1, 0xF0F0AAAA);
    state.set_reg(2, 0x0F0F5555);

    // AND R0, R1, R2 -> 0x00000000 (Z flag set)
    DecodedInstruction and_op{};
    and_op.op = Opcode::AND;
    and_op.rd = 0;
    and_op.rn = 1;
    and_op.rm = 2;
    and_op.set_flags = true;
    interpreter.execute(and_op);
    EXPECT_EQ(state.get_reg(0), 0x00000000u);
    EXPECT_TRUE(state.get_flag_z());

    // ORR R0, R1, R2 -> 0xFFFFFFFF (N flag set)
    DecodedInstruction orr_op{};
    orr_op.op = Opcode::ORR;
    orr_op.rd = 0;
    orr_op.rn = 1;
    orr_op.rm = 2;
    orr_op.set_flags = true;
    interpreter.execute(orr_op);
    EXPECT_EQ(state.get_reg(0), 0xFFFFFFFFu);
    EXPECT_TRUE(state.get_flag_n());

    // EOR R0, R1, R2 -> 0xFFFFFFFF
    DecodedInstruction eor_op{};
    eor_op.op = Opcode::EOR;
    eor_op.rd = 0;
    eor_op.rn = 1;
    eor_op.rm = 2;
    interpreter.execute(eor_op);
    EXPECT_EQ(state.get_reg(0), 0xFFFFFFFFu);

    // BIC R0, R1, R2 (Bit Clear: R1 AND NOT R2) -> 0xF0F0AAAA AND 0xF0F0AAAA = 0xF0F0AAAA
    DecodedInstruction bic_op{};
    bic_op.op = Opcode::BIC;
    bic_op.rd = 0;
    bic_op.rn = 1;
    bic_op.rm = 2;
    interpreter.execute(bic_op);
    EXPECT_EQ(state.get_reg(0), 0xF0F0AAAAu);
}

TEST_F(ExtendedDataProcessingTest, TstAndTeqAndCmn) {
    state.set_reg(0, 0x80000000);
    state.set_reg(1, 0x00000001);

    // TST R0, R1 (Bitwise AND test) -> result 0 -> Z=1
    DecodedInstruction tst_op{};
    tst_op.op = Opcode::TST;
    tst_op.rn = 0;
    tst_op.rm = 1;
    tst_op.set_flags = true;
    interpreter.execute(tst_op);
    EXPECT_TRUE(state.get_flag_z());

    // TEQ R0, R0 (Test Equivalence: EOR) -> result 0 -> Z=1
    DecodedInstruction teq_op{};
    teq_op.op = Opcode::TEQ;
    teq_op.rn = 0;
    teq_op.rm = 0;
    teq_op.set_flags = true;
    interpreter.execute(teq_op);
    EXPECT_TRUE(state.get_flag_z());

    // CMN R0, R1 (Compare Negative: ADD Rn, Rm) -> 0x80000000 + 1 = 0x80000001 -> N=1, Z=0
    DecodedInstruction cmn_op{};
    cmn_op.op = Opcode::CMN;
    cmn_op.rn = 0;
    cmn_op.rm = 1;
    cmn_op.set_flags = true;
    interpreter.execute(cmn_op);
    EXPECT_TRUE(state.get_flag_n());
    EXPECT_FALSE(state.get_flag_z());
}

// === Arithmetic Tests: ADC, SBC, RSB, MUL, MLA ===

TEST_F(ExtendedDataProcessingTest, AdcAndSbcWithCarry) {
    state.set_reg(1, 10);
    state.set_reg(2, 20);
    state.set_flag_c(true); // Carry is 1

    // ADC R0, R1, R2 -> 10 + 20 + 1 = 31
    DecodedInstruction adc_op{};
    adc_op.op = Opcode::ADC;
    adc_op.rd = 0;
    adc_op.rn = 1;
    adc_op.rm = 2;
    interpreter.execute(adc_op);
    EXPECT_EQ(state.get_reg(0), 31u);

    // SBC R0, R2, R1 -> R2 - R1 - NOT(C) = 20 - 10 - 0 = 10 (with C=1)
    DecodedInstruction sbc_op{};
    sbc_op.op = Opcode::SBC;
    sbc_op.rd = 0;
    sbc_op.rn = 2;
    sbc_op.rm = 1;
    interpreter.execute(sbc_op);
    EXPECT_EQ(state.get_reg(0), 10u);

    // Now with C=0 -> SBC should subtract extra 1: 20 - 10 - 1 = 9
    state.set_flag_c(false);
    interpreter.execute(sbc_op);
    EXPECT_EQ(state.get_reg(0), 9u);
}

TEST_F(ExtendedDataProcessingTest, RsbReverseSubtract) {
    state.set_reg(1, 10);
    state.set_reg(2, 35);

    // RSB R0, R1, R2 -> R2 - R1 = 35 - 10 = 25
    DecodedInstruction rsb_op{};
    rsb_op.op = Opcode::RSB;
    rsb_op.rd = 0;
    rsb_op.rn = 1;
    rsb_op.rm = 2;
    interpreter.execute(rsb_op);
    EXPECT_EQ(state.get_reg(0), 25u);
}

TEST_F(ExtendedDataProcessingTest, MulAndMla) {
    state.set_reg(1, 6);
    state.set_reg(2, 7);
    state.set_reg(3, 10);

    // MUL R0, R1, R2 -> 6 * 7 = 42
    DecodedInstruction mul_op{};
    mul_op.op = Opcode::MUL;
    mul_op.rd = 0;
    mul_op.rn = 1;
    mul_op.rm = 2;
    interpreter.execute(mul_op);
    EXPECT_EQ(state.get_reg(0), 42u);

    // MLA R0, R1, R2, R3 -> (6 * 7) + 10 = 52
    DecodedInstruction mla_op{};
    mla_op.op = Opcode::MLA;
    mla_op.rd = 0;
    mla_op.rn = 1;
    mla_op.rm = 2;
    mla_op.rs = 3;
    interpreter.execute(mla_op);
    EXPECT_EQ(state.get_reg(0), 52u);
}

// === Barrel Shifter Tests ===

TEST_F(ExtendedDataProcessingTest, ShiftsLslLsrAsrRor) {
    state.set_reg(1, 0x80000008);

    // LSL R0, R1, #2 -> 0x00000020
    DecodedInstruction lsl_op{};
    lsl_op.op = Opcode::LSL;
    lsl_op.rd = 0;
    lsl_op.rn = 1;
    lsl_op.is_imm = true;
    lsl_op.imm = 2;
    interpreter.execute(lsl_op);
    EXPECT_EQ(state.get_reg(0), 0x00000020u);

    // LSR R0, R1, #3 -> 0x10000001 (logical shift right zeroes top)
    DecodedInstruction lsr_op{};
    lsr_op.op = Opcode::LSR;
    lsr_op.rd = 0;
    lsr_op.rn = 1;
    lsr_op.is_imm = true;
    lsr_op.imm = 3;
    interpreter.execute(lsr_op);
    EXPECT_EQ(state.get_reg(0), 0x10000001u);

    // ASR R0, R1, #3 -> 0xF0000001 (arithmetic shift right preserves sign bit)
    DecodedInstruction asr_op{};
    asr_op.op = Opcode::ASR;
    asr_op.rd = 0;
    asr_op.rn = 1;
    asr_op.is_imm = true;
    asr_op.imm = 3;
    interpreter.execute(asr_op);
    EXPECT_EQ(state.get_reg(0), 0xF0000001u);

    // ROR R0, R1, #4 -> 0x88000000
    DecodedInstruction ror_op{};
    ror_op.op = Opcode::ROR;
    ror_op.rd = 0;
    ror_op.rn = 1;
    ror_op.is_imm = true;
    ror_op.imm = 4;
    interpreter.execute(ror_op);
    EXPECT_EQ(state.get_reg(0), 0x88000000u);
}

// === Wide Immediate: MOVW / MOVT Tests ===

TEST_F(ExtendedDataProcessingTest, MovwAndMovtBuilds32BitAddress) {
    // MOVW R0, #0x5678 (loads lower 16 bits and zeroes upper 16 bits)
    DecodedInstruction movw_op{};
    movw_op.op = Opcode::MOVW;
    movw_op.rd = 0;
    movw_op.is_imm = true;
    movw_op.imm = 0x5678;
    movw_op.instr_size = 4;
    interpreter.execute(movw_op);
    EXPECT_EQ(state.get_reg(0), 0x00005678u);

    // MOVT R0, #0x1234 (loads upper 16 bits, keeping lower 16 bits intact)
    DecodedInstruction movt_op{};
    movt_op.op = Opcode::MOVT;
    movt_op.rd = 0;
    movt_op.is_imm = true;
    movt_op.imm = 0x1234;
    movt_op.instr_size = 4;
    interpreter.execute(movt_op);
    EXPECT_EQ(state.get_reg(0), 0x12345678u);
}

// === 32-bit Thumb Decoder Tests for MOVW & MOVT ===

TEST(DecoderTest, DecodeThumb32Movw) {
    // MOVW R0, #0x1234 -> 32-bit Thumb-2: 0xF241 0x2034
    // Encoding: 11110 i(0) 10 0 100 0000 0 imm3(001) Rd(0000) imm8(00110100) -> imm16 = 0x1234
    uint16_t w1 = 0xF241;
    uint16_t w2 = 0x2034;
    DecodedInstruction instr = Decoder::decode32(w1, w2);

    EXPECT_EQ(instr.op, Opcode::MOVW);
    EXPECT_EQ(instr.rd, 0u);
    EXPECT_TRUE(instr.is_imm);
    EXPECT_EQ(instr.imm, 0x1234u);
    EXPECT_EQ(instr.instr_size, 4u);
}

TEST(DecoderTest, DecodeThumb32Movt) {
    // MOVT R0, #0x5678 -> 32-bit Thumb-2: 0xF2C5 0x6078
    uint16_t w1 = 0xF2C5;
    uint16_t w2 = 0x6078;
    DecodedInstruction instr = Decoder::decode32(w1, w2);

    EXPECT_EQ(instr.op, Opcode::MOVT);
    EXPECT_EQ(instr.rd, 0u);
    EXPECT_TRUE(instr.is_imm);
    EXPECT_EQ(instr.imm, 0x5678u);
    EXPECT_EQ(instr.instr_size, 4u);
}
