#include "VirtualMachine.hpp"

#include "RV64.hpp"

#include <sstream>
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

std::string VirtualMachine::VMException::dump() const {
    std::array<std::string, REGISTER_COUNT> names = {
        "zero",
        "ra",
        "sp",
        "gp",
        "tp",
        "t0",
        "t1",
        "t2",
        "s0 / fp",
        "s1",
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
        "s2",
        "s3",
        "s4",
        "s5",
        "s6",
        "s7",
        "s8",
        "s9",
        "s10",
        "s11",
        "t3",
        "t4",
        "t5",
        "t6"
    };

    std::array<std::string, REGISTER_COUNT> fnames = {
        "ft0",
        "ft1",
        "ft2",
        "ft3",
        "ft4",
        "ft5",
        "ft6",
        "ft7",
        "fs0",
        "fs1",
        "fa0",
        "fa1",
        "fa2",
        "fa3",
        "fa4",
        "fa5",
        "fa6",
        "fa7",
        "fs2",
        "fs3",
        "fs4",
        "fs5",
        "fs6",
        "fs7",
        "fs8",
        "fs9",
        "fs10",
        "fs11",
        "ft8",
        "ft9",
        "ft10",
        "ft11"
    };

    using CSRTuple = std::tuple<Long, std::string>;
    
    static std::array csrs_names = {
        CSRTuple(CSR_FFLAGS, "fflags"),
        CSRTuple(CSR_FRM, "frm"),
        CSRTuple(CSR_FCSR, "fcsr"),
        CSRTuple(CSR_CYCLE, "cycle"),
        CSRTuple(CSR_TIME, "time"),
        CSRTuple(CSR_INSTRET, "instret"),
        CSRTuple(CSR_SSTATUS, "sstatus"),
        CSRTuple(CSR_SIE, "sie"),
        CSRTuple(CSR_STVEC, "stvec"),
        CSRTuple(CSR_SENVCFG, "senvcfg"),
        CSRTuple(CSR_SSCRATCH, "sscratch"),
        CSRTuple(CSR_SEPC, "sepc"),
        CSRTuple(CSR_SCAUSE, "scause"),
        CSRTuple(CSR_STVAL, "stval"),
        CSRTuple(CSR_SIP, "sip"),
        CSRTuple(CSR_SATP, "satp"),
        CSRTuple(CSR_MVENDORID, "mvendorid"),
        CSRTuple(CSR_MARCHID, "marchid"),
        CSRTuple(CSR_MIMPID, "mimpid"),
        CSRTuple(CSR_MHARTID, "mhartid"),
        CSRTuple(CSR_MCONFIGPTR, "mconfigptr"),
        CSRTuple(CSR_MSTATUS, "mstatus"),
        CSRTuple(CSR_MISA, "misa"),
        CSRTuple(CSR_MEDELEG, "medeleg"),
        CSRTuple(CSR_MIDELEG, "mideleg"),
        CSRTuple(CSR_MIE, "mie"),
        CSRTuple(CSR_MTVEC, "mtvec"),
        CSRTuple(CSR_MSCRATCH, "mscratch"),
        CSRTuple(CSR_MEPC, "mepc"),
        CSRTuple(CSR_MCAUSE, "mcause"),
        CSRTuple(CSR_MTVAL, "mtval"),
        CSRTuple(CSR_MIP, "mip"),
        CSRTuple(CSR_MTINST, "mtinst"),
        CSRTuple(CSR_MTVAL2, "mtval2"),
        CSRTuple(CSR_MENVCFG, "menvcfg"),
        CSRTuple(CSR_MSECCFG, "mseccfg"),
        CSRTuple(CSR_MCYCLE, "mcycle"),
        CSRTuple(CSR_MINSTRET, "minstret"),
    };

    std::stringstream ss;
    ss << std::format("hart {}\n\n", hart_id);
    
    for (size_t i = 0; i < regs.size(); i++)
        ss << std::format("{:<10}x{:<2} : 0x{:0>16x} ({})\n", names[i].c_str(), i, regs[i].u64, regs[i].s64);

    ss << "\n";

    for (size_t i = 0; i < fregs.size(); i++) {
        if (fregs[i].is_double)
            ss << std::format("{:<10}f{:<2} : 0x{:0>16x} ({:.8g})\n", fnames[i].c_str(), i, fregs[i].u64, fregs[i].d);
        
        else
            ss << std::format("{:<10}f{:<2} : 0x{:0>8x} ({:.8g})\n", fnames[i].c_str(), i, fregs[i].u32, fregs[i].f);
    }

    ss << "\n";

    for (auto [csr, name] : csrs_names) {
        if (csrs.contains(csr))
            ss << std::format("{:<16}0x{:<3x} : 0x{:0>16x} ({})", name, csr, csrs.at(csr), csrs.at(csr));
        else
            ss << std::format("{:<16}0x{:<3x} : Unknown\n", name, csr);
    }

    ss << "\n";

    ss << std::format("pc : 0x{:0>16x}", virtual_pc);

    {
        const Long pc = std::get<0>(physical_pc);
        const bool valid = std::get<1>(physical_pc);

        if (valid)
            ss << std::format(" -> 0x{:0>16x}\n", pc);
        
        else
            ss << "\n";
    }

    {
        Word word = std::get<0>(instruction);
        bool valid = std::get<1>(instruction);

        if (valid) {
            auto instr = RVInstruction::FromUInt32(word);
            ss << std::format("instruction : {}\n", std::string(instr).c_str());
        }
    }

    return ss.str();
}

const int VirtualMachine::default_rounding_mode = fegetround();

bool VirtualMachine::CSRPrivilegeCheck(Long csr) {
    if (csr < 4 || (csr >= 0xc00 && csr < 0xcf0))
        return true;
    
    if ((csr >= 0x100 && csr < 0x144) || csr == 0x180)
        return privilege_level != PrivilegeLevel::User;
    
    return privilege_level == PrivilegeLevel::Machine;
}

Long VirtualMachine::ReadCSR(Long csr, bool is_internal_read) {
    if (!is_internal_read && !CSRPrivilegeCheck(csr)) {
        RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
        return 0;
    }
    
    if (csr >= CSR_MHPMEVENT3 && csr < (CSR_MHPMEVENT3 + CSR_PERFORMANCE_EVENT_MAX - 3))
        return 0;
    
    if (csr >= CSR_MHPMCOUNTER3 && csr < (CSR_MHPMCOUNTER3 + CSR_PERF_COUNTER_MAX - 3))
        return 0;

    switch (csr) {
        case CSR_FFLAGS:
            return csrs[CSR_FCSR] & CSR_FCSR_FLAGS;
        
        case CSR_FRM:
            return csrs[CSR_FCSR] >> 5;

        case CSR_MCYCLE:
        case CSR_CYCLE:
            return static_cast<Long>(cycles);
        
        case CSR_TIME:
            return static_cast<Long>(csr_mapped_memory->time);

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

void VirtualMachine::WriteCSR(Long csr, Long value) {
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
        case CSR_CYCLE:
        case CSR_TIME:
        case CSR_MCOUNTEREN:
        case CSR_MCOUNTINHIBIT:
        case CSR_MENVCFG:
        case CSR_SENVCFG:
            return; // Non writable
        
        case CSR_FFLAGS: {
            auto last = csrs[CSR_FCSR];
            last &= ~CSR_FCSR_FLAGS;
            last |= value & CSR_FCSR_FLAGS;
            csrs[CSR_FCSR] = last;
            break;
        }

        case CSR_FRM: {
            auto last = csrs[CSR_FCSR];
            last &= ~(0b111 << 5);
            last |= (value & 0b111) << 5;
            csrs[CSR_FCSR] = last;
            break;
        }

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

bool VirtualMachine::ChangeRoundingMode(Byte rm) {
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

void VirtualMachine::RaiseInterrupt(Long cause) {
    auto cause_bit = 1ULL << cause;

    static std::mutex lock;
    lock.lock();
    csrs[CSR_MIP] |= cause_bit;
    lock.unlock();
}

void VirtualMachine::RaiseException(Long cause) {
    auto cause_bit = 1ULL << cause;

    auto delegate = csrs[CSR_MEDELEG];

    if (delegate & cause_bit)
        RaiseSupervisorTrap(cause);
    
    else
        RaiseMachineTrap(cause);
    
}

void VirtualMachine::RaiseMachineTrap(Long cause) {
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

void VirtualMachine::RaiseSupervisorTrap(Long cause) {
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

std::pair<VirtualMachine::TLBEntry, bool> VirtualMachine::GetTLBLookup(Address virt_addr, bool bypass_cache, bool is_amo) {
    // TODO Fix this!
    constexpr auto KB_OFFSET_BITS = GetLog2(0x1000U);
    constexpr auto MB_OFFSET_BITS = GetLog2(0x200000U);

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
            Word offset : 12;
            Word vpn_0 : 10;
            Word vpn_1 : 10;
        };
        Word raw;
    };

    VirtualAddress vaddr;
    vaddr.raw = virt_addr;

    Word root_table_address = satp.PPN << 12;

    auto ReadTLBEntry = [&](Address address) {
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
        constexpr Word PAGE_SIZE = 0x1000;
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

std::pair<Address, bool> VirtualMachine::TranslateMemoryAddress(Address address, bool is_write, bool is_execute, bool is_amo) {
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
            Word offset : 12;
            Word vpn_0 : 10;
            Word vpn_1 : 10;
        };
        Word raw;
    };

    VirtualAddress vaddr;
    vaddr.raw = address;

    Word phys_address;
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
        r.u64 = 0;
    }

    for (auto& f : fregs) {
        f.d = 0.0;
    }

    // User

    csrs[CSR_CYCLE] = 0;
    csrs[CSR_TIME] = 0;
    csrs[CSR_INSTRET] = 0;

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

VirtualMachine::VirtualMachine(Memory& memory, Address starting_pc, Address hart_id) : memory{memory}, pc{starting_pc} {
    csrs[CSR_MVENDORID] = 0;

    csrs[CSR_MARCHID] = ('E' << 24) | ('N' << 16) | ('I' << 8) | ('H');
    csrs[CSR_MIMPID] = ('C' << 24) | ('A' << 16) | ('M' << 8) | ('V');

    csrs[CSR_MHARTID] = hart_id;

    csrs[CSR_MISA] = ISA_64_BITS | ISA_A | ISA_D | ISA_F | ISA_I | ISA_M;

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

bool VirtualMachine::SingleStep() {
    auto SignExtendUnsigned = [](Long value, Long bit) {
        Long sign = -1ULL << bit;
        if (value & (1ULL << bit)) return value | sign;
        return value;
    };

    auto SignExtendSigned = [](SLong value, Long bit) {
        union S64U64 {
            Long u;
            SLong s;
        };
        
        S64U64 v;
        v.s = value;
        Long sign = -1ULL << bit;
        if (v.u & (1ULL << bit)) v.u |= sign;
        return v.s;
    };

    auto AsSigned32 = [](Word value) {
        union S32U32 {
            Word u;
            SWord s;
        };

        S32U32 v;
        v.u = value;
        return v.s;
    };

    auto AsUnsigned32 = [](SWord value) {
        union S32U32 {
            Word u;
            SWord s;
        };

        S32U32 v;
        v.s = value;
        return v.u;
    };

    auto AsSigned64 = [](Long value) {
        union S64U64 {
            Long u;
            SLong s;
        };

        S64U64 v;
        v.u = value;
        return v.s;
    };

    auto AsUnsigned64 = [](SLong value) {
        union S64U64 {
            Long u;
            SLong s;
        };

        S64U64 v;
        v.s = value;
        return v.u;
    };

    auto ToFloat = [](Word value) {
        union F32U32 {
            float f;
            Word u;
        };

        F32U32 v;
        v.u = value;
        Float f;
        f.f = v.f;
        f.is_double = false;
        return f;
    };

    auto ToDouble = [](Long value) {
        union F64U64 {
            double d;
            Long u;
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
            Word u;
        };

        F32U32 v;
        v.f = value.f;
        return v.u;
    };

    auto ToUInt64 = [](Float value) {
        union F64U64 {
            double d;
            Long u;
        };

        F64U64 v;
        v.d = value.d;
        return v.u;
    };

    auto ClassF32 = [](Float value, bool* is_inf, bool* is_nan, bool* is_qnan, bool* is_subnormal, bool* is_zero, bool* is_neg) {
        // TODO This might not work as intended
        Byte sign = value.u32 >> 31;
        Byte exp = (value.u32 >> 23) & 0xff;
        Word frac = value.u32 & 0x7fffff;
        
        if (is_inf) *is_inf = exp == 0xff && frac == 0;
        if (is_nan) *is_nan = exp == 0xff && frac != 0 && !(frac & 0x400000);
        if (is_qnan) *is_qnan = exp == 0xff && (frac & 0x400000);
        if (is_subnormal) *is_subnormal = exp == 0 && frac != 0;
        if (is_zero) *is_zero = exp == 0 && frac == 0;
        if (is_neg) *is_neg = sign != 0;
    };

    auto ClassF64 = [](Float value, bool* is_inf, bool* is_nan, bool* is_qnan, bool* is_subnormal, bool* is_zero, bool* is_neg) {
        // TODO This might not work as intended
        Byte sign = value.u64 >> 63;
        Half exp = (value.u64 >> 52) & 0x7ff;
        Long frac = value.u64 & 0xfffffffffffff;
        
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

    ticks++;

    using Type = RVInstruction::Type;

    constexpr Long RV_F32_NAN = 0xffffffff7fc00000;
    constexpr Long RV_F32_QNAN = 0xffffffffffc00000;
    constexpr Long RV_F64_NAN = 0x7ff0000000000000;
    constexpr Long RV_F64_QNAN = 0xfff0000000000000;
    
    cycles++;

    if (waiting_for_interrupt) {
        if (mip != 0 || sip != 0 || (mie & ~mideleg) == 0)
            waiting_for_interrupt = false;

        if ((mie & mideleg) != 0 && (sie & mideleg) == 0)
            waiting_for_interrupt = false;

        else
            return false;
    }

    if (mstatus.MIE) {
        auto pending_interrupts = mip;
        pending_interrupts &= mie;

        auto delegated = pending_interrupts & mideleg;
        sip |= delegated;

        pending_interrupts &= ~delegated;

        bool handled = false;

        if (pending_interrupts) {
            for (Word cause = 31; cause > 32; cause--) {
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
                for (Word cause = 31; cause > 32; cause--) {
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
        return false;
    }

    auto [translated_address, translation_valid] = TranslateMemoryAddress(pc, false, true);
    if (!translation_valid) return false;
    
    auto instr = RVInstruction::FromUInt32(memory.ReadWord(translated_address));

    bool inc_pc = true;

    auto RS1 = [&]() {
        if (Is32BitMode())
            return static_cast<Long>(regs[instr.rs1].u32);
        
        return regs[instr.rs1].u64;
    };

    auto RS2 = [&]() {
        if (Is32BitMode())
            return static_cast<Long>(regs[instr.rs2].u32);
        
        return regs[instr.rs2].u64;
    };

    auto SignedRS1 = [&]() {
        if (Is32BitMode())
            return static_cast<SLong>(regs[instr.rs1].s32);
        
        return regs[instr.rs1].s64;
    };

    auto SignedRS2 = [&]() {
        if (Is32BitMode())
            return static_cast<SLong>(regs[instr.rs2].s32);
        
        return regs[instr.rs2].s64;
    };

    auto SetRD = [&](Long value) {
        if (instr.rd == 0) return;

        if (Is32BitMode())
            regs[instr.rd].u32 = static_cast<Word>(value);
        
        else
            regs[instr.rd].u64 = value;
    };

    auto SetSignedRD = [&](SLong value) {
        if (instr.rd == 0) return;
        
        if (Is32BitMode())
            regs[instr.rd].s32 = static_cast<SWord>(value);
        
        else
            regs[instr.rd].s64 = value;
    };

    switch (instr.type) {
        case Type::LUI:
            SetRD(instr.immediate);
            break;
        
        case Type::AUIPC:
            SetRD(pc + instr.immediate);
            break;
        
        case Type::JAL: {
            Long next_pc = pc + 4;
            pc += instr.immediate;
            SetRD(next_pc);
            break;
        }
        
        case Type::JALR: {
            Long next_pc = pc + 4;
            pc = (RS1() + instr.immediate) & 0xfffffffffffffffe;
            SetRD(next_pc);
            break;
        }
        
        case Type::BEQ: {
            if (RS1() == RS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::BNE: {
            if (RS1() != RS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::BLT: {
            if (SignedRS1() < SignedRS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::BGE: {
            if (SignedRS1() >= SignedRS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::BLTU: {
            if (RS1() < RS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::BGEU: {
            if (RS1() >= RS2())
                pc += instr.immediate;
            
            else
                pc += 4;
            
            break;
        }
        
        case Type::LB: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(SignExtendUnsigned(memory.ReadByte(translated_address), 7));
            break;
        }
        
        case Type::LH: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(SignExtendUnsigned(memory.ReadHalf(translated_address), 15));
            break;
        }
        
        case Type::LW: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;
            
            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(SignExtendUnsigned(memory.ReadWord(translated_address), 31));
            break;
        }

        case Type::LBU: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(static_cast<Long>(memory.ReadByte(translated_address)));
            break;
        }
        
        case Type::LHU: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(static_cast<Long>(memory.ReadHalf(translated_address)));
            break;
        }
        
        case Type::SB: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xfffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
            memory.WriteByte(translated_address, static_cast<uint8_t>(RS2()));
            break;
        }
        
        case Type::SH: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
            memory.WriteHalf(translated_address, static_cast<uint16_t>(RS2()));
            break;
        }
        
        case Type::SW: {
            Long addr = regs[instr.rs1].u64 + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
            memory.WriteWord(translated_address, RS2());
            break;
        }
        
        case Type::ADDI:
            SetRD(RS1() + instr.immediate);
            break;
        
        case Type::SLTI:
            SetRD(SignedRS1() < instr.s_immediate ? 1 : 0);
            break;
        
        case Type::SLTIU:
            SetRD(RS1() < instr.immediate ? 1 : 0);
            break;
        
        case Type::XORI:
            SetRD(RS1() ^ instr.immediate);
            break;
        
        case Type::ORI:
            SetRD(RS1() | instr.immediate);
            break;
        
        case Type::ANDI:
            SetRD(RS1() & instr.immediate);
            break;
        
        case Type::SLLI: {
            auto amount = instr.immediate & 0b111111;
            SetRD(RS1() << amount);
            break;
        }
        
        case Type::SRLI: {
            auto amount = instr.immediate & 0b111111;
            SetRD(RS1() >> amount);
            break;
        }
        
        case Type::SRAI: {
            auto amount = instr.immediate & 0b111111;
            auto value = RS1() >> amount;

            if (Is32BitMode() && (RS1() & (1ULL << 31)) && (amount != 0))
                value |= -1ULL << (32 - amount);
            else if (!Is32BitMode() && (RS1() & (1ULL << 63)) && (amount != 0))
                value |= -1ULL << (64 - amount);

            SetRD(value);
            break;
        }
        
        case Type::ADD:
            SetRD(RS1() + RS2());
            break;
        
        case Type::SUB:
            SetRD(RS1() - RS2());
            break;
        
        case Type::SLL: {
            auto amount = RS2() & 0x3f;
            SetRD(RS1() << amount);
            break;
        }
        
        case Type::SLT:
            SetRD(SignedRS1() < SignedRS2() ? 1 : 0);
            break;
        
        case Type::SLTU:
            SetRD(RS1() < RS2() ? 1 : 0);
            break;
        
        case Type::XOR:
            SetRD(RS1() ^ RS2());
            break;
        
        case Type::SRL: {
            auto amount = RS2() & 0x3f;
            SetRD(RS1() >> amount);
            break;
        }
        
        case Type::SRA: {
            auto amount = RS2() & 0x3f;
            auto value = RS1() >> amount;

            if (Is32BitMode() && (RS1() & (1ULL << 31)) && (amount != 0))
                value |= -1ULL << (32 - amount);
            else if (!Is32BitMode() && (RS1() & (1ULL << 63)) && (amount != 0))
                value |= -1ULL << (64 - amount);

            SetRD(value);
            break;
        }
        
        case Type::OR:
            SetRD(RS1() | RS2());
            break;
        
        case Type::AND:
            SetRD(RS1() & RS2());
            break;
        
        case Type::FENCE:
            break;
        
        case Type::ECALL:
            switch (privilege_level) {
                case PrivilegeLevel::Machine: {
                    Long value = regs[REG_A0].u64;
                    if (Is32BitMode()) value &= 0xffffffff;

                    if (!ecall_handlers.contains(value))
                        EmptyECallHandler(csrs[CSR_MHARTID], Is32BitMode(), memory, regs, fregs);
                    
                    else
                        ecall_handlers[value](csrs[CSR_MHARTID], Is32BitMode(), memory, regs, fregs);
                    break;
                }
                
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

        case Type::LWU: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long addr = regs[instr.rs1].u64 + instr.immediate;
            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(memory.ReadWord(translated_address));
            break;
        }

        case Type::LD: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long addr = regs[instr.rs1].u64 + instr.immediate;
            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;
            SetRD(memory.ReadLong(translated_address));
            break;
        }

        case Type::SD: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long addr = regs[instr.rs1].u64 + instr.immediate;
            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
            memory.WriteLong(translated_address, regs[instr.rs2].u64);
            break;
        };

        case Type::ADDIW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long val = regs[instr.rs1].u64 + instr.immediate;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SLLIW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = instr.immediate & 0b111111;
            Long val = regs[instr.rs1].u64 << shift;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SRLIW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = instr.immediate & 0b111111;
            Long val = regs[instr.rs1].u64;
            val &= 0xffffffff;
            val >>= shift;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SRAIW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = instr.immediate & 0b111111;
            Long val = (regs[instr.rs1].u64 & 0xffffffff);
            val >>= shift;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::ADDW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long val = regs[instr.rs1].u64 + regs[instr.rs2].u64;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SUBW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long val = regs[instr.rs1].u64 - regs[instr.rs2].u64;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SLLW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = regs[instr.rs2].u64 & 0b111111;
            Long val = regs[instr.rs1].u64 << shift;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SRLW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = regs[instr.rs2].u64 & 0b111111;
            Long val = regs[instr.rs1].u64 >> shift;
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::SRAW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            auto shift = regs[instr.rs2].u64 & 0b111111;
            Long val = regs[instr.rs1].u64 >> shift;
            if (regs[instr.rs1].u64 & (1ULL << 63))
                val |= -1ULL << (64 - shift);
            
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);
            regs[instr.rd].u64 = val;
            break;
        }

        case Type::CSRRW: {
            auto value = RS1();

            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            WriteCSR(instr.immediate, value);
            break;
        }
        
        case Type::CSRRS: {
            auto value = RS1();

            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            if (instr.rs1 != REG_ZERO)
                WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);
            
            break;
        }
        
        case Type::CSRRC: {
            auto value = RS1();
            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            if (instr.rs1 != REG_ZERO)
                WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
            
            break;
        }
        
        case Type::CSRRWI: {
            auto value = instr.rs1;
            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            WriteCSR(instr.immediate, value);
            break;
        }
        
        case Type::CSRRSI: {
            auto value = instr.rs1;
            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) | value);
            break;
        }
        
        case Type::CSRRCI: {
            auto value = instr.rs1;
            if (instr.rd != REG_ZERO)
                SetRD(ReadCSR(instr.immediate));
            
            WriteCSR(instr.immediate, ReadCSR(instr.immediate, true) & ~value);
            break;
        }
        
        case Type::MUL: {
            auto lhs = SignedRS1();
            auto rhs = SignedRS2();
            SetSignedRD(lhs * rhs);
            break;
        }
        
        case Type::MULH: {
            auto lhs = SignedRS1();
            auto rhs = SignedRS2();

            if (Is32BitMode()) 
                SetSignedRD((lhs * rhs) >> 32);

            else {
                __int128_t o_lhs = lhs;
                __int128_t o_rhs = rhs;
                SetSignedRD(static_cast<SLong>((o_lhs * o_rhs) >> 64));
            }
            break;
        }
        
        case Type::MULHSU: {
            auto lhs = SignedRS1();
            auto rhs = RS2();

            if (Is32BitMode())
                SetSignedRD((lhs * rhs) >> 32);

            else {
                __int128_t o_lhs = lhs;
                __uint128_t o_rhs = rhs;
                SetSignedRD(static_cast<SLong>((o_lhs * o_rhs) >> 64));
            }
            break;
        }
        
        case Type::MULHU: {
            auto lhs = RS1();
            auto rhs = RS2();

            if (Is32BitMode())
                SetRD((lhs * rhs) >> 32);
            
            else {
                __uint128_t o_lhs = lhs;
                __uint128_t o_rhs = rhs;
                SetRD((o_lhs * o_rhs) >> 64);
            }
            
            break;
        }

        case Type::DIV: {
            auto lhs = SignedRS1();
            auto rhs = SignedRS2();

            if (rhs == 0)
                SetRD(-1ULL);
            
            else
                SetSignedRD(lhs / rhs);
            
            break;

        }
        
        case Type::DIVU: {
            auto lhs = RS1();
            auto rhs = RS2();

            if (rhs == 0)
                SetRD(-1ULL);
            
            else
                SetRD(lhs / rhs);

            break;
        }
        
        case Type::REM: {
            auto lhs = SignedRS1();
            auto rhs = SignedRS2();

            if (rhs == 0)
                SetRD(0);
            
            else
                SetSignedRD(lhs % rhs);

            break;
        }
        
        case Type::REMU: {
            auto lhs = RS1();
            auto rhs = RS2();

            if (rhs == 0)
                SetRD(0);
            
            else
                SetRD(lhs % rhs);

            break;
        }

        case Type::MULW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            SLong lhs = regs[instr.rs1].s32;
            SLong rhs = regs[instr.rs2].s32;

            auto val = lhs * rhs;
            val &= 0xffffffff;
            val = SignExtendSigned(val, 31);

            regs[instr.rd].s64 = val;
            break;
        }

        case Type::DIVW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            SLong lhs = regs[instr.rs1].s32;
            SLong rhs = regs[instr.rs2].s32;

            SLong val;
            if (rhs == 0)
                val = -1LL;
            
            else
                val = lhs / rhs;
            
            val &= 0xffffffff;
            val = SignExtendSigned(val, 31);

            regs[instr.rd].s64 = val;
            break;
        }

        case Type::DIVUW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long lhs = regs[instr.rs1].u32;
            Long rhs = regs[instr.rs2].u32;

            Long val;
            if (rhs == 0)
                val = -1ULL;
            
            else
                val = lhs / rhs;
            
            val &= 0xffffffff;
            val = SignExtendUnsigned(val, 31);

            regs[instr.rd].s64 = val;
            break;
        }

        case Type::REMW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            SLong lhs = regs[instr.rs1].s32;
            SLong rhs = regs[instr.rs2].s32;

            SLong val;
            if (rhs == 0)
                val = -1LL;
            
            else
                val = lhs % rhs;
            
            val &= 0xffffffff;
            val = SignExtendSigned(val, 31);

            regs[instr.rd].s64 = val;
            break;
        }

        case Type::REMUW: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            Long lhs = regs[instr.rs1].u32;
            Long rhs = regs[instr.rs2].u32;

            Long val;
            if (rhs == 0)
                val = -1ULL;
            
            else
                val = lhs % rhs;
            
            val &= 0xffffffff;

            regs[instr.rd].u64 = val;
            break;
        }
        
        case Type::LR_W: {
            if (instr.rs2 != 0) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false);
            if (!translation_valid) return false;

            SetRD(memory.ReadWordReserved(translated_address, csrs[CSR_MHARTID]));
            break;
        }
        
        case Type::SC_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), true, false);
            if (!translation_valid) return false;
            
            if (memory.WriteWordConditional(translated_address, RS2(), csrs[CSR_MHARTID]))
                SetRD(0);
            
            else
                SetRD(1);
            
            break;
        }
        
        case Type::AMOSWAP_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicSwapW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOADD_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicAddW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOXOR_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicXorW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOAND_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicAndW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOOR_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;
            
            SetRD(memory.AtomicOrW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOMIN_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMinW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOMAX_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMaxW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOMINU_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMinUW(translated_address, RS2()));
            break;
        }
        
        case Type::AMOMAXU_W: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMaxUW(translated_address, RS2()));
            break;
        }

        case Type::LR_D: {
            if (instr.rs2 != 0) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false);
            if (!translation_valid) return false;

            SetRD(memory.ReadLongReserved(translated_address, csrs[CSR_MHARTID]));
            break;
        }

        case Type::SC_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), true, false);
            if (!translation_valid) return false;

            if (memory.WriteLongConditional(translated_address, RS2(), csrs[CSR_MHARTID]))
                SetRD(0);
            
            else
                SetRD(1);
            
            break;
        }

        case Type::AMOSWAP_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicSwapL(translated_address, RS2()));
            break;
        }

        case Type::AMOADD_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicAddL(translated_address, RS2()));
            break;
        }

        case Type::AMOXOR_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicXorL(translated_address, RS2()));
            break;
        }

        case Type::AMOAND_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicAndL(translated_address, RS2()));
            break;
        }

        case Type::AMOOR_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicOrL(translated_address, RS2()));
            break;
        }

        case Type::AMOMIN_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMinL(translated_address, RS2()));
            break;
        }

        case Type::AMOMAX_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMaxL(translated_address, RS2()));
            break;
        }

        case Type::AMOMINU_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMinUL(translated_address, RS2()));
            break;
        }

        case Type::AMOMAXU_D: {
            auto [translated_address, translation_valid] = TranslateMemoryAddress(RS1(), false, false, true);
            if (!translation_valid) return false;

            SetRD(memory.AtomicMaxUL(translated_address, RS2()));
            break;
        }
        
        case Type::FLW: {
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            auto addr = RS1() + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;

            fregs[instr.rd] = ToFloat(translated_address);
            break;
        }
        
        case Type::FSW: {
            auto addr = RS1() + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
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
            
            Word result;

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
                SWord val = fregs[instr.rs1].f;
                if (val != fregs[instr.rs1].f)
                    SetFloatFlags(false, false, false, false, true);

                result = AsUnsigned32(static_cast<SWord>(val));
            }

            regs[instr.rd].u64 = SignExtendUnsigned(result, 31);
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

            Word result;

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
                Word val = fregs[instr.rs1].f;
                if (val != fregs[instr.rs1].f)
                    SetFloatFlags(false, false, false, false, true);
                
                result = val;
            }

            regs[instr.rd].u64 = SignExtendUnsigned(result, 31);
            break;
        }
        
        case Type::FMV_X_W:
            regs[instr.rd].u32 = ToUInt32(fregs[instr.rs1]);
            regs[instr.rd].u64 = SignExtendUnsigned(regs[instr.rd].u64, 31);
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
                regs[instr.rd].u64 = 0;
            
            else
                regs[instr.rd].u64 = lhs.f == rhs.f ? 1 : 0;
            
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
                regs[instr.rd].u64 = 0;
            }
            else
                regs[instr.rd].u64 = lhs.f < rhs.f ? 1 : 0;
            
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
                regs[instr.rd].u64 = 0;
            }
            else
                regs[instr.rd].u64 = lhs.f <= rhs.f ? 1 : 0;
            
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
            
            Word result = 0;
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

            regs[instr.rd].u32 = result;
            regs[instr.rd].is_u64 = 0;
            break;
        }
        
        case Type::FCVT_S_W: {
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            auto val = AsSigned32(regs[instr.rs1].u32);

            fregs[instr.rd].f = val;
            if (fregs[instr.rd].f != val)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        }
        
        case Type::FCVT_S_WU:
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            fregs[instr.rd].f = regs[instr.rs1].u32;
            if (fregs[instr.rd].f != regs[instr.rs1].u32)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        
        case Type::FMV_W_X:
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            fregs[instr.rd].u64 = 0;
            fregs[instr.rd].u32 = regs[instr.rs1].u32;
            break;
        
        case Type::FCVT_L_S: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            if (!ChangeRoundingMode(instr.rm)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            bool is_inf, is_nan, is_qnan;
            ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);

            Long result;

            if (is_inf) {
                if (fregs[instr.rs1].f < 0) result = -1ULL;
                else result = 0x7fffffffffffffff;
                SetFloatFlags(false, false, false, false, true);
            }
            else if (is_nan || is_qnan) {
                result = 0x7fffffffffffffff;
                SetFloatFlags(false, false, false, false, true);
            }
            else {
                SLong val = fregs[instr.rs1].f;
                if (val != fregs[instr.rs1].f)
                    SetFloatFlags(false, false, false, false, true);
                
                result = AsUnsigned64(val);
            }

            regs[instr.rd].u64 = result;
            break;
        }

        case Type::FCVT_LU_S: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            if (!ChangeRoundingMode(instr.rm)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            bool is_inf, is_nan, is_qnan;
            ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);
            
            Long result;

            if (is_inf) {
                if (fregs[instr.rs1].f < 0) result = 0;
                else result = -1ULL;
                SetFloatFlags(false, false, false, false, true);
            }
            else if (is_nan || is_qnan) {
                result = -1ULL;
                SetFloatFlags(false, false, false, false, true);
            }
            else {
                Long val = fregs[instr.rs1].f;
                if (val != fregs[instr.rs1].f)
                    SetFloatFlags(false, false, false, false, true);
                
                result = val;
            }

            regs[instr.rd].u64 = result;
            break;
        }

        case Type::FCVT_S_L: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            SLong val = AsSigned64(regs[instr.rs1].u64);

            fregs[instr.rd].f = val;
            if (fregs[instr.rd].f != val)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        }

        case Type::FCVT_S_LU: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            Long val = regs[instr.rs1].u64;

            fregs[instr.rd].f = val;
            if (fregs[instr.rd].f != val)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        }
        
        case Type::FLD: {
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            auto addr = RS1() + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            if (!translation_valid) return false;

            Long val = memory.ReadWord(translated_address);
            val |= static_cast<Long>(memory.ReadWord(translated_address + 4)) << 32;
            fregs[instr.rd] = ToDouble(val);
            break;
        }
        
        case Type::FSD: {
            auto addr = RS1() + instr.immediate;
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, true, false);
            if (!translation_valid) return false;
            
            auto val = ToUInt64(fregs[instr.rs2]);
            memory.WriteWord(translated_address, static_cast<Word>(val));
            memory.WriteWord(translated_address + 4, static_cast<Word>(val >> 32));
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
                regs[instr.rd].u64 = 0;
            
            else
                regs[instr.rd].u64 = lhs.d == rhs.d ? 1 : 0;
            
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
                regs[instr.rd].u64 = 0;
            }
            else
                regs[instr.rd].u64 = lhs.d < rhs.d ? 1 : 0;
            
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
                regs[instr.rd].u64 = 0;
            }
            else
                regs[instr.rd].u64 = lhs.d <= rhs.d ? 1 : 0;
            
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
            
            Word result = 0;
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

            regs[instr.rd].u32 = result;
            regs[instr.rd].is_u64 = 0;
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
            
            Word result;

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
                SWord val = fregs[instr.rs1].d;
                if (val != fregs[instr.rs1].d)
                    SetFloatFlags(false, false, false, false, true);

                result = AsUnsigned32(static_cast<SWord>(val));
            }

            regs[instr.rd].u32 = result;
            regs[instr.rd].is_u64 = 0;
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

            Word result;

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
                Word val = fregs[instr.rs1].d;
                if (val != fregs[instr.rs1].d)
                    SetFloatFlags(false, false, false, false, true);
                
                result = val;
            }

            regs[instr.rd].u32 = result;
            regs[instr.rd].is_u64 = 0;
            break;
        }
        
        case Type::FCVT_D_W: {
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            auto val = AsSigned32(regs[instr.rs1].u32);

            fregs[instr.rd].d = val;
            if (fregs[instr.rd].d != val)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        }
        
        case Type::FCVT_D_WU:
            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;
            
            fregs[instr.rd].d = regs[instr.rs1].u32;
            if (fregs[instr.rd].d != regs[instr.rs1].u32)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        
        case Type::FCVT_L_D: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            if (!ChangeRoundingMode(instr.rm)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            bool is_inf, is_nan, is_qnan;
            ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);

            Long result;

            if (is_inf) {
                if (fregs[instr.rs1].d < 0) result = -1ULL;
                else result = 0x7fffffffffffffff;
                SetFloatFlags(false, false, false, false, true);
            }
            else if (is_nan || is_qnan) {
                result = 0x7fffffffffffffff;
                SetFloatFlags(false, false, false, false, true);
            }
            else {
                SLong val = fregs[instr.rs1].d;
                if (val != fregs[instr.rs1].d)
                    SetFloatFlags(false, false, false, false, true);
                
                result = AsUnsigned64(val);
            }

            regs[instr.rd].u64 = result;
            break;
        }

        case Type::FCVT_LU_D: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }
            
            if (!ChangeRoundingMode(instr.rm)) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            bool is_inf, is_nan, is_qnan;
            ClassF64(fregs[instr.rs1], &is_inf, &is_nan, &is_qnan, nullptr, nullptr, nullptr);

            Long result;

            if (is_inf) {
                if (fregs[instr.rs1].d < 0) result = 0;
                else result = -1ULL;
                SetFloatFlags(false, false, false, false, true);
            }
            else if (is_nan || is_qnan) {
                result = -1ULL;
                SetFloatFlags(false, false, false, false, true);
            }
            else {
                Long val = fregs[instr.rs1].d;
                if (val != fregs[instr.rs1].d)
                    SetFloatFlags(false, false, false, false, true);
                
                result = val;
            }

            regs[instr.rd].u64 = result;
            break;
        }

        case Type::FMV_X_D:
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            regs[instr.rd].u64 = ToUInt64(fregs[instr.rs1]);
            break;
        
        case Type::FCVT_D_L: {
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            auto val = AsSigned64(regs[instr.rs1].u64);

            fregs[instr.rd].d = val;
            if (fregs[instr.rd].d != val)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        }
        
        case Type::FCVT_D_LU:
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;
            
            fregs[instr.rd].d = regs[instr.rs1].u64;
            if (fregs[instr.rd].d != regs[instr.rs1].u64)
                SetFloatFlags(true, false, false, false, false);
            
            break;
        
        case Type::FMV_D_X:
            if (Is32BitMode()) {
                RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION);
                inc_pc = false;
                break;
            }

            mstatus.FS = FS_DIRTY;
            sstatus.FS = FS_DIRTY;

            fregs[instr.rd].u64 = regs[instr.rs1].u64;
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
            
            return false;
        
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
            return false;
        
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
            auto addr = RS1();
            if (Is32BitMode()) addr &= 0xffffffff;

            auto [translated_address, translation_valid] = TranslateMemoryAddress(addr, false, false);
            regs[instr.rd].u64 = translated_address;
            break;
        }
        
        case Type::CUST_MTRAP:
            pc += 4;

            switch (regs[instr.rs2].u64 & 0b11) {
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

            if (Is32BitMode())
                RaiseMachineTrap(regs[instr.rs1].u32);

            else
                RaiseMachineTrap(regs[instr.rs1].u64);
            
            return false;
        
        case Type::CUST_STRAP:
            pc += 4;

            switch (regs[instr.rs2].u64 & 0b11) {
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
            
            if (Is32BitMode())
                RaiseSupervisorTrap(regs[instr.rs1].u32);
            
            else
                RaiseSupervisorTrap(regs[instr.rs1].u64);
            
            return false;

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

    if (IsBreakPoint(pc)) return true;
    

    return false;
}

bool VirtualMachine::Step(Long steps) {
    try {
        for (Long i = 0; i < steps; i++)
            if (SingleStep())
                return true;
    }
    catch (std::exception& e) {
        VMException vm_e;
        vm_e.hart_id = csrs[CSR_MHARTID];
        
        GetSnapshot(vm_e.regs, vm_e.fregs, vm_e.virtual_pc);

        GetCSRSnapshot(vm_e.csrs);

        Long pc_address = vm_e.virtual_pc;

        if (IsUsingVirtualMemory()) {
            auto [addr, valid] = TranslateMemoryAddress(vm_e.virtual_pc, false, true, false);
            vm_e.physical_pc = std::make_pair(addr, true);

            pc_address = addr;
        }
        else
            vm_e.physical_pc = std::make_pair(0, false);
        
        vm_e.instruction = memory.PeekWord(pc_address);
        vm_e.original_exception = e.what();

        throw vm_e;
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

void VirtualMachine::GetSnapshot(std::array<Reg, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, Long& pc) {
    registers = regs;
    fregisters = fregs;
    pc = this->pc;
}

void VirtualMachine::GetCSRSnapshot(std::unordered_map<Long, Long>& csrs) const {
    csrs = this->csrs;
    csrs[CSR_MCYCLE] = cycles;
    csrs[CSR_CYCLE] = cycles;

    csrs[CSR_TIME] = csr_mapped_memory->time;

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
    Word total_ticks = 0;

    for (size_t i = 0; i < history_delta.size(); i++) {
        total_time += history_delta[i];
        total_ticks += history_tick[i];
    }

    return total_ticks / total_time;
}

bool VirtualMachine::IsBreakPoint(Address addr) {
    if (break_points.contains(addr)) return true;

    auto word = memory.PeekWord(addr);

    if (!word.second)
        return false;

    RVInstruction instr = RVInstruction::FromUInt32(word.first);

    if (privilege_level == PrivilegeLevel::User)
        return false;

    return instr.type == RVInstruction::Type::EBREAK;
}

void VirtualMachine::UpdateTime(double delta_time) {
    history_delta.push_back(delta_time);
    history_tick.push_back(ticks);
    ticks = 0;
    
    csr_mapped_memory->time += static_cast<Long>(delta_time * CSRMappedMemory::TICKS_PER_SECOND);
    if (csr_mapped_memory->time >= csr_mapped_memory->time_cmp)
        RaiseInterrupt(INTERRUPT_MACHINE_TIMER);

    while (history_delta.size() > MAX_HISTORY) {
        history_delta.erase(history_delta.begin());
        history_tick.erase(history_tick.begin());
    }
}

void VirtualMachine::EmptyECallHandler(Hart hart, bool is_32_bit_mode, Memory&, std::array<Reg, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&) {
    if (is_32_bit_mode)
        throw std::runtime_error(std::format("Hart {} called unknown ECall handler: {}", hart, regs[REG_A0].s32));
    else
        throw std::runtime_error(std::format("Hart {} called unknown ECall handler: {}", hart, regs[REG_A0].s64));
}

std::unordered_map<Long, VirtualMachine::ECallHandler> VirtualMachine::ecall_handlers;