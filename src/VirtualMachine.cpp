#include "VirtualMachine.hpp"

#include "RV32I.hpp"
#include "DeltaTime.hpp"

#include <format>
#include <cassert>
#include <stdexcept>
#include <cmath>
#include <fenv.h>

const int VirtualMachine::default_rounding_mode = fegetround();

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

bool VirtualMachine::CSRPrivilegeCheck(uint32_t csr) {
    return true;
}

uint32_t VirtualMachine::ReadCSR(uint32_t csr, bool is_internal_read) {
    if (!is_internal_read && !CSRPrivilegeCheck(csr))
        throw std::runtime_error("CSR Read Privilege");
    
    if (!csrs.contains(csr))
        throw std::runtime_error("Read Invalid CSR");
    
    return csrs[csr];
}

void VirtualMachine::WriteCSR(uint32_t csr, uint32_t value) {
    if (!CSRPrivilegeCheck(csr))
        throw std::runtime_error("CSR Write Privilege");
    
    if (!csrs.contains(csr))
        throw std::runtime_error("Write Invalid CSR");
    
    csrs[csr] = value;
}

bool VirtualMachine::ChangeRoundingMode(uint8_t rm) {
    switch (rm) {
        case RVInstruction::RM_ROUND_TO_NEAREST_TIES_EVEN:
            fesetround(FE_TONEAREST);
            break;
        
        case RVInstruction::RM_ROUND_TO_ZERO:
            fesetround(FE_TOWARDZERO);
            break;
        
        case RVInstruction::RM_ROUND_DOWN:
            fesetround(FE_DOWNWARD);
            break;
        
        case RVInstruction::RM_ROUND_UP:
            fesetround(FE_UPWARD);
            break;
        
        case RVInstruction::RM_ROUND_TO_NEAREST_TIES_MAX_MAGNITUDE:
        case RVInstruction::RM_INVALID0:
        case RVInstruction::RM_INVALID1:
            return false;
        
        case RVInstruction::RM_DYNAMIC:
            return ChangeRoundingMode((csrs[CSR_FCSR] >> 5) & 0b111);
        
        default:
            fesetround(default_rounding_mode);
            break;
    }

    return true;
}

bool VirtualMachine::CheckFloatErrors() {
    fexcept_t except;
    fegetexceptflag(&except, FE_ALL_EXCEPT);

    csrs[CSR_FCSR] &= ~CSR_FCSR_FLAGS;

    if (except & FE_DIVBYZERO) csrs[CSR_FCSR] |= CSR_FCSR_DZ;
    if (except & FE_INEXACT) csrs[CSR_FCSR] |= CSR_FCSR_NX;
    if (except & FE_INVALID) csrs[CSR_FCSR] |= CSR_FCSR_NV;
    if (except & FE_OVERFLOW) csrs[CSR_FCSR] |= CSR_FCSR_OF;
    if (except & FE_UNDERFLOW) csrs[CSR_FCSR] |= CSR_FCSR_UF;

    if (except & (FE_DIVBYZERO | FE_INVALID)) return true;
    return false;
}

VirtualMachine::VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second, uint32_t hart_id) : memory{memory}, pc{starting_pc}, instructions_per_second{instructions_per_second} {
    for (auto& r : regs) {
        r = 0;
    }

    for (auto& f : fregs) {
        f = 0.0;
    }

    // User

    csrs[CSR_FRM] = 0;
    csrs[CSR_CYCLE] = 0;
    csrs[CSR_TIME] = 0;
    csrs[CSR_INSTRET] = 0;
    csrs[CSR_CYCLEH] = 0;
    csrs[CSR_TIMEH] = 0;
    csrs[CSR_INSTRETH] = 0;

    // Supervisor

    csrs[CSR_SSTATUS] = 0;
    csrs[CSR_SIE] = 0;
    csrs[CSR_STVEC] = 0;
    csrs[CSR_SCOUNTEREN] = 0;
    csrs[CSR_SENVCFG] = 0;
    csrs[CSR_SSCRATCH] = 0;
    csrs[CSR_SEPC] = 0;
    csrs[CSR_SCAUSE] = 0;
    csrs[CSR_STVAL] = 0;
    csrs[CSR_SIP] = 0;
    csrs[CSR_SATP] = 0;
    csrs[CSR_SCONTEXT] = 0;

    // Machine

    csrs[CSR_MVENDORID] = 0;

    csrs[CSR_MARCHID] = ('E' << 24) | ('N' << 16) | ('I' << 8) | ('H');
    csrs[CSR_MIMPID] = ('C' << 24) | ('A' << 16) | ('M' << 8) | ('V');

    csrs[CSR_MHARTID] = hart_id;

    csrs[CSR_MISA] = ISA_32_BITS | ISA_A | ISA_I | ISA_M;

    csrs[CSR_MSTATUS] = 0;
}

VirtualMachine::~VirtualMachine() {
    running = false;
}

bool VirtualMachine::Step(uint32_t steps) {
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

    auto ToFloat = [](uint32_t value) {
        union F32U32 {
            float f;
            uint32_t u;
        };

        F32U32 v;
        v.u = value;
        return v.f;
    };

    auto FromFloat = [](float value) {
        union F32U32 {
            float f;
            uint32_t u;
        };

        F32U32 v;
        v.f = value;
        return v.u;
    };

    ticks += steps;
    
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
            
            case RVInstruction::OP_JAL: {
                uint32_t next_pc = pc + 4;
                pc += SignExtend(instr.immediate, 20);
                regs[instr.rd] = next_pc;
                break;
            }
            
            case RVInstruction::OP_JALR: {
                if (instr.func3 != RVInstruction::FUNC3_JALR) InvalidInstruction();

                uint32_t next_pc = pc + 4; 
                pc = (regs[instr.rs1] + SignExtend(instr.immediate, 11)) & 0xfffffffe;
                regs[instr.rd] = next_pc;
                break;
            }
            
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
                switch (instr.func3) {
                    case RVInstruction::FUNC3_SYSTEM:
                        if (instr.rd != RVInstruction::RD_SYSTEM) InvalidInstruction();

                        switch (instr.immediate) {
                            case RVInstruction::IMM_ECALL:
                                if (regs[REG_A0] >= ecall_handlers.size())
                                    EmptyECallHandler(memory, regs, fregs);
                                
                                else
                                    ecall_handlers[regs[REG_A0]](memory, regs, fregs);
                                
                                break;
                            
                            case RVInstruction::IMM_EBREAK:
                                throw std::runtime_error("EBREAK");
                                break;
                            
                            default:
                                switch (instr.immediate) {
                                    case RVInstruction::IMM_URET:
                                        throw std::runtime_error("URET");
                                        break;
                                    
                                    case RVInstruction::IMM_SRET:
                                        throw std::runtime_error("SRET");
                                        break;
                                    
                                    case RVInstruction::IMM_MRET:
                                        throw std::runtime_error("MRET");
                                        break;

                                    case RVInstruction::IMM_WFI:
                                        throw std::runtime_error("WFI");
                                        break;
                                    
                                    default:
                                        InvalidInstruction();
                                        break;
                                }
                                break;
                            }
                            break;
                    
                    case RVInstruction::FUNC3_CSRRW: {
                        uint32_t value = regs[instr.rs1];
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        WriteCSR(instr.immediate, value);
                        break;
                    }
                    
                    case RVInstruction::FUNC3_CSRRS: {
                        uint32_t value = regs[instr.rs1];
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        if (instr.rs1 != REG_ZERO)
                            WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);

                        break;
                    }
                    
                    case RVInstruction::FUNC3_CSRRC: {
                        uint32_t value = regs[instr.rs1];
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        if (instr.rs1 != REG_ZERO)
                            WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
                        
                        break;
                    }
                    
                    case RVInstruction::FUNC3_CSRRWI: {
                        uint32_t value = instr.rs1;
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        WriteCSR(instr.immediate, value);
                        break;
                    }
                    
                    case RVInstruction::FUNC3_CSRRSI: {
                        uint32_t value = instr.rs1;
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);
                        break;
                    }
                    
                    case RVInstruction::FUNC3_CSRRCI: {
                        uint32_t value = instr.rs1;
                        if (instr.rd != REG_ZERO)
                            regs[instr.rd] = ReadCSR(instr.immediate);
                        
                        WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
                        break;
                    }
                    
                    default:
                        InvalidInstruction();
                        break;

                }
                break;

            case RVInstruction::OP_ATOMIC:
                if (instr.func3 != RVInstruction::FUNC3_ATOMIC) InvalidInstruction();

                switch (instr.func7 & RVInstruction::FUNC7_ATOMIC_MASK) {
                    case RVInstruction::FUNC7_LR_W:
                        if (instr.rs2 != 0) InvalidInstruction();
                        regs[instr.rd] = memory.Read32Reserved(regs[instr.rs1], csrs[CSR_MHARTID]);
                        break;
                    
                    case RVInstruction::FUNC7_SC_W:
                        if (memory.Write32Conditional(regs[instr.rs1], regs[instr.rs2], csrs[CSR_MHARTID])) {
                            regs[instr.rd] = 0;
                        } else {
                            regs[instr.rd] = 1;
                        }
                        break;
                    
                    case RVInstruction::FUNC7_AMOSWAP_W:
                        regs[instr.rd] = memory.AtomicSwap(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC7_AMOADD_W:
                        regs[instr.rd] = memory.AtomicAdd(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC7_AMOAND_W:
                        regs[instr.rd] = memory.AtomicAnd(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC7_AMOOR_W:
                        regs[instr.rd] = memory.AtomicOr(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC7_AMOMAX_W: {
                        int32_t result = AsSigned(regs[instr.rs2]);
                        result = memory.AtomicMax(regs[instr.rs1], result);
                        regs[instr.rd] = AsUnsigned(result);
                        break;
                    }
                    
                    case RVInstruction::FUNC7_AMOMAXU_W:
                        regs[instr.rd] = memory.AtomicMaxU(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    case RVInstruction::FUNC7_AMOMIN_W: {
                        int32_t result = AsSigned(regs[instr.rs2]);
                        result = memory.AtomicMin(regs[instr.rs1], result);
                        regs[instr.rd] = AsUnsigned(result);
                        break;
                    }
                    
                    case RVInstruction::FUNC7_AMOMINU_W:
                        regs[instr.rd] = memory.AtomicMinU(regs[instr.rs1], regs[instr.rs2]);
                        break;
                    
                    default:
                        InvalidInstruction();
                        break;
                }
                break;

            case RVInstruction::OP_FLW:
                if (instr.func3 != RVInstruction::FUNC3_FLW) InvalidInstruction();

                fregs[instr.rd] = ToFloat(memory.Read32(regs[instr.rs1] + SignExtend(instr.immediate, 11)));
                break;
            
            case RVInstruction::OP_FSW:
                if (instr.func3 != RVInstruction::FUNC3_FSW) InvalidInstruction();

                memory.Write32(regs[instr.rs1] + SignExtend(instr.immediate, 11), FromFloat(fregs[instr.rs2]));
                break;
            
            case RVInstruction::OP_FMADD_S: {
                if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();
                
                if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                float result = fregs[instr.rs1] * fregs[instr.rs2] + fregs[instr.rs3];
                ChangeRoundingMode();
                
                if (CheckFloatErrors())
                    result = NAN;

                fregs[instr.rd] = result;
                break;
            }

            case RVInstruction::OP_FMSUB_S: {
                if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                float result = fregs[instr.rs1] * fregs[instr.rs2] - fregs[instr.rs3];
                ChangeRoundingMode();

                if (CheckFloatErrors())
                    result = NAN;
                
                fregs[instr.rd] = result;
                break;
            }

            case RVInstruction::OP_FNMSUB_S: {
                if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                float result = -(fregs[instr.rs1] * fregs[instr.rs2]) + fregs[instr.rs3];
                ChangeRoundingMode();

                if (CheckFloatErrors())
                    result = NAN;
                
                fregs[instr.rd] = result;
                break;
            }

            case RVInstruction::OP_FNMADD_S: {
                if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                float result = -(fregs[instr.rs1] * fregs[instr.rs2]) - fregs[instr.rs3];
                ChangeRoundingMode();

                if (CheckFloatErrors())
                    result = NAN;

                fregs[instr.rd] = result;
                break;
            }

            case RVInstruction::OP_FLOAT:
                switch (instr.func7) {
                    case RVInstruction::FUNC7_FADD_S: {
                        if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                        if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                        float result = fregs[instr.rs1] + fregs[instr.rs2];
                        ChangeRoundingMode();

                        if (CheckFloatErrors())
                            result = NAN;
                        
                        fregs[instr.rd] = result;
                        break;
                    }

                    case RVInstruction::FUNC7_FSUB_S: {
                        if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                        if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                        float result = fregs[instr.rs1] - fregs[instr.rs2];
                        ChangeRoundingMode();

                        if (CheckFloatErrors())
                            result = NAN;
                        
                        fregs[instr.rd] = result;
                        break;
                    }

                    case RVInstruction::FUNC7_FMUL_S: {
                        if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                        if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                        float result = fregs[instr.rs1] * fregs[instr.rs2];
                        ChangeRoundingMode();

                        if (CheckFloatErrors())
                            result = NAN;
                        
                        fregs[instr.rd] = result;
                        break;
                    }

                    case RVInstruction::FUNC7_FDIV_S: {
                        if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                        if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                        float result = fregs[instr.rs1] / fregs[instr.rs2];
                        ChangeRoundingMode();

                        if (CheckFloatErrors())
                            result = NAN;
                        
                        fregs[instr.rd] = result;
                        break;
                    }

                    case RVInstruction::FUNC7_FSQRT_S: {
                        if (instr.fmt != RVInstruction::FMT_S || instr.rs2 != 0) InvalidInstruction();

                        if (!ChangeRoundingMode(instr.func3)) InvalidInstruction();
                        float result = sqrtf(fregs[instr.rs1]);
                        ChangeRoundingMode();

                        if (CheckFloatErrors())
                            result = NAN;
                        
                        fregs[instr.rd] = result;
                        break;
                    }

                    case RVInstruction::FUNC7_FMIN_FMAX: {
                        if (instr.fmt != RVInstruction::FMT_S) InvalidInstruction();

                        float lhs = fregs[instr.rs1];
                        float rhs = fregs[instr.rs2];

                        float result = 0.0f;

                        switch (instr.func3) {
                            case RVInstruction::FUNC3_FMIN_S:
                                result = lhs < rhs ? lhs : rhs;
                                break;
                            
                            case RVInstruction::FUNC3_FMAX_S:
                                result = lhs > rhs ? lhs : rhs;
                                break;
                            
                            default:
                                InvalidInstruction();
                        }

                        break;
                    }
                }

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

        if (IsBreakPoint(pc)) return true;
    }

    return false;
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
    double total_time = 0.0;
    uint32_t total_ticks = 0;

    for (size_t i = 0; i < history_delta.size(); i++) {
        total_time += history_delta[i];
        total_ticks += history_tick[i];
    }

    return total_ticks / total_time;
}

void VirtualMachine::UpdateTime() {
    history_delta.push_back(delta_time());
    history_tick.push_back(ticks);
    ticks = 0;

    while (history_delta.size() > MAX_HISTORY) {
        history_delta.erase(history_delta.begin());
        history_tick.erase(history_tick.begin());
    }
}

void VirtualMachine::EmptyECallHandler(Memory& memory, std::array<uint32_t, REGISTER_COUNT>& regs, std::array<float, REGISTER_COUNT>&) {
    throw std::runtime_error(std::format("Unknown ECall handler: {}", regs[REG_A0]));
}

std::vector<VirtualMachine::ECallHandler> VirtualMachine::ecall_handlers;