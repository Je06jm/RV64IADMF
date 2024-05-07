#include "VirtualMachine.hpp"

#include "RV32I.hpp"
#include "DeltaTime.hpp"

#include <format>
#include <cassert>
#include <stdexcept>
#include <cmath>
#include <chrono>
#include <fenv.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sched.h>
#endif

const int VirtualMachine::default_rounding_mode = fegetround();

bool VirtualMachine::CSRPrivilegeCheck(uint32_t csr) {
    if (csr < 4 || (csr >= 0xc00 && csr < 0xcf0))
        return true;
    
    if ((csr >= 0x100 && csr < 0x144) || csr == 0x180)
        return privilege_level != PrivilegeLevel::User;
    
    return privilege_level == PrivilegeLevel::Machine;
}

uint32_t VirtualMachine::ReadCSR(uint32_t csr, bool is_internal_read) {
    if (!is_internal_read && !CSRPrivilegeCheck(csr)) {
        RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
        return 0;
    }
    
    if (csr >= CSR_MHPMEVENT3 && csr < (CSR_MHPMEVENT3 + CSR_PERFORMANCE_EVENT_MAX - 3))
        return 0;
    
    if (csr >= CSR_MHPMCOUNTER3 && csr < (CSR_MHPMCOUNTER3 + CSR_PERF_COUNTER_MAX - 3))
        return 0;
    
    if (csr >= CSR_MHPMCOUNTER3H && csr < (CSR_MHPMCOUNTER3H + CSR_PERF_COUNTER_MAX - 3))
        return 0;

    switch (csr) {
        case CSR_MCYCLE:
        case CSR_CYCLE:
            return static_cast<uint32_t>(cycles);
        
        case CSR_MCYCLEH:
        case CSR_CYCLEH:
            return static_cast<uint32_t>(cycles >> 32);
        
        case CSR_TIME:
            return static_cast<uint32_t>(csr_mapped_memory->time);
        
        case CSR_TIMEH:
            return static_cast<uint32_t>(csr_mapped_memory->time >> 32);

        case CSR_MSTATUS:
            mstatus.SD = mstatus.FS == FS_DIRTY;
            return mstatus.raw;

        case CSR_SSTATUS:
            sstatus.SD = sstatus.FS == FS_DIRTY;
            return sstatus.raw;
        
        case CSR_MIP:
            return mip;
        
        case CSR_MIE:
            return mie;
        
        case CSR_MIDELEG:
            return mideleg;
        
        case CSR_SIP:
            return sip;
        
        case CSR_SIE:
            return sie;

        case CSR_MCOUNTEREN:
        case CSR_MCOUNTINHIBIT:
            return 0;
        
        case CSR_MENVCFG:
        case CSR_MENVCFGH:
            return 0;
        
        case CSR_SENVCFG:
            return 0;
        
        case CSR_SATP:
            return satp.raw;

        default:
            if (!csrs.contains(csr)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                return 0;
            }

            return csrs[csr];
    }

    return csrs[csr];
}

void VirtualMachine::WriteCSR(uint32_t csr, uint32_t value) {
    if (!CSRPrivilegeCheck(csr)) {
        RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
        return;
    }
    
    switch (csr) {
        case CSR_MVENDORID:
        case CSR_MARCHID:
        case CSR_MIMPID:
        case CSR_MHARTID:
        case CSR_MISA:
        case CSR_MINSTRET:
        case CSR_MINSTRETH:
        case CSR_CYCLE:
        case CSR_CYCLEH:
        case CSR_TIME:
        case CSR_TIMEH:
        case CSR_MSTATUSH:
        case CSR_MCOUNTEREN:
        case CSR_MCOUNTINHIBIT:
        case CSR_MENVCFG:
        case CSR_MENVCFGH:
        case CSR_SENVCFG:
            return; // Non writable
        
        case CSR_MSTATUS: {
            auto last = mstatus.MPP;
            mstatus.raw &= ~MSTATUS_WRITABLE_BITS;
            value &= MSTATUS_WRITABLE_BITS;
            mstatus.raw |= value;

            if (mstatus.MPP == 0b10)
                mstatus.MPP = last;
            
            break;
        }

        case CSR_SSTATUS: {
            sstatus.raw &= ~SSTATUS_WRITABLE_BITS;
            value &= SSTATUS_WRITABLE_BITS;
            sstatus.raw |= value;
            break;
        }

        case CSR_MIP:
            mip = value & VALID_INTERRUPT_BITS;
            break;
        
        case CSR_MIE:
            mie = value & VALID_INTERRUPT_BITS;
            break;
        
        case CSR_MIDELEG:
            mideleg = value & VALID_INTERRUPT_BITS;
            break;
        
        case CSR_SIP:
            sip = value & VALID_INTERRUPT_BITS;
            break;
        
        case CSR_SIE:
            sie = value & VALID_INTERRUPT_BITS;
            break;

        case CSR_SATP:
            satp.raw = value;
            break;

        default:
            if (!csrs.contains(csr)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                return;
            }

            csrs[csr] = value;
            break;
    }
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

void VirtualMachine::RaiseInterrupt(uint32_t cause) {
    auto cause_bit = 1ULL << cause;

    static std::mutex lock;
    lock.lock();
    csrs[CSR_MIP] |= cause_bit;
    lock.unlock();
}

void VirtualMachine::RaiseException(uint32_t cause) {
    auto cause_bit = 1ULL << cause;

    auto delegate = csrs[CSR_MEDELEG];

    if (delegate & cause_bit)
        RaiseSupervisorTrap(cause);
    
    else
        RaiseMachineTrap(cause);
    
}

void VirtualMachine::RaiseMachineTrap(uint32_t cause) {
    auto handler_address = csrs[CSR_MTVEC];

    auto mode = handler_address & 0b11;
    handler_address &= ~0b11;

    switch (mode) {
        case 0b00:
            break;
        
        case 0b01:
            handler_address += (cause & ~TRAP_INTERRUPT_BIT) * 4;
            break;
        
        default:
            throw std::runtime_error(std::format("Unhandled machine trap mode {}", mode));
    }

    csrs[CSR_MCAUSE] = cause;
    csrs[CSR_MEPC] = pc;

    mstatus.MPIE = mstatus.MIE;
    mstatus.MIE = 0;

    mstatus.SPIE = sstatus.SIE;

    switch (privilege_level) {
        case PrivilegeLevel::Machine:
                mstatus.MPP = MACHINE_MODE;
                break;
            
        case PrivilegeLevel::Supervisor:
            mstatus.MPP = SUPERVISOR_MODE;
            break;
        
        case PrivilegeLevel::User:
            mstatus.MPP = USER_MODE;
            break;
    }

    pc = handler_address;
    privilege_level = PrivilegeLevel::Machine;
}

void VirtualMachine::RaiseSupervisorTrap(uint32_t cause) {
    auto [handler_address, valid] = TranslateMemoryAddress(csrs[CSR_STVEC], false, false);

    if (!valid) return;

    auto mode = handler_address & 0b11;
    handler_address &= ~0b11;

    switch (mode) {
        case 0b00:
            break;
        
        case 0b01:
            handler_address += (cause & ~TRAP_INTERRUPT_BIT) * 4;
            break;
        
        default:
            throw std::runtime_error(std::format("Unhandled supervisor trap mode {}", mode));
    }

    csrs[CSR_SCAUSE] = cause;
    csrs[CSR_SEPC] = pc;

    mstatus.SPIE = mstatus.SIE;
    mstatus.SIE = 0;

    switch (privilege_level) {
        case PrivilegeLevel::Supervisor:
            sstatus.SPP = 0;
            break;

        case PrivilegeLevel::User:
            sstatus.SPP = 1;
            break;
        
        default:
            throw std::runtime_error(std::format("Unhandled supervisor trap"));
    }

    pc = handler_address;
    privilege_level = PrivilegeLevel::Supervisor;
}

std::pair<VirtualMachine::TLBEntry, bool> VirtualMachine::GetTLBLookup(uint32_t virt_addr, bool bypass_cache, bool is_amo) {
    constexpr auto KB_OFFSET_BITS = GetLog2(0x1000);
    constexpr auto MB_OFFSET_BITS = GetLog2(0x200000);

    auto kb_tag = (virt_addr & ~((1ULL << KB_OFFSET_BITS) - 1)) >> 1;
    auto mb_tag = (virt_addr & ~((1ULL << MB_OFFSET_BITS) - 1)) >> 1;

    if (!bypass_cache) {
        for (size_t i = 0; i < tlb_cache.size(); i++) {
            if (!tlb_cache[i].tlb_entry.V) continue;

            if (tlb_cache[i].super && (mb_tag == tlb_cache[i].tag))
                return {tlb_cache[i].tlb_entry, true};
            
            if (!tlb_cache[i].super && (kb_tag == tlb_cache[i].tag))
                return {tlb_cache[i].tlb_entry, false};
        }
    }

    union VirtualAddress {
        struct {
            uint32_t offset : 12;
            uint32_t vpn_0 : 10;
            uint32_t vpn_1 : 10;
        };
        uint32_t raw;
    };

    VirtualAddress vaddr;
    vaddr.raw = virt_addr;

    uint32_t root_table_address = satp.PPN << 12;

    auto ReadTLBEntry = [&](uint32_t address) {
        TLBEntry ppn;
        ppn.V = 0;
        auto ppn_read = memory.PeekWord(address);
        if (!ppn_read.second) {
            RaiseException(is_amo ? EXCEPTION_STORE_AMO_ACCESS_FAULT : EXCEPTION_LOAD_ACCESS_FAULT);
            return ppn;
        }
        
        ppn.raw = ppn_read.first;
        if (!ppn.V || (!ppn.R && ppn.W)) {
            RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
            return ppn;
        }

        if (!ppn.X && !ppn.W && ppn.R) {
            RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
            ppn.V = 0;
            return ppn;
        }

        if (ppn.X && ppn.W && !ppn.R) {
            RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
            ppn.V = 0;
            return ppn;
        }
        
        return ppn;
    };

    auto& cache = tlb_cache[tlb_cache_round_robin];
    tlb_cache_round_robin++;
    if (tlb_cache_round_robin >= TLB_CACHE_SIZE) tlb_cache_round_robin = 0;

    cache = TLBCacheEntry();

    auto tlb = ReadTLBEntry(root_table_address + vaddr.vpn_1 * 4);

    if (!tlb.V) return {tlb, false};

    if (!tlb.R && !tlb.X) {
        constexpr uint32_t PAGE_SIZE = 0x1000;
        tlb = ReadTLBEntry(tlb.PPN * PAGE_SIZE + vaddr.vpn_0 * 4);

        cache.tag = kb_tag;
        cache.tlb_entry = tlb;
    }
    else {
        if (tlb.PPN_0 != 0) {
            RaiseMachineTrap(EXCEPTION_INSTRUCTION_PAGE_FAULT);
            tlb.V = 0;
            return {tlb, false};
        }
        
        cache.tag = mb_tag;
        cache.super = 1;
        cache.tlb_entry = tlb;
    }

    return {tlb, cache.super != 0};
}

std::pair<uint32_t, bool> VirtualMachine::TranslateMemoryAddress(uint32_t address, bool is_write, bool is_execute, bool is_amo) {
    if (!IsUsingVirtualMemory()) return {address, true};
    
    auto [tlb, super] = GetTLBLookup(address);

    if (!tlb.V) return {0, false};

    if (is_execute && !tlb.X) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    if (is_write && !tlb.W) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    if (!is_write && !tlb.R) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    if (is_amo && (!tlb.R || !tlb.W)) {
        RaiseException(EXCEPTION_STORE_AMO_PAGE_FAULT);
    }

    if (privilege_level == PrivilegeLevel::User && !tlb.U) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    if (privilege_level == PrivilegeLevel::Supervisor && tlb.U && !sstatus.SUM) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    if (!tlb.A || (tlb.D && is_write)) {
        RaiseException(EXCEPTION_INSTRUCTION_PAGE_FAULT);
        return {0, false};
    }

    union VirtualAddress {
        struct {
            uint32_t offset : 12;
            uint32_t vpn_0 : 10;
            uint32_t vpn_1 : 10;
        };
        uint32_t raw;
    };

    VirtualAddress vaddr;
    vaddr.raw = address;

    uint32_t phys_address;
    if (super) {
        phys_address = tlb.PPN_1 << 22;
        phys_address |= vaddr.vpn_0 << 12;
        phys_address |= vaddr.offset;
    }
    else {
        phys_address = tlb.PPN << 12;
        phys_address |= vaddr.offset;
    }

    return {phys_address, true};
}

void VirtualMachine::Setup() {
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
    csrs[CSR_SSCRATCH] = 0;
    csrs[CSR_SEPC] = 0;
    csrs[CSR_SCAUSE] = 0;
    csrs[CSR_STVAL] = 0;
    csrs[CSR_SIP] = 0;
    csrs[CSR_SCAUSE] = 0;
    csrs[CSR_SEPC] = 0;
    
    satp.raw = 0;

    // Machine

    csrs[CSR_MSTATUS] = 0;
    csrs[CSR_MTVEC] = 0;
    csrs[CSR_MSCRATCH] = 0;
    csrs[CSR_MCAUSE] = 0;
    csrs[CSR_MEPC] = 0;

    // TODO Implement this!
    csrs[CSR_MCONFIGPTR] = 0;

    privilege_level = PrivilegeLevel::Machine;

    cycles = 0;
}

VirtualMachine::VirtualMachine(Memory& memory, uint32_t starting_pc, uint32_t hart_id) : memory{memory}, pc{starting_pc} {
    csrs[CSR_MVENDORID] = 0;

    csrs[CSR_MARCHID] = ('E' << 24) | ('N' << 16) | ('I' << 8) | ('H');
    csrs[CSR_MIMPID] = ('C' << 24) | ('A' << 16) | ('M' << 8) | ('V');

    csrs[CSR_MHARTID] = hart_id;

    csrs[CSR_MISA] = ISA_32_BITS | ISA_A | ISA_D | ISA_F | ISA_I | ISA_M;

    csr_mapped_memory = std::make_shared<CSRMappedMemory>();
    memory.AddMemoryRegion(csr_mapped_memory);

    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto dur_ms = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    
    double ms = dur_ms.count() / 1000000.0;
    csr_mapped_memory->time = ms * CSRMappedMemory::TICKS_PER_SECOND;
    
    Setup();
}

VirtualMachine::VirtualMachine(VirtualMachine&& vm) : memory{vm.memory}, pc{vm.pc} {
    regs = std::move(vm.regs);
    fregs = std::move(vm.fregs);
    csrs = std::move(vm.csrs);
    tlb_cache = std::move(vm.tlb_cache);
    running = std::move(vm.running);
    paused = std::move(vm.paused);
    pause_on_break = std::move(vm.pause_on_break);
    pause_on_restart = std::move(vm.pause_on_restart);
    err = std::move(vm.err);
    break_points = std::move(vm.break_points);
    ticks = std::move(vm.ticks);
    history_delta = std::move(vm.history_delta);
    history_tick = std::move(vm.history_tick);
    csr_mapped_memory = std::move(vm.csr_mapped_memory);
    cycles = std::move(vm.cycles);
    privilege_level = std::move(vm.privilege_level);
}

VirtualMachine::~VirtualMachine() {
    running = false;
}

bool VirtualMachine::Step(uint32_t steps) {
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
        cycles++;

        if (waiting_for_interrupt) {
            if (mip != 0 || sip != 0 || (mie & ~mideleg) == 0)
                waiting_for_interrupt = false;

            if ((mie & mideleg) != 0 && (sie & mideleg) == 0)
                waiting_for_interrupt = false;

            else
                continue;
        }

        if (mstatus.MIE) {
            auto pending_interrupts = mip;
            pending_interrupts &= mie;

            auto delegated = pending_interrupts & mideleg;
            sip |= delegated;

            pending_interrupts &= ~delegated;

            bool handled = false;

            if (pending_interrupts) {
                for (uint32_t cause = 31; cause > 32; cause--) {
                    if (pending_interrupts & (1ULL << cause)) {
                        RaiseMachineTrap(cause | TRAP_INTERRUPT_BIT);
                        handled = true;
                        break;
                    }
                }
            }

            if (!handled && mstatus.SIE && sstatus.SIE) {
                pending_interrupts = sip;
                pending_interrupts &= sie;

                if (pending_interrupts) {
                    for (uint32_t cause = 31; cause > 32; cause--) {
                        if (pending_interrupts & (1ULL << cause)) {
                            RaiseSupervisorTrap(cause | TRAP_INTERRUPT_BIT);
                            break;
                        }
                    }
                }
            }
        }
        
        if (pc & 0b11) {
            RaiseException(EXCEPTION_INSTRUCTION_ADDRESS_FAULT);
            continue;
        }

        auto [translated_address, translation_valid] = TranslateMemoryAddress(pc, false, true);
        if (!translation_valid) continue;
        
        auto instr = RVInstruction::FromUInt32(memory.ReadWord(translated_address));

        bool inc_pc = true;

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
            
            case Type::LB: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;
                regs[instr.rd] = SignExtend(memory.ReadByte(translated_address), 7);
                break;
            }
            
            case Type::LH: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;
                regs[instr.rd] = SignExtend(memory.ReadHalf(translated_address), 15);
                break;
            }
            
            case Type::LW: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;
                regs[instr.rd] = memory.ReadWord(translated_address);
                break;
            }

            case Type::LBU: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;
                regs[instr.rd] = memory.ReadByte(translated_address);
                break;
            }
            
            case Type::LHU: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;
                regs[instr.rd] = memory.ReadHalf(translated_address);
                break;
            }
            
            case Type::SB: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, true, false);
                if (!translation_valid) continue;
                memory.WriteByte(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::SH: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, true, false);
                if (!translation_valid) continue;
                memory.WriteHalf(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::SW: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, true, false);
                if (!translation_valid) continue;
                memory.WriteWord(translated_address, regs[instr.rs2]);
                break;
            }
            
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
                switch (privilege_level) {
                    case PrivilegeLevel::Machine:
                        if (!ecall_handlers.contains(regs[REG_A0]))
                            EmptyECallHandler(csrs[CSR_MHARTID], memory, regs, fregs);
                        
                        else
                            ecall_handlers[regs[REG_A0]](csrs[CSR_MHARTID], memory, regs, fregs);
                        break;
                    
                    case PrivilegeLevel::Supervisor:
                        RaiseException(EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE);
                        inc_pc = false;
                        break;
                    
                    case PrivilegeLevel::User:
                        RaiseException(EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE);
                        inc_pc = false;
                        break;
                }
                break;
            
            case Type::EBREAK:
                if (privilege_level == PrivilegeLevel::User) {
                    RaiseException(EXCEPTION_BREAKPOINT);
                    inc_pc = false;
                }
                
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
                    regs[instr.rd] = -1U;
                
                else
                    regs[instr.rd] = AsUnsigned(lhs / rhs);
                break;

            }
            
            case Type::DIVU: {
                auto lhs = regs[instr.rs1];
                auto rhs = regs[instr.rs2];

                if (rhs == 0)
                    regs[instr.rd] = -1U;
                
                else
                    regs[instr.rd] = lhs / rhs;

                break;
            }
            
            case Type::REM: {
                auto lhs = AsSigned(regs[instr.rs1]);
                auto rhs = AsSigned(regs[instr.rs2]);

                if (rhs == 0)
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = AsUnsigned(lhs % rhs);

                break;
            }
            
            case Type::REMU: {
                auto lhs = regs[instr.rs1];
                auto rhs = regs[instr.rs2];

                if (rhs == 0)
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = lhs % rhs;

                break;
            }
            
            case Type::LR_W: {
                if (instr.rs2 != 0) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.ReadWordReserved(translated_address, csrs[CSR_MHARTID]);
                break;
            }
            
            case Type::SC_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], true, false);
                if (!translation_valid) continue;
                
                if (memory.WriteWordConditional(translated_address, regs[instr.rs2], csrs[CSR_MHARTID]))
                    regs[instr.rd] = 0;
                
                else
                    regs[instr.rd] = 1;
                
                break;
            }
            
            case Type::AMOSWAP_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicSwap(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOADD_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicAdd(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOXOR_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicXor(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOAND_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicAnd(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOOR_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;
                
                regs[instr.rd] = memory.AtomicOr(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOMIN_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicMin(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOMAX_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicMax(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOMINU_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicMinU(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::AMOMAXU_W: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false, true);
                if (!translation_valid) continue;

                regs[instr.rd] = memory.AtomicMaxU(translated_address, regs[instr.rs2]);
                break;
            }
            
            case Type::FLW: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;

                fregs[instr.rd] = ToFloat(translated_address);
                break;
            }
            
            case Type::FSW: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, true, false);
                if (!translation_valid) continue;
                memory.WriteWord(translated_address, ToUInt32(fregs[instr.rs2]));
                break;
            }
            
            case Type::FMADD_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f + fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FMSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f - fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FNMSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                float result = -(fregs[instr.rs1].f * fregs[instr.rs2].f) + fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FNMADD_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF32(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF32(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                float result = -(fregs[instr.rs1].f * fregs[instr.rs2].f) - fregs[instr.rs3].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = result;

                break;
            }
            
            case Type::FADD_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                float result = fregs[instr.rs1].f + fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FSUB_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                float result = fregs[instr.rs1].f - fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FMUL_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                float result = fregs[instr.rs1].f * fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FDIV_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                float result = fregs[instr.rs1].f / fregs[instr.rs2].f;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F32_NAN;

                else
                    fregs[instr.rd].f = result;
                
                break;
            }
            
            case Type::FSQRT_S: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool is_inf, is_nan, is_qnan, is_neg;
                ClassF32(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, &is_neg);

                if (is_inf || is_nan || is_qnan || is_neg)
                    fregs[instr.rd].u64 = RV_F32_NAN;
                
                else
                    fregs[instr.rd].f = sqrtf(fregs[instr.rs1].f);
                
                break;
            }
            
            case Type::FSGNJ_S: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u32 &= ~(1<<31);
                result.u32 |= rhs.u32 & (1<<31);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJN_S: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u32 &= ~(1<<31);
                result.u32 |= (~rhs.u32) & (1<<31);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJX_S: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

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

                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

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

                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                auto val = AsSigned(regs[instr.rs1]);

                fregs[instr.rd].f = val;
                if (fregs[instr.rd].f != val)
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            }
            
            case Type::FCVT_S_WU:
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                fregs[instr.rd].f = regs[instr.rs1];
                if (fregs[instr.rd].f != regs[instr.rs1])
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            
            case Type::FMV_W_X:
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                fregs[instr.rd].u64 = 0;
                fregs[instr.rd].u32 = regs[instr.rs1];
                break;
            
            case Type::FLD: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, false, false);
                if (!translation_valid) continue;

                uint64_t val = memory.ReadWord(translated_address);
                val |= static_cast<uint64_t>(memory.ReadWord(translated_address + 4)) << 32;
                fregs[instr.rd] = ToDouble(val);
                break;
            }
            
            case Type::FSD: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1] + instr.immediate, true, false);
                if (!translation_valid) continue;
                
                auto val = ToUInt64(fregs[instr.rs2]);
                memory.WriteWord(translated_address, static_cast<uint32_t>(val));
                memory.WriteWord(translated_address + 4, static_cast<uint32_t>(val >> 32));
                break;
            }
            
            case Type::FMADD_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d + fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FMSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d - fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FNMSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                double result = -(fregs[instr.rs1].d * fregs[instr.rs2].d) + fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FNMADD_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_is_inf;
                bool rhs_is_zero;
                ClassF64(fregs[instr.rs1], &lhs_is_inf, nullptr, nullptr, nullptr, nullptr, nullptr);
                ClassF64(fregs[instr.rs2], nullptr, nullptr, nullptr, nullptr, &rhs_is_zero, nullptr);

                if (lhs_is_inf && rhs_is_zero) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

                double result = -(fregs[instr.rs1].d * fregs[instr.rs2].d) - fregs[instr.rs3].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = result;

                break;
            }
            
            case Type::FADD_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                double result = fregs[instr.rs1].d + fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FSUB_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                double result = fregs[instr.rs1].d - fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FMUL_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                double result = fregs[instr.rs1].d * fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FDIV_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                double result = fregs[instr.rs1].d / fregs[instr.rs2].d;

                if (CheckFloatErrors())
                    fregs[instr.rd].u64 = RV_F64_NAN;

                else
                    fregs[instr.rd].d = result;
                
                break;
            }
            
            case Type::FSQRT_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool is_inf, is_nan, is_qnan, is_neg;
                ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, &is_neg);

                if (is_inf || is_nan || is_qnan || is_neg)
                    fregs[instr.rd].u64 = RV_F64_NAN;
                
                else
                    fregs[instr.rd].d = sqrt(fregs[instr.rs1].d);
                
                break;
            }
            
            case Type::FSGNJ_D: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 &= ~(1ULL<<63);
                result.u64 |= rhs.u64 & (1ULL<<63);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJN_D: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 &= ~(1ULL<<63);
                result.u64 |= (~rhs.u64) & (1ULL<<63);
                fregs[instr.rd] = result;
                break;
            }
            
            case Type::FSGNJX_D: {
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                Float result = fregs[instr.rs1];
                Float rhs = fregs[instr.rs2];

                result.u64 ^= rhs.u64 & (1ULL<<63);
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

                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

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

                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;
                
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
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                bool lhs_snan, lhs_qnan;
                ClassF32(fregs[instr.rs1], nullptr, &lhs_snan, &lhs_qnan, nullptr, nullptr, nullptr);

                if (lhs_snan) fregs[instr.rd].u64 = RV_F64_NAN;
                else if (lhs_qnan) fregs[instr.rd].u64 = RV_F64_QNAN;
                else fregs[instr.rd].d = fregs[instr.rs1].f;

                break;
            }
            
            case Type::FEQ_D: {
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                if (!ChangeRoundingMode(instr.rm)) {
                    RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                    inc_pc = false;
                    break;
                }

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
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;

                auto val = AsSigned(regs[instr.rs1]);

                fregs[instr.rd].d = val;
                if (fregs[instr.rd].d != val)
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            }
            
            case Type::FCVT_D_WU:
                mstatus.FS = FS_DIRTY;
                sstatus.FS = FS_DIRTY;
                
                fregs[instr.rd].d = regs[instr.rs1];
                if (fregs[instr.rd].d != regs[instr.rs1])
                    SetFloatFlags(true, false, false, false, false);
                
                break;
            
            case Type::SRET:
                if (privilege_level == PrivilegeLevel::User) {
                    throw std::runtime_error("Cannot use SRET in user mode");
                }

                pc = csrs[CSR_SEPC];
                sstatus.SIE = sstatus.SPIE;

                if (sstatus.SPP)
                    privilege_level = PrivilegeLevel::Supervisor;
                
                else
                    privilege_level = PrivilegeLevel::User;
                
                continue;
            
            case Type::MRET:
                if (privilege_level == PrivilegeLevel::Supervisor) {
                    RaiseInterrupt(INTERRUPT_SUPERVISOR_SOFTWARE);
                    break;
                }

                if (privilege_level == PrivilegeLevel::User) {
                    throw std::runtime_error(std::format("Cannot use MRET in user mode"));
                }

                pc = csrs[CSR_MEPC];
                mstatus.MIE = mstatus.MPIE;
                sstatus.SIE = mstatus.SPIE;

                switch (mstatus.MPP) {
                    case MACHINE_MODE:
                        privilege_level = PrivilegeLevel::Machine;
                        break;
                    
                    case SUPERVISOR_MODE:
                        privilege_level = PrivilegeLevel::Supervisor;
                        break;
                    
                    case USER_MODE:
                        privilege_level = PrivilegeLevel::User;
                        break;
                    
                    default:
                        throw std::runtime_error("Cannot MRET to hypervisor");
                }
                continue;
            
            case Type::WFI:
                waiting_for_interrupt = true;
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
            
            case Type::CUST_TVA: {
                auto [translated_address, translation_valid] = TranslateMemoryAddress(regs[instr.rs1], false, false);
                regs[instr.rd] = translated_address;
                break;
            }
            
            case Type::CUST_MTRAP:
                pc += 4;

                switch (regs[instr.rs2] & 0b11) {
                    case MACHINE_MODE:
                        privilege_level = PrivilegeLevel::Machine;
                        break;
                    
                    case SUPERVISOR_MODE:
                        privilege_level = PrivilegeLevel::Supervisor;
                        break;
                    
                    default:
                        privilege_level = PrivilegeLevel::User;
                        break;
                }

                RaiseMachineTrap(regs[instr.rs1]);
                continue;
            
            case Type::CUST_STRAP:
                pc += 4;

                switch (regs[instr.rs2] & 0b11) {
                    case MACHINE_MODE:
                        privilege_level = PrivilegeLevel::Machine;
                        break;
                    
                    case SUPERVISOR_MODE:
                        privilege_level = PrivilegeLevel::Supervisor;
                        break;
                    
                    default:
                        privilege_level = PrivilegeLevel::User;
                        break;
                }
                
                RaiseSupervisorTrap(regs[instr.rs1]);
                continue;

            case Type::INVALID:
            default:
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
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
                if (inc_pc)
                    pc += 4;
        }
        
        if (instr.rd == 0) regs[instr.rd] = 0;

        if (IsBreakPoint(pc)) return true;
    }

    return false;
}

void VirtualMachine::Run() {
    while (running) {
        if (paused) {
#if defined(_WIN32) || defined(_WIN64)
            SwitchToThread();
#else
            sched_yield();
#endif
        
        }
        else {
            if (Step() && pause_on_break)
                paused = true;
        }
    }
}

void VirtualMachine::GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, uint32_t& pc) {
    registers = regs;
    fregisters = fregs;
    pc = this->pc;
}

void VirtualMachine::GetCSRSnapshot(std::unordered_map<uint32_t, uint32_t>& csrs) const {
    csrs = this->csrs;
    auto mcycle = static_cast<uint32_t>(cycles);
    auto mcycleh = static_cast<uint32_t>(cycles >> 32);
    csrs[CSR_MCYCLE] = mcycle;
    csrs[CSR_MCYCLEH] = mcycleh;
    csrs[CSR_CYCLE] = mcycle;
    csrs[CSR_CYCLEH] = mcycleh;

    csrs[CSR_TIME] = static_cast<uint32_t>(csr_mapped_memory->time);
    csrs[CSR_TIMEH] = static_cast<uint32_t>(csr_mapped_memory->time >> 32);

    csrs[CSR_MIP] = mip;
    csrs[CSR_MIE] = mie;
    csrs[CSR_MIDELEG] = mideleg;
    csrs[CSR_SIP] = sip;
    csrs[CSR_SIE] = sie;

    csrs[CSR_MSTATUS] = mstatus.raw;
    csrs[CSR_SSTATUS] = sstatus.raw;

    csrs[CSR_SATP] = satp.raw;
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

    if (privilege_level == PrivilegeLevel::User)
        return false;

    return instr.type == RVInstruction::Type::EBREAK;
}

void VirtualMachine::UpdateTime() {
    history_delta.push_back(delta_time());
    history_tick.push_back(ticks);
    ticks = 0;
    
    csr_mapped_memory->time += static_cast<uint64_t>(delta_time() * CSRMappedMemory::TICKS_PER_SECOND);
    if (csr_mapped_memory->time >= csr_mapped_memory->time_cmp)
        RaiseInterrupt(INTERRUPT_MACHINE_TIMER);

    while (history_delta.size() > MAX_HISTORY) {
        history_delta.erase(history_delta.begin());
        history_tick.erase(history_tick.begin());
    }
}

void VirtualMachine::EmptyECallHandler(uint32_t hart, Memory&, std::array<uint32_t, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&) {
    throw std::runtime_error(std::format("Hart {} called unknown ECall handler: {}", hart, regs[REG_A0]));
}

std::unordered_map<uint32_t, VirtualMachine::ECallHandler> VirtualMachine::ecall_handlers;