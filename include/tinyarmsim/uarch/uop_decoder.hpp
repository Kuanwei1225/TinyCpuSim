#pragma once

#include <cstdint>
#include <vector>
#include "tinyarmsim/instruction.hpp"
#include "tinyarmsim/uarch/uop.hpp"

namespace tinyarmsim::uarch {

class UOpDecoder {
public:
    // Expand a single functional DecodedInstruction into 1 or more uOps
    [[nodiscard]] static std::vector<UOp> decode(const DecodedInstruction& instr, uint32_t pc, uint64_t seq_num) {
        std::vector<UOp> uops;

        switch (instr.op) {
            // -------------------------------------------------------------
            // Store Operations: Split into decoupled uop_STA and uop_STD
            // -------------------------------------------------------------
            case Opcode::STR:
            case Opcode::STRB:
            case Opcode::STRH: {
                // 1. Store Address Generation (uop_STA) -> Target: Port 3 Dual AGU
                UOp sta;
                sta.seq_num = seq_num;
                sta.pc = pc;
                sta.raw_inst = instr.raw_hex;
                sta.is_thumb32 = (instr.instr_size == 4);
                sta.type = UOpType::STORE_ADDR;
                sta.target_port = ExecutionPort::PORT_3_DUAL_AGU;
                sta.opcode = instr.op;
                sta.arch_src1 = instr.rn; // Base register
                sta.arch_src2 = instr.is_imm ? UOp::INVALID_REG : instr.rm; // Offset register or immediate
                sta.arch_dest = UOp::INVALID_REG;
                sta.imm = instr.imm;
                sta.offset = static_cast<int32_t>(instr.imm);
                sta.is_imm_valid = instr.is_imm;
                sta.mem_size_bytes = (instr.op == Opcode::STRB) ? 1 : ((instr.op == Opcode::STRH) ? 2 : 4);

                // 2. Store Data Generation (uop_STD) -> Target: Port 4 Store Data
                UOp std;
                std.seq_num = seq_num;
                std.pc = pc;
                std.raw_inst = instr.raw_hex;
                std.is_thumb32 = (instr.instr_size == 4);
                std.type = UOpType::STORE_DATA;
                std.target_port = ExecutionPort::PORT_4_STORE_DATA;
                std.opcode = instr.op;
                std.arch_src1 = instr.rd; // Value register to store
                std.arch_dest = UOp::INVALID_REG;
                std.mem_size_bytes = sta.mem_size_bytes;

                uops.push_back(sta);
                uops.push_back(std);
                break;
            }

            // -------------------------------------------------------------
            // Load Operations: Single LOAD uop -> Target: Port 2 / Port 3
            // -------------------------------------------------------------
            case Opcode::LDR:
            case Opcode::LDRB:
            case Opcode::LDRH:
            case Opcode::LDRSB:
            case Opcode::LDRSH: {
                UOp lda;
                lda.seq_num = seq_num;
                lda.pc = pc;
                lda.raw_inst = instr.raw_hex;
                lda.is_thumb32 = (instr.instr_size == 4);
                lda.type = UOpType::LOAD;
                lda.target_port = ExecutionPort::PORT_2_LOAD_AGU;
                lda.opcode = instr.op;
                lda.arch_dest = instr.rd;
                lda.arch_src1 = instr.rn;
                lda.arch_src2 = instr.is_imm ? UOp::INVALID_REG : instr.rm;
                lda.imm = instr.imm;
                lda.offset = static_cast<int32_t>(instr.imm);
                lda.is_imm_valid = instr.is_imm;
                lda.mem_size_bytes = (instr.op == Opcode::LDRB || instr.op == Opcode::LDRSB) ? 1 :
                                     ((instr.op == Opcode::LDRH || instr.op == Opcode::LDRSH) ? 2 : 4);
                lda.is_signed_mem = (instr.op == Opcode::LDRSB || instr.op == Opcode::LDRSH);
                uops.push_back(lda);
                break;
            }

            // -------------------------------------------------------------
            // Multi-Register Load/Store (PUSH, POP, LDM, STM)
            // -------------------------------------------------------------
            case Opcode::PUSH:
            case Opcode::STM: {
                uint8_t base_reg = (instr.op == Opcode::PUSH) ? 13 : instr.rn;
                int32_t curr_offset = 0;
                for (uint8_t r = 0; r < 16; ++r) {
                    if ((instr.register_list & (1U << r)) != 0) {
                        UOp sta;
                        sta.seq_num = seq_num;
                        sta.pc = pc;
                        sta.type = UOpType::STORE_ADDR;
                        sta.target_port = ExecutionPort::PORT_3_DUAL_AGU;
                        sta.opcode = Opcode::STR;
                        sta.arch_src1 = base_reg;
                        sta.imm = static_cast<uint32_t>(curr_offset);
                        sta.offset = curr_offset;
                        sta.is_imm_valid = true;
                        sta.mem_size_bytes = 4;

                        UOp std;
                        std.seq_num = seq_num;
                        std.pc = pc;
                        std.type = UOpType::STORE_DATA;
                        std.target_port = ExecutionPort::PORT_4_STORE_DATA;
                        std.opcode = Opcode::STR;
                        std.arch_src1 = r;
                        std.mem_size_bytes = 4;

                        uops.push_back(sta);
                        uops.push_back(std);
                        curr_offset += 4;
                    }
                }
                break;
            }
            case Opcode::POP:
            case Opcode::LDM: {
                uint8_t base_reg = (instr.op == Opcode::POP) ? 13 : instr.rn;
                int32_t curr_offset = 0;
                for (uint8_t r = 0; r < 16; ++r) {
                    if ((instr.register_list & (1U << r)) != 0) {
                        UOp lda;
                        lda.seq_num = seq_num;
                        lda.pc = pc;
                        lda.type = UOpType::LOAD;
                        lda.target_port = ExecutionPort::PORT_2_LOAD_AGU;
                        lda.opcode = Opcode::LDR;
                        lda.arch_dest = r;
                        lda.arch_src1 = base_reg;
                        lda.imm = static_cast<uint32_t>(curr_offset);
                        lda.offset = curr_offset;
                        lda.is_imm_valid = true;
                        lda.mem_size_bytes = 4;
                        uops.push_back(lda);
                        curr_offset += 4;
                    }
                }
                break;
            }

            // -------------------------------------------------------------
            // Branches, Calls, Returns
            // -------------------------------------------------------------
            case Opcode::B:
            case Opcode::BX:
            case Opcode::BL:
            case Opcode::BLX:
            case Opcode::CBZ:
            case Opcode::CBNZ: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.raw_inst = instr.raw_hex;
                uop.is_thumb32 = (instr.instr_size == 4);
                uop.is_branch = true;
                uop.target_port = ExecutionPort::PORT_0_ALU_BRANCH;
                uop.opcode = instr.op;

                if (instr.op == Opcode::BL || instr.op == Opcode::BLX) {
                    uop.type = UOpType::CALL;
                    uop.arch_dest = 14; // LR = return PC
                } else if (instr.op == Opcode::BX && (instr.rn == 14 || instr.rm == 14)) {
                    uop.type = UOpType::RET;
                    uop.arch_src1 = 14;
                } else {
                    uop.type = UOpType::BRANCH;
                    if (instr.op == Opcode::CBZ || instr.op == Opcode::CBNZ) {
                        uop.arch_src1 = instr.rn;
                    } else if (instr.op == Opcode::BX) {
                        uop.arch_src1 = (instr.rm != 0) ? instr.rm : instr.rn;
                    }
                }

                uop.offset = static_cast<int32_t>(instr.imm);
                uop.imm = instr.imm;
                uops.push_back(uop);
                break;
            }

            // -------------------------------------------------------------
            // Multiply Operations
            // -------------------------------------------------------------
            case Opcode::MUL:
            case Opcode::MLA: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.raw_inst = instr.raw_hex;
                uop.type = UOpType::MUL;
                uop.target_port = ExecutionPort::PORT_1_ALU_MUL;
                uop.opcode = instr.op;
                uop.arch_dest = instr.rd;
                uop.arch_src1 = instr.rn;
                uop.arch_src2 = instr.rm;
                uop.arch_src3 = instr.rs;
                uops.push_back(uop);
                break;
            }

            // -------------------------------------------------------------
            // System & NOP
            // -------------------------------------------------------------
            case Opcode::MRS:
            case Opcode::MSR: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.type = UOpType::SYS_REG;
                uop.target_port = ExecutionPort::PORT_0_ALU_BRANCH;
                uop.opcode = instr.op;
                uop.arch_dest = instr.rd;
                uop.arch_src1 = instr.rn;
                uops.push_back(uop);
                break;
            }
            case Opcode::SVC: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.type = UOpType::SVC;
                uop.target_port = ExecutionPort::PORT_0_ALU_BRANCH;
                uop.opcode = instr.op;
                uop.imm = instr.imm;
                uops.push_back(uop);
                break;
            }
            case Opcode::NOP: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.type = UOpType::NOP;
                uop.target_port = ExecutionPort::PORT_0_ALU_BRANCH;
                uop.opcode = instr.op;
                uops.push_back(uop);
                break;
            }

            // -------------------------------------------------------------
            // Standard ALU (ADD, SUB, MOV, CMP, AND, ORR, etc.)
            // -------------------------------------------------------------
            default: {
                UOp uop;
                uop.seq_num = seq_num;
                uop.pc = pc;
                uop.raw_inst = instr.raw_hex;
                uop.is_thumb32 = (instr.instr_size == 4);
                uop.type = UOpType::ALU;
                uop.target_port = ExecutionPort::PORT_0_ALU_BRANCH;
                uop.opcode = instr.op;
                uop.arch_dest = instr.rd;
                uop.arch_src1 = instr.rn;
                uop.arch_src2 = instr.is_imm ? UOp::INVALID_REG : instr.rm;
                uop.imm = instr.imm;
                uop.offset = static_cast<int32_t>(instr.imm);
                uop.is_imm_valid = instr.is_imm;
                uops.push_back(uop);
                break;
            }
        }

        return uops;
    }
};

} // namespace tinyarmsim::uarch
