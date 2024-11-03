#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

#include "Memory.hpp"
#include "Float.hpp"
#include "Expected.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <format>
#include <stdexcept>
#include <unordered_map>

class VirtualMachine {
    friend class TLBEntry;

public:
    static constexpr size_t REGISTER_COUNT = 32;

    union Reg {
        struct {
            Word u32;
            Word is_u64;
        };
        struct {
            SWord s32;
            SWord _unused;
        };
        Long u64;
        SLong s64;
    };

    struct VMException : public std::exception {
        Hart hart_id;
        std::array<Reg, REGISTER_COUNT> regs;
        std::array<Float, REGISTER_COUNT> fregs;
        std::unordered_map<Long, Long> csrs;
        Long virtual_pc;
        std::pair<Long, bool> physical_pc;
        std::pair<Word, bool> instruction;
        std::string original_exception;

        std::string dump() const;

        inline const char* what() {
            return original_exception.c_str();
        }
    };

private:
    static constexpr Byte MACHINE_MODE = 0b11;
    static constexpr Byte SUPERVISOR_MODE = 0b01;
    static constexpr Byte USER_MODE = 0b00;

    static constexpr Long ISA_BITS_MASK = 0b11 << 30;
    static constexpr Long ISA_64_BITS = 1ULL << 63;

    static constexpr Long ISA_A = 1<<0;
    static constexpr Long ISA_D = 1<<3;
    static constexpr Long ISA_F = 1<<5;
    static constexpr Long ISA_I = 1<<8;
    static constexpr Long ISA_M = 1<<12;
    static constexpr Long ISA_S = 1<<18;
    static constexpr Long ISA_U = 1<<20;

    std::array<Reg, REGISTER_COUNT> regs;
    std::array<Float, REGISTER_COUNT> fregs;

    std::unordered_map<Long, Long> csrs;

    bool CSRPrivilegeCheck(Long csr);
    Long ReadCSR(Long csr, bool is_internal_read = false);
    void WriteCSR(Long csr, Long value);

    static const int default_rounding_mode;
    bool ChangeRoundingMode(Byte rm = 0xff);
    bool CheckFloatErrors();

public:
    static constexpr Half CSR_FFLAGS = 0x001;
    static constexpr Half CSR_FRM = 0x002;
    static constexpr Half CSR_FCSR = 0x003;

    static constexpr Long CSR_FCSR_NV = 0b10000;
    static constexpr Long CSR_FCSR_DZ = 0b1000;
    static constexpr Long CSR_FCSR_OF = 0b100;
    static constexpr Long CSR_FCSR_UF = 0b10;
    static constexpr Long CSR_FCSR_NX = 0b1;

    static constexpr Long CSR_FCSR_FLAGS = 0b11111;
    
    static constexpr Half CSR_CYCLE = 0xc00;
    static constexpr Half CSR_TIME = 0xc01;
    static constexpr Half CSR_INSTRET = 0xc02;
    
    static constexpr Half CSR_PERF_COUNTER_MAX = 32;
    static constexpr Half CSR_HPMCOUNTER = 0xc03;

    static constexpr Half CSR_SSTATUS = 0x100;
    static constexpr Half CSR_SIE = 0x104;
    static constexpr Half CSR_STVEC = 0x105;
    static constexpr Half CSR_SCOUNTEREN = 0x106;
    static constexpr Half CSR_SENVCFG = 0x10a;
    static constexpr Half CSR_SSCRATCH = 0x140;
    static constexpr Half CSR_SEPC = 0x141;
    static constexpr Half CSR_SCAUSE = 0x142;
    static constexpr Half CSR_STVAL = 0x143;
    static constexpr Half CSR_SIP = 0x144;
    static constexpr Half CSR_SATP = 0x180;

    static constexpr Half CSR_MVENDORID = 0xf11;
    static constexpr Half CSR_MARCHID = 0xf12;
    static constexpr Half CSR_MIMPID = 0xf13;
    static constexpr Half CSR_MHARTID = 0xf14;
    static constexpr Half CSR_MCONFIGPTR = 0xf15;
    static constexpr Half CSR_MSTATUS = 0x300;
    static constexpr Half CSR_MISA = 0x301;
    static constexpr Half CSR_MEDELEG = 0x302;
    static constexpr Half CSR_MIDELEG = 0x303;
    static constexpr Half CSR_MIE = 0x304;
    static constexpr Half CSR_MTVEC = 0x305;
    static constexpr Half CSR_MCOUNTEREN = 0x306;
    static constexpr Half CSR_MSCRATCH = 0x340;
    static constexpr Half CSR_MEPC = 0x341;
    static constexpr Half CSR_MCAUSE = 0x342;
    static constexpr Half CSR_MTVAL = 0x343;
    static constexpr Half CSR_MIP = 0x344;
    static constexpr Half CSR_MTINST = 0x34a;
    static constexpr Half CSR_MTVAL2 = 0x34b;
    static constexpr Half CSR_MENVCFG = 0x30a;
    static constexpr Half CSR_MSECCFG = 0x747;
    
    static constexpr Half CSR_PMPCFG_MAX = 16;
    static constexpr Half CSR_PMPCFG0 = 0x3a0;

    static constexpr Half CSR_PMPADDR_MAX = 64;
    static constexpr Half CSR_PMPADDR0 = 0x3b0;

    static constexpr Half CSR_MCYCLE = 0xb00;
    static constexpr Half CSR_MINSTRET = 0xb02;
    static constexpr Half CSR_MHPMCOUNTER3 = 0xb03;
    static constexpr Half CSR_MCYCLEH = 0xb80;
    static constexpr Half CSR_MCOUNTINHIBIT = 0x320;

    static constexpr Half CSR_PERFORMANCE_EVENT_MAX = 32;
    static constexpr Half CSR_MHPMEVENT3 = 0x323;

    static constexpr Long MSTATUS_WRITABLE_BITS = 0b00000000000011100111100110101010;

    union MStatus {
        struct {
            Long _unused0 : 1;
            Long SIE : 1;
            Long _unused1 : 1;
            Long MIE : 1;
            Long _unused2 : 1;
            Long SPIE : 1;
            Long _unused3 : 1;
            Long MPIE : 1;
            Long SPP : 1;
            Long _unused4 : 2;
            Long MPP : 2;
            Long FS : 2;
            Long _unused5 : 2;
            Long MPRIV : 1;
            Long SUM : 1;
            Long MXR : 1;
            Long _unused6 : 1;
            Long _unused7 : 1;
            Long _unused8 : 1;
            Long _unused9 : 8;
            Long SD : 1;
        };
        Long raw;
    };

    static constexpr Byte FS_OFF = 0;
    static constexpr Byte FS_INITIAL = 1;
    static constexpr Byte FS_CLEAN = 2;
    static constexpr Byte FS_DIRTY = 3;

    inline MStatus ReadMStatus() const {
        MStatus mstatus;
        mstatus.raw = csrs.at(CSR_MSTATUS);
        return mstatus;
    }

    inline void WriteMStatus(MStatus mstatus) {
        csrs[CSR_MSTATUS] = mstatus.raw;
    }

    inline bool Is32BitMode() const {
        return false;
    }

    static constexpr Long SSTATUS_WRITABLE_BITS = 0b00000000000011000110000100100010;

    union SStatus {
        struct {
            Long _unused0 : 1;
            Long SIE : 1;
            Long _unused1 : 3;
            Long SPIE : 1;
            Long _unused2 : 1;
            Long _unused3 : 1;
            Long SPP : 1;
            Long _unused4 : 2;
            Long _unused5 : 2;
            Long FS : 2;
            Long _unused6 : 2;
            Long _unused7 : 1;
            Long SUM : 1;
            Long MXR : 1;
            Long _unused8 : 11;
            Long SD : 1;
        };
        Long raw;
    };
    inline SStatus ReadSStatus() const {
        SStatus sstatus;
        sstatus.raw = csrs.at(CSR_SSTATUS);
        return sstatus;
    }

    inline void WriteSStatus(SStatus sstatus) {
        csrs[CSR_SSTATUS] = sstatus.raw;
    }

    enum class PrivilegeLevel {
        Machine,
        Supervisor,
        User
    };
    PrivilegeLevel privilege_level;

    static constexpr size_t REG_ZERO = 0;
    static constexpr size_t REG_RA = 1;
    static constexpr size_t REG_SP = 2;
    static constexpr size_t REG_GP = 3;
    static constexpr size_t REG_TP = 4;
    static constexpr size_t REG_T0 = 5;
    static constexpr size_t REG_T1 = 6;
    static constexpr size_t REG_T2 = 7;
    static constexpr size_t REG_S0 = 8;
    static constexpr size_t REG_FP = 8;
    static constexpr size_t REG_S1 = 9;
    static constexpr size_t REG_A0 = 10;
    static constexpr size_t REG_A1 = 11;
    static constexpr size_t REG_A2 = 12;
    static constexpr size_t REG_A3 = 13;
    static constexpr size_t REG_A4 = 14;
    static constexpr size_t REG_A5 = 15;
    static constexpr size_t REG_A6 = 16;
    static constexpr size_t REG_A7 = 17;
    static constexpr size_t REG_S2 = 18;
    static constexpr size_t REG_S3 = 19;
    static constexpr size_t REG_S4 = 20;
    static constexpr size_t REG_S5 = 21;
    static constexpr size_t REG_S6 = 22;
    static constexpr size_t REG_S7 = 23;
    static constexpr size_t REG_S8 = 24;
    static constexpr size_t REG_S9 = 25;
    static constexpr size_t REG_S10 = 26;
    static constexpr size_t REG_S11 = 27;
    static constexpr size_t REG_T3 = 28;
    static constexpr size_t REG_T4 = 29;
    static constexpr size_t REG_T5 = 30;
    static constexpr size_t REG_T6 = 31;

    static constexpr size_t REG_FT0 = 0;
    static constexpr size_t REG_FT1 = 1;
    static constexpr size_t REG_FT2 = 2;
    static constexpr size_t REG_FT3 = 3;
    static constexpr size_t REG_FT4 = 4;
    static constexpr size_t REG_FT5 = 5;
    static constexpr size_t REG_FT6 = 6;
    static constexpr size_t REG_FT7 = 7;
    static constexpr size_t REG_FS0 = 8;
    static constexpr size_t REG_FS1 = 9;
    static constexpr size_t REG_FA0 = 10;
    static constexpr size_t REG_FA1 = 11;
    static constexpr size_t REG_FA2 = 12;
    static constexpr size_t REG_FA3 = 13;
    static constexpr size_t REG_FA4 = 14;
    static constexpr size_t REG_FA5 = 15;
    static constexpr size_t REG_FA6 = 16;
    static constexpr size_t REG_FA7 = 17;
    static constexpr size_t REG_FS2 = 18;
    static constexpr size_t REG_FS3 = 19;
    static constexpr size_t REG_FS4 = 20;
    static constexpr size_t REG_FS5 = 21;
    static constexpr size_t REG_FS6 = 22;
    static constexpr size_t REG_FS7 = 23;
    static constexpr size_t REG_FS8 = 24;
    static constexpr size_t REG_FS9 = 25;
    static constexpr size_t REG_FS10 = 26;
    static constexpr size_t REG_FS11 = 27;
    static constexpr size_t REG_FT8 = 28;
    static constexpr size_t REG_FT9 = 29;
    static constexpr size_t REG_FT10 = 30;
    static constexpr size_t REG_FT11 = 31;

    union TLBEntry {
        struct {
            Long V : 1;
            Long R : 1;
            Long W : 1;
            Long X : 1;
            Long U : 1;
            Long G : 1;
            Long A : 1;
            Long D : 1;
            Long RSW : 2;
            Long PPN_0 : 10;
            Long PPN_1 : 12;
        };
        struct {
            Long _unused : 10;
            Long PPN : 22;
        };
        Long raw;

        inline bool IsLeaf() const {
            return X || W || R;
        }
    };

    static constexpr Long INTERRUPT_SUPERVISOR_SOFTWARE = 0x1;
    static constexpr Long INTERRUPT_MACHINE_SOFTWARE = 0x3;
    static constexpr Long INTERRUPT_SUPERVISOR_TIMER = 0x5;
    static constexpr Long INTERRUPT_MACHINE_TIMER = 0x7;
    static constexpr Long INTERRUPT_SUPERVISOR_EXTERNAL = 0x9;
    static constexpr Long INTERRUPT_MACHINE_EXTERNAL = 0xa;

    void RaiseInterrupt(Long cause);

private:
    bool waiting_for_interrupt = false;

public:
    inline bool IsWaitingForInterrupt() const {
        return waiting_for_interrupt;
    }

private:
    static constexpr Long TRAP_INTERRUPT_BIT = (1ULL << 31);

    static constexpr Long EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED = 0x0;
    static constexpr Long EXCEPTION_INSTRUCTION_ADDRESS_FAULT = 0x1;
    static constexpr Long EXCEPTION_ILLEGAL_INSTRUCTION = 0x2;
    static constexpr Long EXCEPTION_BREAKPOINT = 0x3;
    static constexpr Long EXCEPTION_LOAD_ADDRESS_MISALIGNED = 0x4;
    static constexpr Long EXCEPTION_LOAD_ACCESS_FAULT = 0x5;
    static constexpr Long EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED = 0x6;
    static constexpr Long EXCEPTION_STORE_AMO_ACCESS_FAULT = 0x7;
    static constexpr Long EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE = 0x8;
    static constexpr Long EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE = 0x9;
    static constexpr Long EXCEPTION_ENVIRONMENT_CALL_FROM_M_MODE = 0xb;
    static constexpr Long EXCEPTION_INSTRUCTION_PAGE_FAULT = 0xc;
    static constexpr Long EXCEPTION_INSTRUCTION_LOAD_PAGE_FAULT = 0xd;
    static constexpr Long EXCEPTION_STORE_AMO_PAGE_FAULT = 0xf;

    void RaiseException(Long cause);

    static constexpr Long VALID_INTERRUPT_BITS = 0b0;

    Long mip = 0;
    Long mie = 0;
    Long mideleg = 0;
    Long sip = 0;
    Long sie = 0;

    void RaiseMachineTrap(Long cause);
    void RaiseSupervisorTrap(Long cause);
    
    MStatus mstatus;
    SStatus sstatus;

    struct TLBCacheEntry {
        Long tag : 63;
        Long super : 1;
        
        TLBEntry tlb_entry;

        TLBCacheEntry() : tag{0}, super{0} { tlb_entry.raw = 0; }
    };

    static constexpr size_t TLB_CACHE_SIZE = 16;
    
    template <typename Type>
    inline static consteval Type GetLog2(Type value) {
        static_assert(!std::is_signed_v<Type>);
        
        Long i = 0;
        if constexpr (sizeof(value) == sizeof(uint32_t)) {
            while ((1U << i) < value) i++;
        }
        
        else
            while ((1ULL << i) < value) i++;
        
        return i;
    }

public:
    std::pair<TLBEntry, bool> GetTLBLookup(Address phys_addr, bool bypass_cache = false, bool is_amo = false);

private:
    size_t tlb_cache_round_robin = 0;
    std::array<TLBCacheEntry, TLB_CACHE_SIZE> tlb_cache;

    Memory& memory;

    union SATP {
        struct {
            Long PPN : 22;
            Long ASID : 9;
            Long MODE : 1;
        };
        Long raw;
    };

    SATP satp;

public:
    inline bool IsUsingVirtualMemory() const {
        return (privilege_level != PrivilegeLevel::Machine) && (satp.MODE != 0);
    }

    inline Long GetPendingMachineInterrupts() const {
        return mip;
    }

    inline Long GetEnabledMachineInterrupts() const {
        return mie;
    }

    inline Long GetDelegatedMachineInterrupts() const {
        return mideleg;
    }

    inline Long GetPendingSupervisorInterrupts() const {
        return sip;
    }

    inline Long GetEnabledSupervisorInterrupts() const {
        return sie;
    }

private:
    std::pair<Address, bool> TranslateMemoryAddress(Address address, bool is_write, bool is_execute, bool is_amo = false);

    Long pc;
    Long cycles;

    bool running = false;
    bool paused = false;
    bool pause_on_break = false;
    bool pause_on_restart = false;
    std::string err = "";

    std::set<Long> break_points;

    Long ticks;
    std::vector<double> history_delta;
    std::vector<Long> history_tick;

    static constexpr size_t MAX_HISTORY = 15;

    void Setup();

    class CSRMappedMemory : public MemoryRegion {
    public:
        static constexpr Long TICKS_PER_SECOND = 32768;
        Long time = 0;
        Long time_cmp = -1ULL;

        CSRMappedMemory() : MemoryRegion{TYPE_MAPPED_CSRS, 0,  0Xf00, 0x100, true, true} {}

        Word ReadWord(Address address) const override {
            address >>= 2;

            switch (address) {
                case 0:
                    return static_cast<Word>(time);
                
                case 1:
                    return static_cast<Word>(time >> 32);
                
                case 2:
                    return static_cast<Word>(time_cmp);
                
                case 3:
                    return static_cast<Word>(time_cmp >> 32);
            }

            return 0;
        }

        void Lock() const override {}
        void Unlock() const override {}
    };

    std::shared_ptr<CSRMappedMemory> csr_mapped_memory;

public:
    VirtualMachine(Memory& memory, Long starting_pc, Hart hart_id);
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&);
    ~VirtualMachine();
    
    inline void Start() { running = true; }
    inline void Restart(Long pc, Hart source_hart) {
        this->pc = pc;

        if (source_hart == csrs[CSR_MHARTID])
            this->pc -= 4;

        Setup();
        paused = pause_on_restart;
    }
    inline bool IsRunning() const { return running; }
    inline void Stop() { running = false; }

    inline void Pause() { paused = true; }
    inline bool IsPaused() const { return paused; }
    inline void Unpause() { paused = false; }

    inline void SetPauseOnBreak(bool pause_on_break) { this->pause_on_break = pause_on_break; }
    inline bool PauseOnBreak() const { return pause_on_break; }

    inline void SetPauseOnRestart(bool pause_on_restart) { this->pause_on_restart = pause_on_restart; }
    inline bool PauseOnRestart() const { return pause_on_restart; }

private:
    bool SingleStep();

public:
    bool Step(Long steps = 1000);
    void Run();

    inline void SetPC(Long pc) { this->pc = pc; }

    void GetSnapshot(std::array<Reg, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, Long& pc);
    void GetCSRSnapshot(std::unordered_map<Long, Long>& csrs) const;

    inline Expected<Reg&, std::range_error> GetRegister(size_t reg) {
        if (reg >= REGISTER_COUNT) {
            return Unexpected<std::range_error>(std::range_error(std::format("Could not get register {}. Max is {}", reg, REGISTER_COUNT)));
        }

        return regs[reg];
    }

    inline Expected<Float&, std::range_error> GetFloatRegister(size_t reg) {
        if (reg >= REGISTER_COUNT) {
            return Unexpected<std::range_error>(std::range_error(std::format("Could not get float register {}. Max is {}", reg, REGISTER_COUNT)));
        }

        return fregs[reg];
    }

    inline Long GetPC() const {
        return pc;
    }

    inline Long GetSP() const {
        return regs[REG_SP].u64;
    }

    size_t GetInstructionsPerSecond();

    inline void SetBreakPoint(Address addr) {
        break_points.insert(addr);
    }

    inline void ClearBreakPoint(Address addr) {
        if (break_points.contains(addr))
            break_points.erase(addr);
    }

    bool IsBreakPoint(Address addr);

    void UpdateTime(double delta_time);

    using ECallHandler = std::function<void(Hart, bool, Memory& memory, std::array<Reg, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>& fregs)>;

private:
    static void EmptyECallHandler(Hart hart, bool is_32_bit_mode, Memory& memory, std::array<Reg, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&);

    static std::unordered_map<Long, ECallHandler> ecall_handlers;

public:
    inline static void RegisterECall(Long handler_index, ECallHandler handler) {
        ecall_handlers[handler_index] = handler;
    }
};

#endif