#include <gtest/gtest.h>
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/instruction.hpp"
#include "tinyarmsim/decoder.hpp"
#include "tinyarmsim/interpreter.hpp"

using namespace tinyarmsim;

class SystemControlAndSvcTest : public ::testing::Test {
protected:
    ArchitecturalState state;
    MemoryBus bus{1024 * 1024};
    IsaInterpreter interpreter{state, bus};
};

TEST_F(SystemControlAndSvcTest, MrsAndMsrManipulateFlags) {
    // Set NZCV flags: N=1, Z=0, C=1, V=0 -> CPSR flags = 0xA0000000
    state.set_flags(true, false, true, false);

    // MRS R0, APSR (read CPSR/APSR into R0)
    DecodedInstruction mrs_op{};
    mrs_op.op = Opcode::MRS;
    mrs_op.rd = 0;
    mrs_op.instr_size = 4;

    interpreter.execute(mrs_op);
    EXPECT_EQ(state.get_reg(0) & 0xF0000000u, 0xA0000000u);

    // Now modify R1 to 0x40000000 (Z=1, others 0) and write back via MSR
    state.set_reg(1, 0x40000000u);
    DecodedInstruction msr_op{};
    msr_op.op = Opcode::MSR;
    msr_op.rn = 1;
    msr_op.instr_size = 4;

    interpreter.execute(msr_op);
    EXPECT_FALSE(state.get_flag_n());
    EXPECT_TRUE(state.get_flag_z());
    EXPECT_FALSE(state.get_flag_c());
    EXPECT_FALSE(state.get_flag_v());
}

TEST_F(SystemControlAndSvcTest, NopAdvancesPc) {
    state.set_pc(0x1000);
    DecodedInstruction nop_op{};
    nop_op.op = Opcode::NOP;
    nop_op.instr_size = 2;

    interpreter.execute(nop_op);
    EXPECT_EQ(state.get_pc(), 0x1002u);
}

TEST_F(SystemControlAndSvcTest, RunLoopHaltsOnSvcAndReturnsR0) {
    // Write a small program into memory:
    // 0x1000: MOVS R0, #42   (0x202A)
    // 0x1002: SVC #0         (0xDF00)
    bus.write16(0x1000, 0x202A);
    bus.write16(0x1002, 0xDF00);

    state.set_pc(0x1000);
    uint32_t exit_code = interpreter.run(100);

    EXPECT_EQ(exit_code, 42u);
    EXPECT_EQ(state.get_reg(0), 42u);
    EXPECT_EQ(state.get_pc(), 0x1002u);
}

TEST_F(SystemControlAndSvcTest, RunLoopDetectsMaxStepTimeout) {
    // Infinite loop: 0x1000: B 0x1000 (0xE7FE: imm11 = -1 -> offset = -2 -> PC+4-2 = 0x1002?? offset=-1*2=-2 -> 0x1000+4-4 = 0x1000)
    bus.write16(0x1000, 0xE7FE);
    state.set_pc(0x1000);

    EXPECT_THROW(interpreter.run(50), CpuFaultException);
}

// === Decoder Tests for MRS / MSR ===

TEST(DecoderSystemTest, DecodeThumb32MrsMsr) {
    // MRS R0, APSR: 0xF3EF 0x8000
    uint16_t w1_mrs = 0xF3EF;
    uint16_t w2_mrs = 0x8000;
    DecodedInstruction mrs_instr = Decoder::decode32(w1_mrs, w2_mrs);

    EXPECT_EQ(mrs_instr.op, Opcode::MRS);
    EXPECT_EQ(mrs_instr.rd, 0u);
    EXPECT_EQ(mrs_instr.instr_size, 4u);

    // MSR APSR_nzcvq, R1: 0xF381 0x8800
    uint16_t w1_msr = 0xF381;
    uint16_t w2_msr = 0x8800;
    DecodedInstruction msr_instr = Decoder::decode32(w1_msr, w2_msr);

    EXPECT_EQ(msr_instr.op, Opcode::MSR);
    EXPECT_EQ(msr_instr.rn, 1u);
    EXPECT_EQ(msr_instr.instr_size, 4u);
}
