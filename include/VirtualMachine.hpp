#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

#include "Memory.hpp"
#include "Float.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

class VirtualMachine {
    friend class TLBEntry;

public:
    static constexpr size_t REGISTER_COUNT = 32;

private:
    static constexpr Byte MACHINE_MODE = 0b11;
    static constexpr Byte SUPERVISOR_MODE = 0b01;
    static constexpr Byte USER_MODE = 0b00;

    static constexpr Word ISA_BITS_MASK = 0b11 << 30;
    static constexpr Word ISA_32_BITS = 1 << 30;

    static constexpr Word ISA_A = 1<<0;
    static constexpr Word ISA_D = 1<<3;
    static constexpr Word ISA_F = 1<<5;
    static constexpr Word ISA_I = 1<<8;
    static constexpr Word ISA_M = 1<<12;
    static constexpr Word ISA_S = 1<<18;
    static constexpr Word ISA_U = 1<<20;

    std::array<Word, REGISTER_COUNT> regs;
    std::array<Float, REGISTER_COUNT> fregs;

    std::unordered_map<Word, Word> csrs;

    bool CSRPrivilegeCheck(Word csr);
    Word ReadCSR(Word csr, bool is_internal_read = false);
    void WriteCSR(Word csr, Word value);

    static const int default_rounding_mode;
    bool ChangeRoundingMode(Byte rm = 0xff);
    bool CheckFloatErrors();

public:
    static constexpr Half CSR_FFLAGS = 0x001;
    static constexpr Half CSR_FRM = 0x002;
    static constexpr Half CSR_FCSR = 0x003;

    static constexpr Word CSR_FCSR_NV = 0b10000;
    static constexpr Word CSR_FCSR_DZ = 0b1000;
    static constexpr Word CSR_FCSR_OF = 0b100;
    static constexpr Word CSR_FCSR_UF = 0b10;
    static constexpr Word CSR_FCSR_NX = 0b1;

    static constexpr Word CSR_FCSR_FLAGS = 0b11111;
    
    static constexpr Half CSR_CYCLE = 0xc00;
    static constexpr Half CSR_TIME = 0xc01;
    static constexpr Half CSR_INSTRET = 0xc02;
    
    static constexpr Half CSR_PERF_COUNTER_MAX = 32;
    static constexpr Half CSR_HPMCOUNTER = 0xc03;

    static constexpr Half CSR_CYCLEH = 0xc80;
    static constexpr Half CSR_TIMEH = 0xc81;
    static constexpr Half CSR_INSTRETH = 0xc82;
    static constexpr Half CSR_HPMCOUNTERH = 0xc84;

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
    static constexpr Half CSR_MSTATUSH = 0x310;
    static constexpr Half CSR_MSCRATCH = 0x340;
    static constexpr Half CSR_MEPC = 0x341;
    static constexpr Half CSR_MCAUSE = 0x342;
    static constexpr Half CSR_MTVAL = 0x343;
    static constexpr Half CSR_MIP = 0x344;
    static constexpr Half CSR_MTINST = 0x34a;
    static constexpr Half CSR_MTVAL2 = 0x34b;
    static constexpr Half CSR_MENVCFG = 0x30a;
    static constexpr Half CSR_MENVCFGH = 0x31a;
    static constexpr Half CSR_MSECCFG = 0x747;
    static constexpr Half CSR_MSECCFGH = 0x757;
    
    static constexpr Half CSR_PMPCFG_MAX = 16;
    static constexpr Half CSR_PMPCFG0 = 0x3a0;

    static constexpr Half CSR_PMPADDR_MAX = 64;
    static constexpr Half CSR_PMPADDR0 = 0x3b0;

    static constexpr Half CSR_MCYCLE = 0xb00;
    static constexpr Half CSR_MINSTRET = 0xb02;
    static constexpr Half CSR_MHPMCOUNTER3 = 0xb03;
    static constexpr Half CSR_MCYCLEH = 0xb80;
    static constexpr Half CSR_MINSTRETH = 0xb82;
    static constexpr Half CSR_MHPMCOUNTER3H = 0xb83;
    static constexpr Half CSR_MCOUNTINHIBIT = 0x320;

    static constexpr Half CSR_PERFORMANCE_EVENT_MAX = 32;
    static constexpr Half CSR_MHPMEVENT3 = 0x323;

    static constexpr Word MSTATUS_WRITABLE_BITS = 0b00000000000011100111100110101010;

    union MStatus {
        struct {
            Word _unused0 : 1;
            Word SIE : 1;
            Word _unused1 : 1;
            Word MIE : 1;
            Word _unused2 : 1;
            Word SPIE : 1;
            Word _unused3 : 1;
            Word MPIE : 1;
            Word SPP : 1;
            Word _unused4 : 2;
            Word MPP : 2;
            Word FS : 2;
            Word _unused5 : 2;
            Word MPRIV : 1;
            Word SUM : 1;
            Word MXR : 1;
            Word _unused6 : 1;
            Word _unused7 : 1;
            Word _unused8 : 1;
            Word _unused9 : 8;
            Word SD : 1;
        };
        Word raw;
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

    static constexpr Word SSTATUS_WRITABLE_BITS = 0b00000000000011000110000100100010;

    union SStatus {
        struct {
            Word _unused0 : 1;
            Word SIE : 1;
            Word _unused1 : 3;
            Word SPIE : 1;
            Word _unused2 : 1;
            Word _unused3 : 1;
            Word SPP : 1;
            Word _unused4 : 2;
            Word _unused5 : 2;
            Word FS : 2;
            Word _unused6 : 2;
            Word _unused7 : 1;
            Word SUM : 1;
            Word MXR : 1;
            Word _unused8 : 11;
            Word SD : 1;
        };
        Word raw;
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
            Word V : 1;
            Word R : 1;
            Word W : 1;
            Word X : 1;
            Word U : 1;
            Word G : 1;
            Word A : 1;
            Word D : 1;
            Word RSW : 2;
            Word PPN_0 : 10;
            Word PPN_1 : 12;
        };
        struct {
            Word _unused : 10;
            Word PPN : 22;
        };
        Word raw;

        inline bool IsLeaf() const {
            return X || W || R;
        }
    };

    static constexpr Word INTERRUPT_SUPERVISOR_SOFTWARE = 0x1;
    static constexpr Word INTERRUPT_MACHINE_SOFTWARE = 0x3;
    static constexpr Word INTERRUPT_SUPERVISOR_TIMER = 0x5;
    static constexpr Word INTERRUPT_MACHINE_TIMER = 0x7;
    static constexpr Word INTERRUPT_SUPERVISOR_EXTERNAL = 0x9;
    static constexpr Word INTERRUPT_MACHINE_EXTERNAL = 0xa;

    void RaiseInterrupt(Word cause);

private:
    bool waiting_for_interrupt = false;

public:
    inline bool IsWaitingForInterrupt() const {
        return waiting_for_interrupt;
    }

private:
    static constexpr Word TRAP_INTERRUPT_BIT = (1ULL << 31);

    static constexpr Word EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED = 0x0;
    static constexpr Word EXCEPTION_INSTRUCTION_ADDRESS_FAULT = 0x1;
    static constexpr Word EXCEPTION_ILLEGAL_INSTRUCTION = 0x2;
    static constexpr Word EXCEPTION_BREAKPOINT = 0x3;
    static constexpr Word EXCEPTION_LOAD_ADDRESS_MISALIGNED = 0x4;
    static constexpr Word EXCEPTION_LOAD_ACCESS_FAULT = 0x5;
    static constexpr Word EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED = 0x6;
    static constexpr Word EXCEPTION_STORE_AMO_ACCESS_FAULT = 0x7;
    static constexpr Word EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE = 0x8;
    static constexpr Word EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE = 0x9;
    static constexpr Word EXCEPTION_ENVIRONMENT_CALL_FROM_M_MODE = 0xb;
    static constexpr Word EXCEPTION_INSTRUCTION_PAGE_FAULT = 0xc;
    static constexpr Word EXCEPTION_INSTRUCTION_LOAD_PAGE_FAULT = 0xd;
    static constexpr Word EXCEPTION_STORE_AMO_PAGE_FAULT = 0xf;

    void RaiseException(Word cause);

    static constexpr Word VALID_INTERRUPT_BITS = 0b0;

    Word mip = 0;
    Word mie = 0;
    Word mideleg = 0;
    Word sip = 0;
    Word sie = 0;

    void RaiseMachineTrap(Word cause);
    void RaiseSupervisorTrap(Word cause);
    
    MStatus mstatus;
    SStatus sstatus;

    struct TLBCacheEntry {
        Word tag : 31;
        Word super : 1;
        
        TLBEntry tlb_entry;

        TLBCacheEntry() : tag{0}, super{0} { tlb_entry.raw = 0; }
    };

    static constexpr size_t TLB_CACHE_SIZE = 16;
    
    inline static consteval auto GetLog2(auto value) {
        auto i = 0;
        while ((1ULL << i) < value) i++;
        return i;
    }

public:
    std::pair<TLBEntry, bool> GetTLBLookup(Word phys_addr, bool bypass_cache = false, bool is_amo = false);

private:
    size_t tlb_cache_round_robin = 0;
    std::array<TLBCacheEntry, TLB_CACHE_SIZE> tlb_cache;

    Memory& memory;

    union SATP {
        struct {
            Word PPN : 22;
            Word ASID : 9;
            Word MODE : 1;
        };
        Word raw;
    };

    SATP satp;

public:
    inline bool IsUsingVirtualMemory() const {
        return (privilege_level != PrivilegeLevel::Machine) && (satp.MODE != 0);
    }

    inline Word GetPendingMachineInterrupts() const {
        return mip;
    }

    inline Word GetEnabledMachineInterrupts() const {
        return mie;
    }

    inline Word GetDelegatedMachineInterrupts() const {
        return mideleg;
    }

    inline Word GetPendingSupervisorInterrupts() const {
        return sip;
    }

    inline Word GetEnabledSupervisorInterrupts() const {
        return sie;
    }

private:
    std::pair<Address, bool> TranslateMemoryAddress(Address address, bool is_write, bool is_execute, bool is_amo = false);

    Word pc;
    uint64_t cycles;

    bool running = false;
    bool paused = false;
    bool pause_on_break = false;
    bool pause_on_restart = false;
    std::string err = "";

    std::set<Word> break_points;

    Word ticks;
    std::vector<double> history_delta;
    std::vector<Word> history_tick;

    static constexpr size_t MAX_HISTORY = 15;

    void Setup();

    class CSRMappedMemory : public MemoryRegion {
    public:
        static constexpr uint64_t TICKS_PER_SECOND = 32768;
        uint64_t time = 0;
        uint64_t time_cmp = -1ULL;

        CSRMappedMemory() : MemoryRegion{TYPE_MAPPED_CSRS, 0Xf00, 0x100, true, true} {}

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
    VirtualMachine(Memory& memory, Word starting_pc, Word hart_id);
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&);
    ~VirtualMachine();
    
    inline void Start() { running = true; }
    inline void Restart(Word pc, Word source_hart) {
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

    bool Step(Word steps = 1000);
    void Run();

    inline void SetPC(Word pc) { this->pc = pc; }

    void GetSnapshot(std::array<Word, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, Word& pc);
    void GetCSRSnapshot(std::unordered_map<Word, Word>& csrs) const;

    inline Word GetPC() const {
        return pc;
    }

    inline Word GetSP() const {
        return regs[REG_SP];
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

    void UpdateTime();

    using ECallHandler = std::function<void(Word hart, Memory& memory, std::array<Word, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>& fregs)>;

private:
    static void EmptyECallHandler(Word hart, Memory& memory, std::array<Word, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&);

    static std::unordered_map<Word, ECallHandler> ecall_handlers;

public:
    inline static void RegisterECall(Word handler_index, ECallHandler handler) {
        ecall_handlers[handler_index] = handler;
    }
};

#endif