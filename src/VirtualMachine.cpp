#include "VirtualMachine.hpp"

#include "RV32I.hpp"

#include <format>
#include <cassert>
#include <stdexcept>

bool VirtualMachine::TLBEntry::IsFlagsSet(uint32_t flags) const {
    assert(IsVirtual());
    
    if (IsMegaPage()) {
        return (vm.memory.Read32(table) & flags) == flags;
    } else {
        return (vm.memory.Read32(table_entry) & flags) == flags;
    }
}

void VirtualMachine::TLBEntry::SetFlags(uint32_t flags) {
    assert(IsVirtual());

    if (IsMegaPage()) {
        auto data = vm.memory.Read32(table);
        vm.memory.Write32(table, data | flags);
    } else {
        auto data = vm.memory.Read32(table_entry);
        vm.memory.Write32(table_entry, data | flags);
    }
}

void VirtualMachine::TLBEntry::ClearFlgs(uint32_t flags) {
    assert(IsVirtual());

    if (IsMegaPage()) {
        auto data = vm.memory.Read32(table);
        vm.memory.Write32(table, data & ~flags);
    } else {
        auto data = vm.memory.Read32(table_entry);
        vm.memory.Write32(table_entry, data & ~flags);
    }
}

bool VirtualMachine::TLBEntry::IsInPage(uint32_t phys_address) const {
    if (!IsVirtual()) return false;

    if (IsMegaPage()) {
        auto aligned_addr = phys_address & ~MEGA_PAGE_MASK;
        return physical_address == aligned_addr;
    } else {
        auto aligned_addr = phys_address & ~PAGE_MASK;
        return physical_address == aligned_addr;
    }
}

uint32_t VirtualMachine::TLBEntry::TranslateAddress(uint32_t phys_address) const {
    if (!IsVirtual()) return phys_address;

    if (IsMegaPage()) {
        return (physical_address & ~MEGA_PAGE_MASK) | (phys_address & MEGA_PAGE_MASK);
    } else {
        return (physical_address & ~PAGE_MASK) | (phys_address & PAGE_MASK);
    }
}

bool VirtualMachine::TLBEntry::CheckValid() {
    if (!IsFlagsSet(FLAG_VALID)) {
        // Raise exception
    }
}

bool VirtualMachine::TLBEntry::CheckExecution(bool as_user) {
    if (!CheckValid()) {
        return false;
    }

    if (!IsFlagsSet(FLAG_EXECUTE)) {
        // Raise exception
        return false;
    }

    if (as_user && !IsFlagsSet(FLAG_USER)) {
        // Raise exception
        return false;
    }
    else if (!as_user && IsFlagsSet(FLAG_USER)) {
        // Raise exception
        return false;
    }

    return true;
}

bool VirtualMachine::TLBEntry::CheckRead(bool as_user) {
    if (!CheckValid()) {
        return false;
    }
    
    if (!IsFlagsSet(FLAG_READ)) {
        // Raise exception
        return false;
    }

    if (as_user && !IsFlagsSet(FLAG_USER)) {
        // Raise exception
        return false;
    }

    if (!as_user && IsFlagsSet(FLAG_USER) && (vm.csrs[CSR_SSTATUS] & (1ULL<<18)) == 0) {
        // Raise exception
        return false;
    }

    return true;
}

bool VirtualMachine::TLBEntry::CheckWrite(bool as_user) {
    if (!CheckValid()) {
        return false;
    }

    if (as_user && !IsFlagsSet(FLAG_WRITE)) {
        // Raise exception
        return false;
    }

    if (!as_user && IsFlagsSet(FLAG_USER) && (vm.csrs[CSR_SSTATUS] & (1ULL<<18)) == 0) {
        // Raise exception
        return false;
    }

    return true;
}

VirtualMachine::VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second, uint32_t hart_id) : memory{memory}, pc{starting_pc}, instructions_per_second{instructions_per_second} {
    for (auto& r : regs) {
        r = 0;
    }

    for (auto& f : fregs) {
        f = 0.0;
    }

    for (auto& csr : csrs) {
        csr = 0;
    }

    csrs[CSR_MVENDORID] = 0;
    csrs[CSR_MARCHID] = ('V' << 24) | ('M' << 16) | ('A' << 8) | ('C');
    csrs[CSR_MIMPID] = ('H' << 24) | ('I' << 16) | ('N' << 8) | ('E');

    csrs[CSR_MHARTID] = hart_id;

    csrs[CSR_MISA] = ISA_32_BITS | ISA_F | ISA_I | ISA_M;
}

VirtualMachine::~VirtualMachine() {
    running = false;
}

void VirtualMachine::Step(uint32_t steps) {
    auto InvalidInstruction = [&]() {
        uint32_t instr = memory.Read32(pc);
        throw std::runtime_error(std::format("Invalid instruction at 0x{:08x}: 0x{:08x}", pc, instr));
    };

    auto SignExtend = [](uint32_t value, uint32_t bit) {
        uint32_t sign = -1U << bit;
        if (value & (1U << bit)) return value | sign;
        return value;
    };

    auto AsSigned = [](uint32_t value) {
        union S32U32 {
            uint32_t u;
            int32_t s;
        };

        S32U32 v;
        v.u = value;
        return v.s;
    };

    auto AsUnsigned = [](int32_t value) {
        union S32U32 {
            uint32_t u;
            int32_t s;
        };

        S32U32 v;
        v.s = value;
        return v.u;
    };
    
    for (uint32_t i = 0; i < steps && running; i++) {
        if (pc & 0b11)
            throw std::runtime_error(std::format("Invalid PC address {:08x}", pc));
        
        auto instr = RVInstruction::FromUInt32(memory.Read32(pc));

        switch (instr.opcode) {
            case RVInstruction::OP_LUI:
                regs[instr.rd] = instr.immediate;
                break;

            case RVInstruction::OP_AUIPC:
                regs[instr.rd] = pc + instr.immediate;
                break;
            
            case RVInstruction::OP_JAL:
                regs[instr.rd] = pc + 4;
                pc += SignExtend(instr.immediate, 20);
                break;
            
            case RVInstruction::OP_JALR:
                if (instr.func3 != RVInstruction::FUNC3_JALR) InvalidInstruction();
                
                regs[instr.rd] = pc + 4;
                pc = (regs[instr.rs1] + SignExtend(instr.immediate, 11)) & 0xfffffffe;
                break;
            
            case RVInstruction::OP_BRANCH:
                switch (instr.func3) {
                    case RVInstruction::FUNC3_BEQ:
                        if (regs[instr.rs1] == regs[instr.rs2])
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    case RVInstruction::FUNC3_BNE:
                        if (regs[instr.rs1] != regs[instr.rs2])
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    case RVInstruction::FUNC3_BLT:
                        if (AsSigned(regs[instr.rs1]) < AsSigned(regs[instr.rs2]))
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    case RVInstruction::FUNC3_BGE:
                        if (AsSigned(regs[instr.rs1]) > AsSigned(regs[instr.rs2]))
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    case RVInstruction::FUNC3_BLTU:
                        if (regs[instr.rs1] < regs[instr.rs2])
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    case RVInstruction::FUNC3_BGEU:
                        if (regs[instr.rs1] > regs[instr.rs2])
                            pc += SignExtend(instr.immediate, 12);
                        
                        else
                            pc += 4;
                        
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;
            
            case RVInstruction::OP_LOAD: {
                uint32_t address = regs[instr.rs1] + SignExtend(instr.immediate, 11);

                switch (instr.func3) {
                    case RVInstruction::FUNC3_LB:
                        regs[instr.rd] = SignExtend(memory.Read8(address), 7);
                        break;
                    
                    case RVInstruction::FUNC3_LH:
                        regs[instr.rd] = SignExtend(memory.Read16(address), 15);
                        break;
                    
                    case RVInstruction::FUNC3_LW:
                        regs[instr.rd] = memory.Read32(address);
                        break;
                    
                    case RVInstruction::FUNC3_LBU:
                        regs[instr.rd] = memory.Read8(address);
                        break;
                    
                    case RVInstruction::FUNC3_LHU:
                        regs[instr.rd] = memory.Read16(address);
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;
            }

            case RVInstruction::OP_STORE: {
                uint32_t address = regs[instr.rs1] + SignExtend(instr.immediate, 11);

                switch (instr.func3) {
                    case RVInstruction::FUNC3_SB:
                        memory.Write8(address, regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC3_SH:
                        memory.Write16(address, regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC3_SW:
                        memory.Write32(address, regs[instr.rs2]);
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;
            }

            case RVInstruction::OP_MATH_IMMEDIATE:
                switch (instr.func3) {
                    case RVInstruction::FUNC3_ADDI:
                        regs[instr.rd] = regs[instr.rs1] + SignExtend(instr.immediate, 11);
                        break;
                    
                    case RVInstruction::FUNC3_SLTI:
                        if (AsSigned(regs[instr.rs1]) < AsSigned(SignExtend(instr.immediate, 11)))
                            regs[instr.rd] = 1;

                        else
                            regs[instr.rd] = 0;
                        
                        break;
                    
                    case RVInstruction::FUNC3_SLTIU:
                        if (regs[instr.rs1] < SignExtend(instr.immediate, 11))
                            regs[instr.rd] = 1;
                        
                        else
                            regs[instr.rd] = 0;
                        
                        break;
                    
                    case RVInstruction::FUNC3_XORI:
                        regs[instr.rd] = regs[instr.rs1] ^ SignExtend(instr.immediate, 11);
                        break;
                    
                    case RVInstruction::FUNC3_ORI:
                        regs[instr.rd] = regs[instr.rs1] | SignExtend(instr.immediate, 11);
                        break;
                    
                    case RVInstruction::FUNC3_ANDI:
                        regs[instr.rd] = regs[instr.rs1] & SignExtend(instr.immediate, 11);
                        break;
                    
                    case RVInstruction::FUNC3_SLLI:
                        if (instr.func7 != RVInstruction::FUNC7_SLLI) InvalidInstruction();

                        regs[instr.rd] = regs[instr.rs1] << instr.rs2;
                        break;
                    
                    case RVInstruction::FUNC3_SHIFT_RIGHT_IMMEDIATE:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_SRLI:
                                regs[instr.rd] = regs[instr.rs1] >> instr.rs2;
                                break;
                            
                            case RVInstruction::FUNC7_SRAI: {
                                int32_t value = AsSigned(regs[instr.rs1]);
                                value >>= instr.rs2;
                                regs[instr.rd] = AsUnsigned(value);
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;
            
            case RVInstruction::OP_MATH:
                switch (instr.func3) {
                    case RVInstruction::FUNC3_ADD_SUB_MUL:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_ADD:
                                regs[instr.rd] = regs[instr.rs1] + regs[instr.rs2];
                                break;
                            
                            case RVInstruction::FUNC7_SUB:
                                regs[instr.rd] = regs[instr.rs1] - regs[instr.rs2];
                                break;
                            
                            case RVInstruction::FUNC7_MUL:
                                regs[instr.rd] = regs[instr.rs1] * regs[instr.rs2];
                                break;
                            
                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_SLL_MULH:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_SLL:
                                regs[instr.rd] = regs[instr.rs1] << (regs[instr.rs2] & 0x1f);
                                break;
                            
                            case RVInstruction::FUNC7_MULH: {
                                int32_t lhs = AsSigned(regs[instr.rs1]);
                                int32_t rhs = AsSigned(regs[instr.rs2]);
                                int32_t result = lhs * rhs;
                                regs[instr.rd] = AsUnsigned(result);
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_SLT_MULHSU:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_SLT:
                                if (AsSigned(regs[instr.rs1]) < AsSigned(regs[instr.rs2]))
                                    regs[instr.rd] = 1;
                                
                                else
                                    regs[instr.rd] = 0;
                                
                                break;
                            
                            case RVInstruction::FUNC7_MULHSU: {
                                int64_t lhs = AsSigned(regs[instr.rs1]);
                                uint64_t rhs = regs[instr.rs2];
                                int64_t result = lhs * rhs;
                                regs[instr.rd] = AsUnsigned(static_cast<int32_t>(result >> 32));
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_SLTU_MULHU:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_SLTU:
                                if (regs[instr.rs1] < regs[instr.rs2])
                                    regs[instr.rd] = 1;
                                
                                else
                                    regs[instr.rd] = 0;
                                
                                break;
                            
                            case RVInstruction::FUNC7_MULHU: {
                                uint64_t lhs = regs[instr.rs1];
                                uint64_t rhs = regs[instr.rs2];
                                uint64_t result = lhs * rhs;
                                regs[instr.rd] = static_cast<uint32_t>(result >> 32);
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_XOR_DIV:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_XOR:
                                regs[instr.rd] = regs[instr.rs1] ^ regs[instr.rs2];
                                break;
                            
                            case RVInstruction::FUNC7_DIV: {
                                int32_t lhs = AsSigned(regs[instr.rs1]);
                                int32_t rhs = AsSigned(regs[instr.rs2]);

                                if (rhs == 0)
                                    throw std::runtime_error("Div by zero is not handled yet");

                                int32_t result = lhs / rhs;
                                regs[instr.rd] = AsUnsigned(result);
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_SHIFT_RIGHT_DIVU:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_SRL:
                                regs[instr.rd] = regs[instr.rs1] >> (regs[instr.rs2] & 0x1f);
                                break;
                            
                            case RVInstruction::FUNC7_SRA: {
                                int32_t value = AsSigned(regs[instr.rs1]);
                                value >>= regs[instr.rs2] & 0x1f;
                                regs[instr.rd] = AsUnsigned(value);
                                break;
                            }

                            case RVInstruction::FUNC7_DIVU: {
                                uint32_t lhs = regs[instr.rs1];
                                uint32_t rhs = regs[instr.rs2];

                                if (rhs == 0)
                                    throw std::runtime_error("Div by zero is not handled yet");
                                
                                uint32_t result = lhs / rhs;
                                regs[instr.rd] = result;
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_OR_REMW:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_OR:
                                regs[instr.rd] = regs[instr.rs1] | regs[instr.rs2];
                                break;
                            
                            case RVInstruction::FUNC7_REMW: {
                                int32_t lhs = AsSigned(regs[instr.rs1]);
                                int32_t rhs = AsSigned(regs[instr.rs2]);

                                if (rhs == 0)
                                    throw std::runtime_error("Div by zero is not handled yet");

                                int32_t value = lhs % rhs;
                                regs[instr.rd] = AsUnsigned(value);
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    case RVInstruction::FUNC3_AND_REMU:
                        switch (instr.func7) {
                            case RVInstruction::FUNC7_AND:
                                regs[instr.rd] = regs[instr.rs1] & regs[instr.rs2];
                                break;
                            
                            case RVInstruction::FUNC7_REMU: {
                                uint32_t lhs = regs[instr.rs1];
                                uint32_t rhs = regs[instr.rs2];

                                if (rhs == 0)
                                    throw std::runtime_error("Div by zero is not handled yet");
                                
                                uint32_t value = lhs % rhs;
                                regs[instr.rd] = value;
                                break;
                            }

                            default:
                                InvalidInstruction();
                                break;
                        }
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;

            case RVInstruction::OP_FENCE:
                if (instr.func3 != RVInstruction::FUNC3_FENCE) InvalidInstruction();
                break;
            
            case RVInstruction::OP_SYSTEM:
                if ((instr.immediate == RVInstruction::IMM_MRET) && (instr.rs2 == RVInstruction::RS2_SRET_MRET) && (instr.rs1 == RVInstruction::RS1_SYSTEM) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("SRET");
                
                else if ((instr.immediate == RVInstruction::IMM_MRET) && (instr.rs2 == RVInstruction::RS2_SRET_MRET) && (instr.rs1 == RVInstruction::RS1_SYSTEM) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("MRET");
                
                else if ((instr.immediate == RVInstruction::IMM_WFI) && (instr.rs2 == RVInstruction::RS2_WFI) && (instr.rs1 == RVInstruction::RS1_SYSTEM) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("WFI");
                
                else if ((instr.func7 == RVInstruction::FUNC7_SFENCE_VMA) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("SFENCE.VMA");
                
                else if ((instr.func7 == RVInstruction::FUNC7_SINVAL_VMA) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("SINVAL.VMA");
                
                else if ((instr.func7 == RVInstruction::FUNC7_SFENCE_VMA) && (instr.rs2 == RVInstruction::RS2_SFENCE_W_INVAL) && (instr.rs1 == RVInstruction::RS1_SYSTEM) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("SFENCE.W.INVAL");
                
                else if ((instr.func7 == RVInstruction::FUNC7_SFENCE_VMA) && (instr.rs2 == RVInstruction::RS2_SFENCE_INVAL_IR) && (instr.rs1 == RVInstruction::RS1_SYSTEM) && (instr.rd == RVInstruction::RD_SYSTEM))
                    throw std::runtime_error("SFENCE.INVAL.IR");
                
                else if ((instr.immediate == RVInstruction::IMM_ECALL) && (instr.rd == 0) && (instr.rs1 == 0))
                    throw std::runtime_error("ECALL");
                
                else if ((instr.immediate == RVInstruction::IMM_EBREAK) && (instr.rd == 0) && (instr.rs1 == 0))
                    throw std::runtime_error("EBREAK");

                else
                    InvalidInstruction();
                
                break;

            default:
                InvalidInstruction();
                break;
        }

        switch (instr.opcode) {
            case RVInstruction::OP_JAL:
            case RVInstruction::OP_JALR:
            case RVInstruction::OP_BRANCH:
                break;
            
            default:
                pc += 4;
                break;
        }

        if (instr.rd == 0) regs[instr.rd] = 0;
    }

}

void VirtualMachine::GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<float, REGISTER_COUNT>& fregisters, uint32_t& pc) {
    registers = regs;
    fregisters = fregs;
    pc = this->pc;
}

VirtualMachine::TLBEntry VirtualMachine::GetTLBLookup(uint32_t phys_addr, bool bypass_cache) {
    uint32_t table_addr = csrs[CSR_SATP];

    if ((table_addr & (1ULL << 31)) == 0) return TLBEntry::CreateNonVirtual(*this);

    if (!bypass_cache) {
        for (size_t i = 0; i < tlb_cache.size(); i++) {
            const auto entry = tlb_cache[i];

            if (entry.IsInPage(phys_addr)) {
                tlb_cache.erase(tlb_cache.begin() + i);
                tlb_cache.push_back(entry);
                return entry;
            }
        }

        if (tlb_cache.size() == TLB_CACHE_SIZE) {
            tlb_cache.erase(tlb_cache.begin());
        }
    }

    table_addr = (table_addr & 0x3fffff) * 0x1000;

    auto table = memory.Read(table_addr, 1024);
    uint32_t table_index = (phys_addr >> 12) & 0x3ff;

    auto table_entry_address = table;

    TLBEntry entry = TLBEntry::CreateNonVirtual(*this);
}

size_t VirtualMachine::GetInstructionsPerSecond() {
    return instructions_per_second;
}