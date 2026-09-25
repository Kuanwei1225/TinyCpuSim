#include <gtest/gtest.h>
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/instruction.hpp"
#include "tinyarmsim/decoder.hpp"
#include "tinyarmsim/interpreter.hpp"

using namespace tinyarmsim;

class LoadStoreTest : public ::testing::Test {
protected:
    ArchitecturalState state;
    MemoryBus bus{1024 * 1024};
    IsaInterpreter interpreter{state, bus};
};

TEST_F(LoadStoreTest, SingleWordLoadAndStore) {
    state.set_reg(1, 0x1000); // Base address
    state.set_reg(2, 0xAABBCCDD);

    // STR R2, [R1, #0]
    DecodedInstruction str_op{};
    str_op.op = Opcode::STR;
    str_op.rd = 2;
    str_op.rn = 1;
    str_op.is_imm = true;
    str_op.imm = 0;
    str_op.mem_size = 4;
    str_op.instr_size = 2;

    interpreter.execute(str_op);
    EXPECT_EQ(bus.read32(0x1000), 0xAABBCCDDu);

    // LDR R0, [R1, #0]
    DecodedInstruction ldr_op{};
    ldr_op.op = Opcode::LDR;
    ldr_op.rd = 0;
    ldr_op.rn = 1;
    ldr_op.is_imm = true;
    ldr_op.imm = 0;
    ldr_op.mem_size = 4;
    ldr_op.instr_size = 2;

    interpreter.execute(ldr_op);
    EXPECT_EQ(state.get_reg(0), 0xAABBCCDDu);
}

TEST_F(LoadStoreTest, ByteAndHalfwordAccessWithExtensions) {
    state.set_reg(1, 0x2000);

    // Write a negative signed byte (0xFE = -2) and negative signed halfword (0xFFF0 = -16)
    bus.write8(0x2000, 0xFE);
    bus.write16(0x2002, 0xFFF0);

    // LDRB R0, [R1, #0] -> zero-extended (0x000000FE)
    DecodedInstruction ldrb_op{};
    ldrb_op.op = Opcode::LDRB;
    ldrb_op.rd = 0;
    ldrb_op.rn = 1;
    ldrb_op.is_imm = true;
    ldrb_op.imm = 0;
    ldrb_op.mem_size = 1;
    ldrb_op.is_signed_mem = false;
    interpreter.execute(ldrb_op);
    EXPECT_EQ(state.get_reg(0), 0x000000FEu);

    // LDRSB R0, [R1, #0] -> sign-extended (0xFFFFFFFE = -2)
    DecodedInstruction ldrsb_op{};
    ldrsb_op.op = Opcode::LDRSB;
    ldrsb_op.rd = 0;
    ldrsb_op.rn = 1;
    ldrsb_op.is_imm = true;
    ldrsb_op.imm = 0;
    ldrsb_op.mem_size = 1;
    ldrsb_op.is_signed_mem = true;
    interpreter.execute(ldrsb_op);
    EXPECT_EQ(state.get_reg(0), 0xFFFFFFFEu);

    // LDRH R0, [R1, #2] -> zero-extended (0x0000FFF0)
    DecodedInstruction ldrh_op{};
    ldrh_op.op = Opcode::LDRH;
    ldrh_op.rd = 0;
    ldrh_op.rn = 1;
    ldrh_op.is_imm = true;
    ldrh_op.imm = 2;
    ldrh_op.mem_size = 2;
    ldrh_op.is_signed_mem = false;
    interpreter.execute(ldrh_op);
    EXPECT_EQ(state.get_reg(0), 0x0000FFF0u);

    // LDRSH R0, [R1, #2] -> sign-extended (0xFFFFFFF0 = -16)
    DecodedInstruction ldrsh_op{};
    ldrsh_op.op = Opcode::LDRSH;
    ldrsh_op.rd = 0;
    ldrsh_op.rn = 1;
    ldrsh_op.is_imm = true;
    ldrsh_op.imm = 2;
    ldrsh_op.mem_size = 2;
    ldrsh_op.is_signed_mem = true;
    interpreter.execute(ldrsh_op);
    EXPECT_EQ(state.get_reg(0), 0xFFFFFFF0u);
}

TEST_F(LoadStoreTest, PushAndPopStackRegisters) {
    state.set_sp(0x8000);
    state.set_reg(0, 0x11111111);
    state.set_reg(1, 0x22222222);
    state.set_lr(0x33333333);

    // PUSH {R0, R1, LR} -> bitmask: (1 << 0) | (1 << 1) | (1 << 14)
    DecodedInstruction push_op{};
    push_op.op = Opcode::PUSH;
    push_op.register_list = (1 << 0) | (1 << 1) | (1 << 14);
    push_op.instr_size = 2;

    interpreter.execute(push_op);
    // Pushed 3 registers (12 bytes) -> SP = 0x8000 - 12 = 0x7FF4
    EXPECT_EQ(state.get_sp(), 0x7FF4u);
    EXPECT_EQ(bus.read32(0x7FF4), 0x11111111u);
    EXPECT_EQ(bus.read32(0x7FF8), 0x22222222u);
    EXPECT_EQ(bus.read32(0x7FFC), 0x33333333u);

    // Clear registers
    state.set_reg(0, 0);
    state.set_reg(1, 0);
    state.set_pc(0x1000);

    // POP {R0, R1, PC} -> bitmask: (1 << 0) | (1 << 1) | (1 << 15)
    DecodedInstruction pop_op{};
    pop_op.op = Opcode::POP;
    pop_op.register_list = (1 << 0) | (1 << 1) | (1 << 15);
    pop_op.instr_size = 2;

    interpreter.execute(pop_op);
    EXPECT_EQ(state.get_sp(), 0x8000u);
    EXPECT_EQ(state.get_reg(0), 0x11111111u);
    EXPECT_EQ(state.get_reg(1), 0x22222222u);
    EXPECT_EQ(state.get_pc(), 0x33333332u); // PC loaded from pop (cleared bit 0)
}

TEST_F(LoadStoreTest, LdmAndStmBlockTransfer) {
    state.set_reg(4, 0x4000); // Base pointer in R4
    state.set_reg(0, 10);
    state.set_reg(1, 20);
    state.set_reg(2, 30);

    // STM R4!, {R0, R1, R2}
    DecodedInstruction stm_op{};
    stm_op.op = Opcode::STM;
    stm_op.rn = 4;
    stm_op.register_list = (1 << 0) | (1 << 1) | (1 << 2);
    stm_op.writeback = true;
    stm_op.instr_size = 2;

    interpreter.execute(stm_op);
    EXPECT_EQ(bus.read32(0x4000), 10u);
    EXPECT_EQ(bus.read32(0x4004), 20u);
    EXPECT_EQ(bus.read32(0x4008), 30u);
    EXPECT_EQ(state.get_reg(4), 0x400Cu); // Base updated

    // Clear registers and LDM
    state.set_reg(0, 0);
    state.set_reg(1, 0);
    state.set_reg(2, 0);
    state.set_reg(4, 0x4000);

    // LDM R4!, {R0, R1, R2}
    DecodedInstruction ldm_op{};
    ldm_op.op = Opcode::LDM;
    ldm_op.rn = 4;
    ldm_op.register_list = (1 << 0) | (1 << 1) | (1 << 2);
    ldm_op.writeback = true;
    ldm_op.instr_size = 2;

    interpreter.execute(ldm_op);
    EXPECT_EQ(state.get_reg(0), 10u);
    EXPECT_EQ(state.get_reg(1), 20u);
    EXPECT_EQ(state.get_reg(2), 30u);
    EXPECT_EQ(state.get_reg(4), 0x400Cu);
}

// === Decoder Tests for Load/Store ===

TEST(DecoderLoadStoreTest, DecodeThumb16LdrStrImmediate) {
    // 0x6808 = LDR R0, [R1, #0] (Thumb 16-bit: 0110 1 imm5(00000) Rn(001) Rd(000))
    uint16_t raw_ldr = 0x6808;
    DecodedInstruction instr = Decoder::decode16(raw_ldr);

    EXPECT_EQ(instr.op, Opcode::LDR);
    EXPECT_EQ(instr.rd, 0u);
    EXPECT_EQ(instr.rn, 1u);
    EXPECT_TRUE(instr.is_imm);
    EXPECT_EQ(instr.imm, 0u);
    EXPECT_EQ(instr.mem_size, 4u);
}

TEST(DecoderLoadStoreTest, DecodeThumb16PushPop) {
    // 0xB503 = PUSH {R0, R1, LR} (Thumb 16-bit: 1011 0 10 M(1) rlist(00000011))
    uint16_t raw_push = 0xB503;
    DecodedInstruction push_instr = Decoder::decode16(raw_push);

    EXPECT_EQ(push_instr.op, Opcode::PUSH);
    EXPECT_EQ(push_instr.register_list, (1 << 0) | (1 << 1) | (1 << 14));

    // 0xBD03 = POP {R0, R1, PC} (Thumb 16-bit: 1011 1 10 P(1) rlist(00000011))
    uint16_t raw_pop = 0xBD03;
    DecodedInstruction pop_instr = Decoder::decode16(raw_pop);

    EXPECT_EQ(pop_instr.op, Opcode::POP);
    EXPECT_EQ(pop_instr.register_list, (1 << 0) | (1 << 1) | (1 << 15));
}
