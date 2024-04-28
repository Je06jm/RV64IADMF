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
        return (vm.memory.ReadWord(table) & flags) == flags;
    } else {
        return (vm.memory.ReadWord(table_entry) & flags) == flags;
    }
}

void VirtualMachine::TLBEntry::SetFlags(uint32_t flags) {
    assert(IsVirtual());

    if (IsMegaPage()) {
        auto data = vm.memory.ReadWord(table);
        vm.memory.WriteWord(table, data | flags);
    } else {
        auto data = vm.memory.ReadWord(table_entry);
        vm.memory.WriteWord(table_entry, data | flags);
    }
}

void VirtualMachine::TLBEntry::ClearFlgs(uint32_t flags) {
    assert(IsVirtual());

    if (IsMegaPage()) {
        auto data = vm.memory.ReadWord(table);
        vm.memory.WriteWord(table, data & ~flags);
    } else {
        auto data = vm.memory.ReadWord(table_entry);
        vm.memory.WriteWord(table_entry, data & ~flags);
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
    return true;
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
        f.f = 0.0;
        f.is_double = false;
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

    csrs[CSR_MISA] = ISA_32_BITS | ISA_A | ISA_D | ISA_F | ISA_I | ISA_M;

    csrs[CSR_MSTATUS] = 0;
}

VirtualMachine::~VirtualMachine() {
    running = false;
}

bool VirtualMachine::Step(uint32_t steps) {
    auto InvalidInstruction = [&]() {
        uint32_t instr = memory.ReadWord(pc);
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
        Float f;
        f.f = v.f;
        f.is_double = false;
        return f;
    };

    auto ToDouble = [](uint64_t value) {
        union F64U64 {
            double d;
            uint64_t u;
        };

        F64U64 v;
        v.u = value;
        Float f;
        f.d = v.d;
        return f;
    };

    auto ToUInt32 = [](Float value) {
        union F32U32 {
            float f;
            uint32_t u;
        };

        F32U32 v;
        v.f = value.f;
        return v.u;
    };

    auto ToUInt64 = [](Float value) {
        union F64U64 {
            double d;
            uint64_t u;
        };

        F64U64 v;
        v.d = value.d;
        return v.u;
    };

    auto ClassF32 = [](Float value, bool* is_inf, bool* is_nan, bool* is_qnan, bool* is_subnormal, bool* is_zero, bool* is_neg) {
        // TODO This might not work as intended
        uint8_t sign = value.u32 >> 31;
        uint8_t exp = (value.u32 >> 23) & 0xff;
        uint32_t frac = value.u32 & 0x7fffff;
        
        if (is_inf) *is_inf = exp == 0xff && frac == 0;
        if (is_nan) *is_nan = exp == 0xff && frac != 0 && !(frac & 0x400000);
        if (is_qnan) *is_qnan = exp == 0xff && (frac & 0x400000);
        if (is_subnormal) *is_subnormal = exp == 0 && frac != 0;
        if (is_zero) *is_zero = exp == 0 && frac == 0;
        if (is_neg) *is_neg = sign != 0;
    };

    auto ClassF64 = [](Float value, bool* is_inf, bool* is_nan, bool* is_qnan, bool* is_subnormal, bool* is_zero, bool* is_neg) {
        // TODO This might not work as intended
        uint8_t sign = value.u64 >> 63;
        uint16_t exp = (value.u64 >> 52) & 0x7ff;
        uint64_t frac = value.u64 & 0xfffffffffffff;
        
        if (is_inf) *is_inf = exp == 0x7ff && frac == 0;
        if (is_nan) *is_nan = exp == 0x7ff && frac != 0 && !(frac & 0x8000000000000);
        if (is_qnan) *is_qnan = exp == 0x7ff && (frac & 0x8000000000000);
        if (is_subnormal) *is_subnormal = exp == 0 && frac != 0;
        if (is_zero) *is_zero = exp == 0 && frac == 0;
        if (is_neg) *is_neg = sign != 0;
    };

    auto SetFloatFlags = [&](bool invalid_op, bool div_by_zero, bool overflow, bool underflow, bool inexact) {
        if (invalid_op) csrs[CSR_FCSR] |= CSR_FCSR_NV;
        if (div_by_zero) csrs[CSR_FCSR] |= CSR_FCSR_DZ;
        if (overflow) csrs[CSR_FCSR] |= CSR_FCSR_OF;
        if (underflow) csrs[CSR_FCSR] |= CSR_FCSR_UF;
        if (inexact) csrs[CSR_FCSR] |= CSR_FCSR_NX;
    };

    ticks += steps;

    using Type = RVInstruction::Type;

    constexpr uint64_t RV_F32_NAN = 0xffffffff7fc00000;
    constexpr uint64_t RV_F32_QNAN = 0xffffffffffc00000;
    constexpr uint64_t RV_F64_NAN = 0x7ff0000000000000;
    constexpr uint64_t RV_F64_QNAN = 0xfff0000000000000;
    
    for (uint32_t i = 0; i < steps && running; i++) {
        if (pc & 0b11)
            throw std::runtime_error(std::format("Invalid PC address {:08x}", pc));
        
        auto instr = RVInstruction::FromUInt32(memory.ReadWord(pc));

        switch (instr.type) {
            case Type::LUI:
                regs[instr.rd] = instr.immediate;
                break;
            
            case Type::AUIPC:
                regs[instr.rd] = pc + instr.immediate;
                break;
            
            case Type::JAL: {
                uint32_t next_pc = pc + 4;
                pc += instr.immediate;
                regs[instr.rd] = next_pc;
                break;
            }
            
            case Type::JALR: {
                uint32_t next_pc = pc + 4;
                pc = (regs[instr.rs1] + instr.immediate) & 0xfffffffe;
                regs[instr.rd] = next_pc;
                break;
            }
            
            case Type::BEQ: {
                if (regs[instr.rs1] == regs[instr.rs2])
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::BNE: {
                if (regs[instr.rs1] != regs[instr.rs2])
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::BLT: {
                if (AsSigned(regs[instr.rs1]) < AsSigned(regs[instr.rs2]))
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::BGE: {
                if (AsSigned(regs[instr.rs1]) >= AsSigned(regs[instr.rs2]))
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::BLTU: {
                if (regs[instr.rs1] < regs[instr.rs2])
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::BGEU: {
                if (regs[instr.rs1] >= regs[instr.rs2])
                    pc += instr.immediate;
                
                else
                    pc += 4;
                
                break;
            }
            
            case Type::LB: 
                regs[instr.rd] = SignExtend(memory.ReadByte(regs[instr.rs1] + instr.immediate), 7);
                break;
            
            case Type::LH:
                regs[instr.rd] = SignExtend(memory.ReadHalf(regs[instr.rs1] + instr.immediate), 15);
                break;
            
            case Type::LW:
                regs[instr.rd] = memory.ReadWord(regs[instr.rs1] + instr.immediate);
                break;

            case Type::LBU:
                regs[instr.rd] = memory.ReadByte(regs[instr.rs1] + instr.immediate);
                break;
            
            case Type::LHU:
                regs[instr.rd] = memory.ReadHalf(regs[instr.rs1] + instr.immediate);
                break;
            
            case Type::SB:
                memory.WriteByte(regs[instr.rs1] + instr.immediate, regs[instr.rs2]);
                break;
            
            case Type::SH:
                memory.WriteHalf(regs[instr.rs1] + instr.immediate, regs[instr.rs2]);
                break;
            
            case Type::SW:
                memory.WriteWord(regs[instr.rs1] + instr.immediate, regs[instr.rs2]);
                break;
            
            case Type::ADDI:
                regs[instr.rd] = regs[instr.rs1] + instr.immediate;
                break;
            
            case Type::SLTI:
                regs[instr.rd] = AsSigned(regs[instr.rs1]) < AsSigned(instr.immediate) ? 1 : 0;
                break;
            
            case Type::SLTIU:
                regs[instr.rd] = regs[instr.rs1] < instr.immediate ? 1 : 0;
                break;
            
            case Type::XORI:
                regs[instr.rd] = regs[instr.rs1] ^ instr.immediate;
                break;
            
            case Type::ORI:
                regs[instr.rd] = regs[instr.rs1] | instr.immediate;
                break;
            
            case Type::ANDI:
                regs[instr.rd] = regs[instr.rs1] & instr.immediate;
                break;
            
            case Type::SLLI: {
                auto amount = instr.rs2;
                regs[instr.rd] = regs[instr.rs1] << amount;
                break;
            }
            
            case Type::SRLI: {
                auto amount = instr.rs2;
                regs[instr.rd] = regs[instr.rs1] >> amount;
                break;
            }
            
            case Type::SRAI: {
                auto amount = instr.rs2;
                auto value = regs[instr.rs1] >> amount;
                uint32_t sign = -1U << amount;

                if (regs[instr.rs1] & (1U << (32 - amount))) value |= sign;
                regs[instr.rd] = value;
                break;
            }
            
            case Type::ADD:
                regs[instr.rd] = regs[instr.rs1] + regs[instr.rs2];
                break;
            
            case Type::SUB:
                regs[instr.rd] = regs[instr.rs1] - regs[instr.rs2];
                break;
            
            case Type::SLL: {
                auto amount = regs[instr.rs2] & 0x1f;
                regs[instr.rd] = regs[instr.rs1] << amount;
                break;
            }
            
            case Type::SLT:
                regs[instr.rd] = AsSigned(regs[instr.rs1]) < AsSigned(regs[instr.rs2]) ? 1 : 0;
                break;
            
            case Type::SLTU:
                regs[instr.rd] = regs[instr.rs1] < regs[instr.rs2] ? 1 : 0;
                break;
            
            case Type::XOR:
                regs[instr.rd] = regs[instr.rs1] ^ regs[instr.rs2];
                break;
            
            case Type::SRL: {
                auto amount = regs[instr.rs2] & 0x1f;
                regs[instr.rd] = regs[instr.rs1] >> amount;
                break;
            }
            
            case Type::SRA: {
                auto amount = regs[instr.rs2] & 0x1f;
                auto value = regs[instr.rs1] >> amount;
                uint32_t sign = -1U << amount;

                if (regs[instr.rs1] & (1U << (32 - amount))) value |= sign;
                regs[instr.rd] = value;
                break;
            }
            
            case Type::OR:
                regs[instr.rd] = regs[instr.rs1] | regs[instr.rs2];
                break;
            
            case Type::AND:
                regs[instr.rd] = regs[instr.rs1] & regs[instr.rs2];
                break;
            
            case Type::FENCE:
                break;
            
            case Type::ECALL:
                if (!ecall_handlers.contains(regs[REG_A0]))
                    EmptyECallHandler(memory, regs, fregs);
                
                else
                    ecall_handlers[regs[REG_A0]](memory, regs, fregs);

                break;
            
            case Type::EBREAK:
                break;
            
            case Type::CSRRW: {
                auto value = regs[instr.rs1];

                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                WriteCSR(instr.immediate, value);
                break;
            }
            
            case Type::CSRRS: {
                auto value = regs[instr.rs1];

                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                if (instr.rs1 != REG_ZERO)
                    WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);
                
                break;
            }
            
            case Type::CSRRC: {
                auto value = regs[instr.rs1];
                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                if (instr.rs1 != REG_ZERO)
                    WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
                
                break;
            }
            
            case Type::CSRRWI: {
                auto value = instr.rs1;
                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                WriteCSR(instr.immediate, value);
                break;
            }
            
            case Type::CSRRSI: {
                auto value = instr.rs1;
                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);
                break;
            }
            
            case Type::CSRRCI: {
                auto value = instr.rs1;
                if (instr.rd != REG_ZERO)
                    regs[instr.rd] = ReadCSR(instr.immediate);
                
                WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
                break;
            }
            
            case Type::MUL: {
                auto lhs = AsSigned(regs[instr.rs1]);
                auto rhs = AsSigned(regs[instr.rs2]);
                regs[instr.rd] = AsUnsigned(lhs * rhs);
                break;
            }
            
            case Type::MULH: {
                int64_t lhs = AsSigned(regs[instr.rs1]);
                int64_t rhs = AsSigned(regs[instr.rs2]);
                int64_t result = lhs * rhs;
                regs[instr.rd] = AsUnsigned(static_cast<int32_t>(result >> 32));
                break;
            }
            
            case Type::MULHSU: {
                int64_t lhs = AsSigned(regs[instr.rs1]);
                uint64_t rhs = regs[instr.rs2];
                int64_t result = lhs * rhs;
                regs[instr.rd] = AsUnsigned(static_cast<int32_t>(result >> 32));
                break;
            }
            
            case Type::MULHU: {
                uint64_t lhs = regs[instr.rs1];
                uint64_t rhs = regs[instr.rs2];
                auto result = lhs * rhs;
                regs[instr.rd] = static_cast<uint32_t>(result >> 32);
                break;
            }

            case Type::DIV: {
                auto lhs = AsSigned(regs[instr.rs1]);
                auto rhs = AsSigned(regs[instr.rs2]);

                if (rhs == 0)
                    throw std::runtime_error("Div by zero is not handled yet");
                
                regs[instr.rd] = AsUnsigned(lhs / rhs);
                break;
            }
            
            case Type::DIVU: {
                auto lhs = regs[instr.rs1];
                auto rhs = regs[instr.rs2];

                if (rhs == 0)
                    throw std::runtime_error("Div by zero is not handled yet");
                
                regs[instr.rd] = lhs / rhs;
                break;
            }
            
            case Type::REM: {
                auto lhs = AsSigned(regs[instr.rs1]);
                auto rhs = AsSigned(regs[instr.rs2]);

                if (rhs == 0)
                    throw std::runtime_error("Div by zero is not handled yet");
                
                regs[instr.rd] = AsUnsigned(lhs % rhs);
                break;
            }
            
            case Type::REMU: {
                auto lhs = regs[instr.rs1];
                auto rhs = regs[instr.rs2];

                if (rhs == 0)
                    throw std::runtime_error("Div by zero is not handled yet");
                
                regs[instr.rd] = lhs % rhs;
                break;
            }
            
            case Type::LR_W:
                if (instr.rs2 != 0) InvalidInstruction();
                regs[instr.rd] = memory.ReadWordReserved(regs[instr.rs1], csrs[CSR_MHARTID]);
                break;
            
            case Type::SC_W:
                if (memory.WriteWordConditional(regs[instr.rs1], regs[instr.rs2], csrs[CSR_MHARTID]))
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = 1;
                
                break;
            
            case Type::AMOSWAP_W:
                regs[instr.rd] = memory.AtomicSwap(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOADD_W:
                regs[instr.rd] = memory.AtomicAdd(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOXOR_W:
                regs[instr.rd] = memory.AtomicXor(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOAND_W:
                regs[instr.rd] = memory.AtomicAnd(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOOR_W:
                regs[instr.rd] = memory.AtomicOr(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOMIN_W:
                regs[instr.rd] = memory.AtomicMin(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOMAX_W:
                regs[instr.rd] = memory.AtomicMax(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOMINU_W:
                regs[instr.rd] = memory.AtomicMinU(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::AMOMAXU_W:
                regs[instr.rd] = memory.AtomicMaxU(regs[instr.rs1], regs[instr.rs2]);
                break;
            
            case Type::FLW:
                fregs[instr.rd] = ToFloat(memory.ReadWord(regs[instr.rs1] + instr.immediate));
                break;
            
            case Type::FSW:
                memory.WriteWord(regs[instr.rs1] + instr.immediate, ToUInt32(fregs[instr.rs2]));
                break;
            
            case Type::FMADD_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f + fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FMSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f - fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FNMSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                float result = -(fregs[instr.rs1].f * fregs[instr.rs2].f) + fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FNMADD_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                float result = -(fregs[instr.rs1].f * fregs[instr.rs2].f) - fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FADD_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                float result = fregs[instr.rs1].f + fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                float result = fregs[instr.rs1].f - fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FMUL_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FDIV_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                float result = fregs[instr.rs1].f / fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FSQRT_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan, is_neg;
                ClassF32(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, &is_neg);

                if (is_inf || is_nan || is_qnan || is_neg)
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = sqrtf(fregs[instr.rs1].f);
                
                break;
            }
            
            case Type::FSGNJ_S: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u32 &= ~(1<<31);
                result.u32 |= rhs.u32 & (1<<31);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJN_S: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u32 &= ~(1<<31);
                result.u32 |= (~rhs.u32) & (1<<31);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJX_S: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u32 ^= rhs.u32 & (1<<31);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FMIN_S: {
                bool lhs_neg;
                bool rhs_neg;
                bool lhs_snan, lhs_qnan;
                bool rhs_snan, rhs_qnan;
                ClassF32(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, &lhs_neg);
                ClassF32(fregs[instr.rs2], nullptr, &rhs_snan, &rhs_qnan, nullptr, nullptr, &rhs_neg);
                bool lhs_nan = lhs_snan || lhs_qnan;
                bool rhs_nan = rhs_snan || rhs_qnan;

                if (lhs_nan && rhs_nan) {
                    SetFloatFlags(true, false, false, false, false);
                    fregs[instr.rd].u64 = RV_F32_NAN;
                    break;
                }

                bool lhs_less = false;
                if (lhs_nan) {
                    lhs_less = false;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (rhs_nan) {
                    lhs_less = true;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (lhs_neg && !rhs_neg) lhs_less = true;
                else if (!lhs_neg && rhs_neg) lhs_less = false;
                else if (fregs[instr.rs1].f < fregs[instr.rs2].f) lhs_less = true;

                if (lhs_less)
                    fregs[instr.rd] = fregs[instr.rs1];
                
                else
                    fregs[instr.rd] = fregs[instr.rs2];
                
                break;
            }
            
            case Type::FMAX_S: {
                bool lhs_neg;
                bool rhs_neg;
                bool lhs_snan, lhs_qnan;
                bool rhs_snan, rhs_qnan;
                ClassF32(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, &lhs_neg);
                ClassF32(fregs[instr.rs2], nullptr, &rhs_snan, &rhs_qnan, nullptr, nullptr, &rhs_neg);
                bool lhs_nan = lhs_snan || lhs_qnan;
                bool rhs_nan = rhs_snan || rhs_qnan;

                if (lhs_nan && rhs_nan) {
                    SetFloatFlags(true, false, false, false, false);
                    fregs[instr.rd].u64 = RV_F32_NAN;
                    break;
                }

                bool lhs_less = false;
                if (lhs_nan) {
                    lhs_less = false;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (rhs_nan) {
                    lhs_less = true;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (lhs_neg && !rhs_neg) lhs_less = true;
                else if (!lhs_neg && rhs_neg) lhs_less = false;
                else if (fregs[instr.rs1].f < fregs[instr.rs2].f) lhs_less = true;

                if (!lhs_less)
                    fregs[instr.rd] = fregs[instr.rs1];
                
                else
                    fregs[instr.rd] = fregs[instr.rs2];
                
                break;
            }
            
            case Type::FCVT_W_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan;
                ClassF32(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);
                
                uint32_t result;

                if (is_inf) {
                    if (fregs[instr.rs1].f < 0) result = -1U;
                    else result = 0x7fffffff;
                    SetFloatFlags(false, false, false, false, true);
                }
                else if (is_nan || is_qnan) {
                    result = 0x7fffffff;
                    SetFloatFlags(false, false, false, false, true);
                }
                else {
                    int32_t val = fregs[instr.rs1].f;
                    if (val != fregs[instr.rs1].f)
                        SetFloatFlags(false, false, false, false, true);

                    result = AsUnsigned(static_cast<int32_t>(val));
                }

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FCVT_WU_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan;
                ClassF32(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);

                uint32_t result;

                if (is_inf) {
                    if (fregs[instr.rs1].f < 0) result = 0;
                    else result = -1U;
                    SetFloatFlags(false, false, false, false, true);
                }
                else if (is_nan || is_qnan) {
                    result = -1U;
                    SetFloatFlags(false, false, false, false, true);
                }
                else {
                    uint32_t val = fregs[instr.rs1].f;
                    if (val != fregs[instr.rs1].f)
                        SetFloatFlags(false, false, false, false, true);
                    
                    result = val;
                }

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FMV_X_W:
                regs[instr.rd] = ToUInt32(fregs[instr.rs1]);
                break;
            
            case Type::FEQ_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF32(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF32(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);

                if (lhs_nan || rhs_nan)
                    SetFloatFlags(true, false, false, false, false);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan)
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = lhs.f == rhs.f ? 1 : 0;
                
                break;
            }
            
            case Type::FLT_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF32(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF32(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan) {
                    SetFloatFlags(true, false, false, false, false);
                    regs[instr.rd] = 0;
                }
                else
                    regs[instr.rd] = lhs.f < rhs.f ? 1 : 0;
                
                break;
            }
            
            case Type::FLE_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF32(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF32(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan) {
                    SetFloatFlags(true, false, false, false, false);
                    regs[instr.rd] = 0;
                }
                else
                    regs[instr.rd] = lhs.f <= rhs.f ? 1 : 0;
                
                break;
            }
            
            case Type::FCLASS_S: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan, is_subnormal, is_zero, is_neg;
                ClassF32(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, &is_subnormal, &is_zero, &is_neg);
                
                uint32_t result = 0;
                if (is_inf && is_neg) result |= 1 << 0;
                if (!is_subnormal && is_neg) result |= 1 << 1;
                if (is_subnormal && is_neg) result |= 1 << 2;
                if (is_zero && is_neg) result |= 1 << 3;
                if (is_zero && !is_neg) result |= 1 << 4;
                if (is_subnormal && !is_neg) result |= 1 << 5;
                if (!is_subnormal && !is_nan) result |= 1 << 6;
                if (is_inf && !is_neg) result |= 1 << 7;
                if (is_nan) result |= 1 << 8;
                if (is_qnan) result |= 1 << 9;

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FCVT_S_W: {
                auto val = AsSigned(regs[instr.rs1]);

                fregs[instr.rd].f = val;
                if (fregs[instr.rd].f != val)
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            }
            
            case Type::FCVT_S_WU:
                fregs[instr.rd].f = regs[instr.rs1];
                if (fregs[instr.rd].f != regs[instr.rs1])
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            
            case Type::FMV_W_X:
                fregs[instr.rd].u64 = 0;
                fregs[instr.rd].u32 = regs[instr.rs1];
                break;
            
            case Type::FLD: {
                auto addr = regs[instr.rs1] + instr.immediate;
                uint64_t val = memory.ReadWord(addr);
                val |= static_cast<uint64_t>(memory.ReadWord(addr + 4)) << 32;
                fregs[instr.rd] = ToDouble(val);
                break;
            }
            
            case Type::FSD: {
                auto addr = regs[instr.rs1] + instr.immediate;
                auto val = ToUInt64(fregs[instr.rs2]);
                memory.WriteWord(addr, static_cast<uint32_t>(val));
                memory.WriteWord(addr + 4, static_cast<uint32_t>(val >> 32));
                break;
            }
            
            case Type::FMADD_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d + fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FMSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d - fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FNMSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                double result = -(fregs[instr.rs1].d * fregs[instr.rs2].d) + fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FNMADD_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) InvalidInstruction();

                double result = -(fregs[instr.rs1].d * fregs[instr.rs2].d) - fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FADD_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                double result = fregs[instr.rs1].d + fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                double result = fregs[instr.rs1].d - fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FMUL_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FDIV_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                double result = fregs[instr.rs1].d / fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FSQRT_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan, is_neg;
                ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, &is_neg);

                if (is_inf || is_nan || is_qnan || is_neg)
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = sqrt(fregs[instr.rs1].d);
                
                break;
            }
            
            case Type::FSGNJ_D: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 &= ~(1U<<63);
                result.u64 |= rhs.u64 & (1U<<63);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJN_D: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 &= ~(1U<<63);
                result.u64 |= (~rhs.u64) & (1U<<63);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJX_D: {
                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 ^= rhs.u64 & (1U<<63);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FMIN_D: {
                bool lhs_neg;
                bool rhs_neg;
                bool lhs_snan, lhs_qnan;
                bool rhs_snan, rhs_qnan;
                ClassF64(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, &lhs_neg);
                ClassF64(fregs[instr.rs2], nullptr, &rhs_snan, &rhs_qnan, nullptr, nullptr, &rhs_neg);
                bool lhs_nan = lhs_snan || lhs_qnan;
                bool rhs_nan = rhs_snan || rhs_qnan;

                if (lhs_nan && rhs_nan) {
                    SetFloatFlags(true, false, false, false, false);
                    fregs[instr.rd].u64 = RV_F64_NAN;
                    break;
                }

                bool lhs_less = false;
                if (lhs_nan) {
                    lhs_less = false;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (rhs_nan) {
                    lhs_less = true;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (lhs_neg && !rhs_neg) lhs_less = true;
                else if (!lhs_neg && rhs_neg) lhs_less = false;
                else if (fregs[instr.rs1].d < fregs[instr.rs2].d) lhs_less = true;

                if (lhs_less)
                    fregs[instr.rd] = fregs[instr.rs1];
                
                else
                    fregs[instr.rd] = fregs[instr.rs2];
                
                break;
            }
            
            case Type::FMAX_D: {
                bool lhs_neg;
                bool rhs_neg;
                bool lhs_snan, lhs_qnan;
                bool rhs_snan, rhs_qnan;
                ClassF64(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, &lhs_neg);
                ClassF64(fregs[instr.rs2], nullptr, &rhs_snan, &rhs_qnan, nullptr, nullptr, &rhs_neg);
                bool lhs_nan = lhs_snan || lhs_qnan;
                bool rhs_nan = rhs_snan || rhs_qnan;

                if (lhs_nan && rhs_nan) {
                    SetFloatFlags(true, false, false, false, false);
                    fregs[instr.rd].u64 = RV_F64_NAN;
                    break;
                }

                bool lhs_less = false;
                if (lhs_nan) {
                    lhs_less = false;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (rhs_nan) {
                    lhs_less = true;
                    SetFloatFlags(true, false, false, false, false);
                }
                else if (lhs_neg && !rhs_neg) lhs_less = true;
                else if (!lhs_neg && rhs_neg) lhs_less = false;
                else if (fregs[instr.rs1].d < fregs[instr.rs2].d) lhs_less = true;

                if (!lhs_less)
                    fregs[instr.rd] = fregs[instr.rs1];
                
                else
                    fregs[instr.rd] = fregs[instr.rs2];
                
                break;
            }
            
            case Type::FCVT_S_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();
                
                bool lhs_snan, lhs_qnan;
                ClassF64(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, nullptr);

                if (lhs_snan) fregs[instr.rd].u64 = RV_F32_NAN;
                else if (lhs_qnan) fregs[instr.rd].u64 = RV_F32_QNAN;
                else {
                    auto val = fregs[instr.rs1].d;
                    fregs[instr.rd].u64 = 0;
                    fregs[instr.rd].f = val;
                }

                break;
            }
            
            case Type::FCVT_D_S: {
                bool lhs_snan, lhs_qnan;
                ClassF32(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, nullptr);

                if (lhs_snan) fregs[instr.rd].u64 = RV_F64_NAN;
                else if (lhs_qnan) fregs[instr.rd].u64 = RV_F64_QNAN;
                else fregs[instr.rd].d = fregs[instr.rs1].f;

                break;
            }
            
            case Type::FEQ_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF64(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF64(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);

                if (lhs_nan || rhs_nan)
                    SetFloatFlags(true, false, false, false, false);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan)
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = lhs.d == rhs.d ? 1 : 0;
                
                break;
            }
            
            case Type::FLT_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF64(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF64(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan) {
                    SetFloatFlags(true, false, false, false, false);
                    regs[instr.rd] = 0;
                }
                else
                    regs[instr.rd] = lhs.d < rhs.d ? 1 : 0;
                
                break;
            }
            
            case Type::FLE_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                auto lhs = fregs[instr.rs1];
                auto rhs = fregs[instr.rs2];

                bool lhs_nan, lhs_qnan;
                bool rhs_nan, rhs_qnan;
                ClassF64(lhs, nullptr, &lhs_nan, &lhs_qnan, nullptr, nullptr, nullptr);
                ClassF64(rhs, nullptr, &rhs_nan, &rhs_qnan, nullptr, nullptr, nullptr);
                
                if (lhs_nan || rhs_nan || lhs_qnan || rhs_qnan) {
                    SetFloatFlags(true, false, false, false, false);
                    regs[instr.rd] = 0;
                }
                else
                    regs[instr.rd] = lhs.d <= rhs.d ? 1 : 0;
                
                break;
            }
            
            case Type::FCLASS_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan, is_subnormal, is_zero, is_neg;
                ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, &is_subnormal, &is_zero, &is_neg);
                
                uint32_t result = 0;
                if (is_inf && is_neg) result |= 1 << 0;
                if (!is_subnormal && is_neg) result |= 1 << 1;
                if (is_subnormal && is_neg) result |= 1 << 2;
                if (is_zero && is_neg) result |= 1 << 3;
                if (is_zero && !is_neg) result |= 1 << 4;
                if (is_subnormal && !is_neg) result |= 1 << 5;
                if (!is_subnormal && !is_nan) result |= 1 << 6;
                if (is_inf && !is_neg) result |= 1 << 7;
                if (is_nan) result |= 1 << 8;
                if (is_qnan) result |= 1 << 9;

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FCVT_W_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan;
                ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);
                
                uint32_t result;

                if (is_inf) {
                    if (fregs[instr.rs1].d < 0) result = -1U;
                    else result = 0x7fffffff;
                    SetFloatFlags(false, false, false, false, true);
                }
                else if (is_nan || is_qnan) {
                    result = 0x7fffffff;
                    SetFloatFlags(false, false, false, false, true);
                }
                else {
                    int32_t val = fregs[instr.rs1].d;
                    if (val != fregs[instr.rs1].d)
                        SetFloatFlags(false, false, false, false, true);

                    result = AsUnsigned(static_cast<int32_t>(val));
                }

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FCVT_WU_D: {
                if (!ChangeRoundingMode(instr.rm)) InvalidInstruction();

                bool is_inf, is_nan, is_qnan;
                ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);

                uint32_t result;

                if (is_inf) {
                    if (fregs[instr.rs1].d < 0) result = 0;
                    else result = -1U;
                    SetFloatFlags(false, false, false, false, true);
                }
                else if (is_nan || is_qnan) {
                    result = -1U;
                    SetFloatFlags(false, false, false, false, true);
                }
                else {
                    uint32_t val = fregs[instr.rs1].d;
                    if (val != fregs[instr.rs1].d)
                        SetFloatFlags(false, false, false, false, true);
                    
                    result = val;
                }

                regs[instr.rd] = result;
                break;
            }
            
            case Type::FCVT_D_W: {
                auto val = AsSigned(regs[instr.rs1]);

                fregs[instr.rd].d = val;
                if (fregs[instr.rd].d != val)
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            }
            
            case Type::FCVT_D_WU:
                fregs[instr.rd].d = regs[instr.rs1];
                if (fregs[instr.rd].d != regs[instr.rs1])
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            
            case Type::URET:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SRET:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::MRET:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::WFI:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SFENCE_VMA:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SINVAL_VMA:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SINVAL_GVMA:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SFENCE_W_INVAL:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;
            
            case Type::SFENCE_INVAL_IR:
                throw std::runtime_error(std::format("Instruction not implemented {}", std::string(instr)));
                break;

            case Type::INVALID:
            default:
                InvalidInstruction();
                break;
        }

        switch (instr.type) {
            case Type::JAL:
            case Type::JALR:
            case Type::BEQ:
            case Type::BGE:
            case Type::BGEU:
            case Type::BLT:
            case Type::BLTU:
            case Type::BNE:
                break;
            
            default:
                pc += 4;
        }
        
        if (instr.rd == 0) regs[instr.rd] = 0;

        if (IsBreakPoint(pc)) return true;
    }

    return false;
}

void VirtualMachine::GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, uint32_t& pc) {
    registers = regs;
    fregisters = fregs;
    pc = this->pc;
}

void VirtualMachine::GetCSRSnapshot(std::unordered_map<uint32_t, uint32_t>& csrs) const {
    csrs = this->csrs;
}

VirtualMachine::TLBEntry VirtualMachine::GetTLBLookup(uint32_t phys_addr, bool bypass_cache) {
    
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

bool VirtualMachine::IsBreakPoint(uint32_t addr) {
    if (break_points.contains(addr)) return true;

    auto word = memory.PeekWord(addr);

    if (!word.second)
        return false;

    RVInstruction instr = RVInstruction::FromUInt32(word.first);

    return instr.type == RVInstruction::Type::EBREAK;
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

void VirtualMachine::EmptyECallHandler(Memory&, std::array<uint32_t, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&) {
    throw std::runtime_error(std::format("Unknown ECall handler: {}", regs[REG_A0]));
}

std::unordered_map<uint32_t, VirtualMachine::ECallHandler> VirtualMachine::ecall_handlers;