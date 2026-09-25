#pragma once

#include <cstdint>
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/instruction.hpp"
#include "tinyarmsim/decoder.hpp"
#include "tinyarmsim/faults.hpp"

namespace tinyarmsim {

class IsaInterpreter {
public:
    IsaInterpreter(ArchitecturalState& state, MemoryBus& bus) noexcept
        : state_(state), bus_(bus) {}

    [[nodiscard]] bool evaluate_condition(ConditionCode cond) const noexcept {
        bool n = state_.get_flag_n();
        bool z = state_.get_flag_z();
        bool c = state_.get_flag_c();
        bool v = state_.get_flag_v();

        switch (cond) {
            case ConditionCode::EQ: return z;
            case ConditionCode::NE: return !z;
            case ConditionCode::CS: return c;
            case ConditionCode::CC: return !c;
            case ConditionCode::MI: return n;
            case ConditionCode::PL: return !n;
            case ConditionCode::VS: return v;
            case ConditionCode::VC: return !v;
            case ConditionCode::HI: return c && !z;
            case ConditionCode::LS: return !c || z;
            case ConditionCode::GE: return n == v;
            case ConditionCode::LT: return n != v;
            case ConditionCode::GT: return !z && (n == v);
            case ConditionCode::LE: return z || (n != v);
            case ConditionCode::AL: return true;
            case ConditionCode::NV: return false;
        }
        return false;
    }

    void execute(const DecodedInstruction& instr) {
        if (!evaluate_condition(instr.cond)) {
            state_.advance_pc(instr.instr_size);
            return;
        }

        bool pc_written = false;

        switch (instr.op) {
            case Opcode::MOV: {
                uint32_t val = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                state_.set_reg(instr.rd, val);
                if (instr.rd == 15) {
                    pc_written = true;
                }
                if (instr.set_flags) {
                    state_.set_flag_n((val & 0x80000000u) != 0);
                    state_.set_flag_z(val == 0);
                }
                break;
            }

            case Opcode::MVN: {
                uint32_t val = ~(instr.is_imm ? instr.imm : state_.get_reg(instr.rm));
                state_.set_reg(instr.rd, val);
                if (instr.set_flags) {
                    state_.set_flag_n((val & 0x80000000u) != 0);
                    state_.set_flag_z(val == 0);
                }
                break;
            }

            case Opcode::MOVW: {
                state_.set_reg(instr.rd, instr.imm & 0xFFFFu);
                break;
            }

            case Opcode::MOVT: {
                uint32_t current = state_.get_reg(instr.rd);
                uint32_t updated = (current & 0x0000FFFFu) | ((instr.imm & 0xFFFFu) << 16);
                state_.set_reg(instr.rd, updated);
                break;
            }

            case Opcode::ADD: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint64_t result = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2);
                uint32_t res32 = static_cast<uint32_t>(result);

                state_.set_reg(instr.rd, res32);
                if (instr.rd == 15) {
                    pc_written = true;
                }

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (result > 0xFFFFFFFFu);
                    bool v = (~(op1 ^ op2) & (op1 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::ADC: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t carry_in = state_.get_flag_c() ? 1u : 0u;
                uint64_t result = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2) + carry_in;
                uint32_t res32 = static_cast<uint32_t>(result);

                state_.set_reg(instr.rd, res32);

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (result > 0xFFFFFFFFu);
                    bool v = (~(op1 ^ op2) & (op1 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::SUB:
            case Opcode::CMP: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint64_t result = static_cast<uint64_t>(op1) - static_cast<uint64_t>(op2);
                uint32_t res32 = static_cast<uint32_t>(result);

                if (instr.op != Opcode::CMP) {
                    state_.set_reg(instr.rd, res32);
                    if (instr.rd == 15) {
                        pc_written = true;
                    }
                }

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (op1 >= op2);
                    bool v = ((op1 ^ op2) & (op1 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::SBC: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t borrow = state_.get_flag_c() ? 0u : 1u;
                uint64_t result = static_cast<uint64_t>(op1) - static_cast<uint64_t>(op2) - borrow;
                uint32_t res32 = static_cast<uint32_t>(result);

                state_.set_reg(instr.rd, res32);

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (static_cast<uint64_t>(op1) >= static_cast<uint64_t>(op2) + borrow);
                    bool v = ((op1 ^ op2) & (op1 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::RSB: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint64_t result = static_cast<uint64_t>(op2) - static_cast<uint64_t>(op1);
                uint32_t res32 = static_cast<uint32_t>(result);

                state_.set_reg(instr.rd, res32);

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (op2 >= op1);
                    bool v = ((op2 ^ op1) & (op2 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::MUL: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = state_.get_reg(instr.rm);
                uint32_t res = op1 * op2;
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::MLA: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = state_.get_reg(instr.rm);
                uint32_t op3 = state_.get_reg(instr.rs);
                uint32_t res = (op1 * op2) + op3;
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::AND:
            case Opcode::TST: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t res = op1 & op2;
                if (instr.op != Opcode::TST) {
                    state_.set_reg(instr.rd, res);
                }
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::ORR: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t res = op1 | op2;
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::EOR:
            case Opcode::TEQ: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t res = op1 ^ op2;
                if (instr.op != Opcode::TEQ) {
                    state_.set_reg(instr.rd, res);
                }
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::BIC: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t res = op1 & (~op2);
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::CMN: {
                uint32_t op1 = state_.get_reg(instr.rn);
                uint32_t op2 = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint64_t result = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2);
                uint32_t res32 = static_cast<uint32_t>(result);

                if (instr.set_flags) {
                    bool n = (res32 & 0x80000000u) != 0;
                    bool z = (res32 == 0);
                    bool c = (result > 0xFFFFFFFFu);
                    bool v = (~(op1 ^ op2) & (op1 ^ res32) & 0x80000000u) != 0;
                    state_.set_flags(n, z, c, v);
                }
                break;
            }

            case Opcode::LSL: {
                uint32_t val = state_.get_reg(instr.rn);
                uint32_t shift = instr.is_imm ? instr.imm : (state_.get_reg(instr.rm) & 0xFFu);
                uint32_t res = (shift >= 32) ? 0 : (val << shift);
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::LSR: {
                uint32_t val = state_.get_reg(instr.rn);
                uint32_t shift = instr.is_imm ? instr.imm : (state_.get_reg(instr.rm) & 0xFFu);
                uint32_t res = (shift >= 32) ? 0 : (val >> shift);
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::ASR: {
                int32_t val = static_cast<int32_t>(state_.get_reg(instr.rn));
                uint32_t shift = instr.is_imm ? instr.imm : (state_.get_reg(instr.rm) & 0xFFu);
                if (shift >= 32) shift = 31;
                uint32_t res = static_cast<uint32_t>(val >> shift);
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            case Opcode::ROR: {
                uint32_t val = state_.get_reg(instr.rn);
                uint32_t shift = (instr.is_imm ? instr.imm : state_.get_reg(instr.rm)) & 31u;
                uint32_t res = (shift == 0) ? val : ((val >> shift) | (val << (32 - shift)));
                state_.set_reg(instr.rd, res);
                if (instr.set_flags) {
                    state_.set_flag_n((res & 0x80000000u) != 0);
                    state_.set_flag_z(res == 0);
                }
                break;
            }

            // === Branching Instructions ===
            case Opcode::B: {
                state_.set_pc(instr.imm);
                pc_written = true;
                break;
            }

            case Opcode::BL:
            case Opcode::BLX: {
                uint32_t return_address = state_.get_pc() + instr.instr_size;
                state_.set_lr(return_address | 1u);
                uint32_t target = instr.is_imm ? instr.imm : (state_.get_reg(instr.rm) & ~1u);
                state_.set_pc(target);
                pc_written = true;
                break;
            }

            case Opcode::BX: {
                uint32_t target = state_.get_reg(instr.rm) & ~1u;
                state_.set_pc(target);
                pc_written = true;
                break;
            }

            case Opcode::CBZ: {
                if (state_.get_reg(instr.rn) == 0) {
                    state_.set_pc(instr.imm);
                    pc_written = true;
                }
                break;
            }

            case Opcode::CBNZ: {
                if (state_.get_reg(instr.rn) != 0) {
                    state_.set_pc(instr.imm);
                    pc_written = true;
                }
                break;
            }

            // === Load / Store Single ===
            case Opcode::LDR:
            case Opcode::LDRB:
            case Opcode::LDRH:
            case Opcode::LDRSB:
            case Opcode::LDRSH: {
                uint32_t base = state_.get_reg(instr.rn);
                uint32_t offset = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t addr = base + offset;

                uint32_t loaded_val = 0;
                if (instr.op == Opcode::LDR) {
                    loaded_val = bus_.read32(addr);
                } else if (instr.op == Opcode::LDRB) {
                    loaded_val = bus_.read8(addr);
                } else if (instr.op == Opcode::LDRH) {
                    loaded_val = bus_.read16(addr);
                } else if (instr.op == Opcode::LDRSB) {
                    loaded_val = static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(bus_.read8(addr))));
                } else if (instr.op == Opcode::LDRSH) {
                    loaded_val = static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(bus_.read16(addr))));
                }

                state_.set_reg(instr.rd, loaded_val);
                if (instr.rd == 15) {
                    state_.set_pc(loaded_val & ~1u);
                    pc_written = true;
                }
                break;
            }

            case Opcode::STR:
            case Opcode::STRB:
            case Opcode::STRH: {
                uint32_t base = state_.get_reg(instr.rn);
                uint32_t offset = instr.is_imm ? instr.imm : state_.get_reg(instr.rm);
                uint32_t addr = base + offset;
                uint32_t val = state_.get_reg(instr.rd);

                if (instr.op == Opcode::STR) {
                    bus_.write32(addr, val);
                } else if (instr.op == Opcode::STRB) {
                    bus_.write8(addr, static_cast<uint8_t>(val & 0xFF));
                } else if (instr.op == Opcode::STRH) {
                    bus_.write16(addr, static_cast<uint16_t>(val & 0xFFFF));
                }
                break;
            }

            // === Stack Operations: PUSH / POP ===
            case Opcode::PUSH: {
                uint32_t sp = state_.get_sp();
                // Count registers
                int reg_count = 0;
                for (int i = 0; i < 16; ++i) {
                    if (instr.register_list & (1 << i)) ++reg_count;
                }
                uint32_t new_sp = sp - static_cast<uint32_t>(reg_count * 4);
                uint32_t cur_addr = new_sp;
                for (int i = 0; i < 16; ++i) {
                    if (instr.register_list & (1 << i)) {
                        bus_.write32(cur_addr, state_.get_reg(static_cast<size_t>(i)));
                        cur_addr += 4;
                    }
                }
                state_.set_sp(new_sp);
                break;
            }

            case Opcode::POP: {
                uint32_t sp = state_.get_sp();
                uint32_t cur_addr = sp;
                for (int i = 0; i < 16; ++i) {
                    if (instr.register_list & (1 << i)) {
                        uint32_t val = bus_.read32(cur_addr);
                        state_.set_reg(static_cast<size_t>(i), val);
                        if (i == 15) {
                            state_.set_pc(val & ~1u);
                            pc_written = true;
                        }
                        cur_addr += 4;
                    }
                }
                state_.set_sp(cur_addr);
                break;
            }

            // === Multiple Load/Store: LDM / STM ===
            case Opcode::STM: {
                uint32_t base = state_.get_reg(instr.rn);
                uint32_t cur_addr = base;
                for (int i = 0; i < 16; ++i) {
                    if (instr.register_list & (1 << i)) {
                        bus_.write32(cur_addr, state_.get_reg(static_cast<size_t>(i)));
                        cur_addr += 4;
                    }
                }
                if (instr.writeback) {
                    state_.set_reg(instr.rn, cur_addr);
                }
                break;
            }

            case Opcode::LDM: {
                uint32_t base = state_.get_reg(instr.rn);
                uint32_t cur_addr = base;
                for (int i = 0; i < 16; ++i) {
                    if (instr.register_list & (1 << i)) {
                        uint32_t val = bus_.read32(cur_addr);
                        state_.set_reg(static_cast<size_t>(i), val);
                        if (i == 15) {
                            state_.set_pc(val & ~1u);
                            pc_written = true;
                        }
                        cur_addr += 4;
                    }
                }
                if (instr.writeback) {
                    state_.set_reg(instr.rn, cur_addr);
                }
                break;
            }

            // === System Control ===
            case Opcode::MRS: {
                state_.set_reg(instr.rd, state_.get_cpsr());
                break;
            }

            case Opcode::MSR: {
                uint32_t val = state_.get_reg(instr.rn);
                // Update CPSR flags (NZCV)
                state_.set_cpsr((state_.get_cpsr() & 0x0FFFFFFFu) | (val & 0xF0000000u));
                break;
            }

            case Opcode::NOP:
                break;

            case Opcode::SVC:
                throw CpuFaultException(FaultType::SoftwareInterrupt, "SVC interrupt with code " + std::to_string(instr.imm));

            default:
                throw CpuFaultException(FaultType::UndefinedInstruction, "Unhandled opcode in interpreter");
        }

        if (!pc_written) {
            state_.advance_pc(instr.instr_size);
        }
    }

    void step() {
        uint32_t pc = state_.get_pc();
        uint16_t first_halfword = bus_.read16(pc);
        if (Decoder::is_32bit_thumb(first_halfword)) {
            uint16_t second_halfword = bus_.read16(pc + 2);
            DecodedInstruction instr = Decoder::decode32(first_halfword, second_halfword, pc);
            execute(instr);
        } else {
            DecodedInstruction instr = Decoder::decode16(first_halfword, pc);
            execute(instr);
        }
    }

    uint32_t run(uint64_t max_steps = 1000000) {
        for (uint64_t step_count = 0; step_count < max_steps; ++step_count) {
            try {
                step();
            } catch (const CpuFaultException& e) {
                if (e.get_fault_type() == FaultType::SoftwareInterrupt) {
                    return state_.get_reg(0); // Exit code in R0
                }
                throw;
            }
        }
        throw CpuFaultException(FaultType::MemoryOutOfBounds, "Simulation execution exceeded max step limit (" + std::to_string(max_steps) + ")");
    }

private:
    ArchitecturalState& state_;
    MemoryBus& bus_;
};

} // namespace tinyarmsim
