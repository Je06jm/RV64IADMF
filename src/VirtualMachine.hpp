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
    static constexpr uint16_t CSR_FFLAGS = 0x001;
    static constexpr uint16_t CSR_FRM = 0x002;
    static constexpr uint16_t CSR_FCSR = 0x003;

    static constexpr uint32_t CSR_FCSR_NV = 0b10000;
    static constexpr uint32_t CSR_FCSR_DZ = 0b1000;
    static constexpr uint32_t CSR_FCSR_OF = 0b100;
    static constexpr uint32_t CSR_FCSR_UF = 0b10;
    static constexpr uint32_t CSR_FCSR_NX = 0b1;

    static constexpr uint32_t CSR_FCSR_FLAGS = 0b11111;
    
    static constexpr uint16_t CSR_CYCLE = 0xc00;
    static constexpr uint16_t CSR_TIME = 0xc01;
    static constexpr uint16_t CSR_INSTRET = 0xc02;
    
    static constexpr uint16_t CSR_PERF_COUNTER_MAX = 32;
    static constexpr uint16_t CSR_HPMCOUNTER = 0xc04;

    static constexpr uint16_t CSR_CYCLEH = 0xc80;
    static constexpr uint16_t CSR_TIMEH = 0xc81;
    static constexpr uint16_t CSR_INSTRETH = 0xc82;
    static constexpr uint16_t CSR_HPMCOUNTERH = 0xc84;

    static constexpr uint16_t CSR_SSTATUS = 0x100;
    static constexpr uint16_t CSR_SIE = 0x104;
    static constexpr uint16_t CSR_STVEC = 0x105;
    static constexpr uint16_t CSR_SCOUNTEREN = 0x106;
    static constexpr uint16_t CSR_SENVCFG = 0x10a;
    static constexpr uint16_t CSR_SSCRATCH = 0x140;
    static constexpr uint16_t CSR_SEPC = 0x141;
    static constexpr uint16_t CSR_SCAUSE = 0x142;
    static constexpr uint16_t CSR_STVAL = 0x143;
    static constexpr uint16_t CSR_SIP = 0x144;
    static constexpr uint16_t CSR_SATP = 0x145;
    static constexpr uint16_t CSR_SCONTEXT = 0x5a8;

    static constexpr uint16_t CSR_MVENDORID = 0xf11;
    static constexpr uint16_t CSR_MARCHID = 0xf12;
    static constexpr uint16_t CSR_MIMPID = 0xf13;
    static constexpr uint16_t CSR_MHARTID = 0xf14;
    static constexpr uint16_t CSR_MCONFIGPTR = 0xf15;
    static constexpr uint16_t CSR_MSTATUS = 0x300;
    static constexpr uint16_t CSR_MISA = 0x301;
    static constexpr uint16_t CSR_MEDELEG = 0x302;
    static constexpr uint16_t CSR_MIDELEG = 0x303;
    static constexpr uint16_t CSR_MIE = 0x304;
    static constexpr uint16_t CSR_MTVEC = 0x305;
    static constexpr uint16_t CSR_MCOUNTEREN = 0x306;
    static constexpr uint16_t CSR_MSTATUSH = 0x310;
    static constexpr uint16_t CSR_MSCRATCH = 0x340;
    static constexpr uint16_t CSR_MEPC = 0x341;
    static constexpr uint16_t CSR_MCAUSE = 0x342;
    static constexpr uint16_t CSR_MTVAL = 0x343;
    static constexpr uint16_t CSR_MIP = 0x344;
    static constexpr uint16_t CSR_MTINST = 0x34a;
    static constexpr uint16_t CSR_MTVAL2 = 0x34b;
    static constexpr uint16_t CSR_MENVCFG = 0x30a;
    static constexpr uint16_t CSR_MENVCFGH = 0x31a;
    static constexpr uint16_t CSR_MSECCFG = 0x747;
    static constexpr uint16_t CSR_MSECCFGH = 0x757;
    
    static constexpr uint16_t CSR_PMP_MAX = 64;
    static constexpr uint16_t CSR_PMPCFG0 = 0x3a0;

    static constexpr uint16_t CSR_MCYCLE = 0xb00;
    static constexpr uint16_t CSR_MINSTRET = 0xb02;
    static constexpr uint16_t CSR_HPMCOUNTER3 = 0xb03;
    static constexpr uint16_t CSR_MCOUNTINHIBIT = 0x320;

    static constexpr uint16_t CSR_PERFORMANCE_EVENT_MAX = 32;
    static constexpr uint16_t CSR_MHPMEVENT3 = 0x323;
    static constexpr uint16_t CSR_TSELECT = 0x7a0;
    static constexpr uint16_t CSR_TDATA1 = 0x7a1;
    static constexpr uint16_t CSR_TDATA2 = 0x7a2;
    static constexpr uint16_t CSR_TDATA3 = 0x7a3;
    static constexpr uint16_t CSR_MCONTEXT = 0x7a8;

    static constexpr uint8_t MACHINE_MODE = 0b00;
    static constexpr uint8_t SUPERVISOR_MODE = 0b01;
    static constexpr uint8_t USER_MODE = 0b10;

    static constexpr uint32_t ISA_BITS_MASK = 0b11 << 30;
    static constexpr uint32_t ISA_32_BITS = 1 << 30;

    static constexpr uint32_t ISA_A = 1<<0;
    static constexpr uint32_t ISA_F = 1<<5;
    static constexpr uint32_t ISA_I = 1<<8;
    static constexpr uint32_t ISA_M = 1<<12;
    static constexpr uint32_t ISA_S = 1<<18;
    static constexpr uint32_t ISA_U = 1<<20;

    std::array<uint32_t, REGISTER_COUNT> regs;
    std::array<Float, REGISTER_COUNT> fregs;

    std::unordered_map<uint32_t, uint32_t> csrs;

    bool CSRPrivilegeCheck(uint32_t csr);
    uint32_t ReadCSR(uint32_t csr, bool is_internal_read = false);
    void WriteCSR(uint32_t csr, uint32_t value);

    static const int default_rounding_mode;
    bool ChangeRoundingMode(uint8_t rm = 0xff);
    bool CheckFloatErrors();

    static constexpr size_t TLB_CACHE_SIZE = 16;

public:
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

    class TLBEntry {
        VirtualMachine& vm;
        uint32_t physical_address;
        uint32_t table;
        uint32_t table_entry;

    public:
        TLBEntry(VirtualMachine& vm, uint32_t physical_address, uint32_t table, uint32_t table_entry) : vm{vm}, physical_address{physical_address}, table{table}, table_entry{table_entry} {}

        inline TLBEntry& operator=(const TLBEntry& other) {
            physical_address = other.physical_address;
            table = other.table;
            table_entry = other.table_entry;
            return *this;
        }

        static constexpr uint32_t FLAG_VALID = (1<<0);
        static constexpr uint32_t FLAG_READ = (1<<1);
        static constexpr uint32_t FLAG_WRITE = (1<<2);
        static constexpr uint32_t FLAG_EXECUTE = (1<<3);
        static constexpr uint32_t FLAG_USER = (1<<4);
        static constexpr uint32_t FLAG_GLOBAL = (1<<5);
        static constexpr uint32_t FLAG_ACCESSED = (1<<6);
        static constexpr uint32_t FLAG_DIRTY = (1<<7);
        
        static constexpr uint32_t PAGE_MASK = 0xfff;
        static constexpr uint32_t MEGA_PAGE_MASK = 0x3fffff;

        inline bool IsVirtual() const {
            return (table == -1U);
        }

        inline bool IsMegaPage() const {
            return IsFlagsSet(FLAG_READ | FLAG_WRITE | FLAG_EXECUTE);
        }
        
        bool IsFlagsSet(uint32_t flags) const;
        void SetFlags(uint32_t flags);
        void ClearFlgs(uint32_t flags);
        
        bool IsInPage(uint32_t phys_address) const;
        
        uint32_t TranslateAddress(uint32_t phys_address) const;

        bool CheckValid();
        bool CheckExecution(bool as_user);
        bool CheckRead(bool as_user);
        bool CheckWrite(bool as_user);

        inline static TLBEntry CreateNonVirtual(VirtualMachine& vm) {
            return {vm, -1U, -1U, -1U};
        }
    };
    
private:

    std::vector<TLBEntry> tlb_cache;

    Memory& memory;

    uint32_t pc;

    bool running = false;
    std::string err = "";

    std::set<uint32_t> break_points;

    uint32_t ticks;
    std::vector<double> history_delta;
    std::vector<uint32_t> history_tick;

    static constexpr size_t MAX_HISTORY = 15;

public:
    const size_t instructions_per_second;
    VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second, uint32_t hart_id);
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&) = delete;
    ~VirtualMachine();
    
    inline void Start() { running = true; }
    inline bool IsRunning() const { return running; }
    inline void Stop() { running = false; }

    bool Step(uint32_t steps = 1000);

    void GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<Float, REGISTER_COUNT>& fregisters, uint32_t& pc);

    inline uint32_t GetPC() const {
        return pc;
    }

    inline uint32_t GetSP() const {
        return regs[REG_SP];
    }

    TLBEntry GetTLBLookup(uint32_t phys_addr, bool bypass_cache = false);

    size_t GetInstructionsPerSecond();

    inline void SetBreakPoint(uint32_t addr) {
        break_points.insert(addr);
    }

    inline void ClearBreakPoint(uint32_t addr) {
        if (break_points.contains(addr))
            break_points.erase(addr);
    }

    inline bool IsBreakPoint(uint32_t addr) {
        return break_points.contains(addr);
    }

    void UpdateTime();

    using ECallHandler = std::function<void(Memory& memory, std::array<uint32_t, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>& fregs)>;

private:
    static void EmptyECallHandler(Memory& memory, std::array<uint32_t, REGISTER_COUNT>& regs, std::array<Float, REGISTER_COUNT>&);

    static std::vector<ECallHandler> ecall_handlers;

public:
    inline static void RegisterECall(uint32_t handler_index, ECallHandler handler) {
        if (handler_index >= ecall_handlers.size()) ecall_handlers.resize(handler_index + 1, EmptyECallHandler);
        ecall_handlers[handler_index] = handler;
    }
};

#endif