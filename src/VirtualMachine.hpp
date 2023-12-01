#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

#include "Memory.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <string>

class VirtualMachine {
    friend class TLBEntry;

public:
    static constexpr size_t REGISTER_COUNT = 32;
    static constexpr size_t CSR_COUNT = 4096;

private:
    static constexpr uint16_t CSR_FFLAGS = 0x001;
    static constexpr uint16_t CSR_FRM = 0x002;
    static constexpr uint16_t CSR_FCSR = 0x003;
    
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

    static constexpr uint32_t ISA_BITS_MASK = 0b11 << 30;
    static constexpr uint32_t ISA_32_BITS = 1 << 30;

    static constexpr uint32_t ISA_A = 1<<0;
    static constexpr uint32_t ISA_F = 1<<5;
    static constexpr uint32_t ISA_I = 1<<8;
    static constexpr uint32_t ISA_M = 1<<12;
    static constexpr uint32_t ISA_S = 1<<18;
    static constexpr uint32_t ISA_U = 1<<20;

    std::array<uint32_t, REGISTER_COUNT> regs;
    std::array<float, REGISTER_COUNT> fregs;
    std::array<uint32_t, CSR_COUNT> csrs;

    static constexpr size_t TLB_CACHE_SIZE = 16;

public:
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
            return (table == -1ULL);
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
            return {vm, -1ULL, -1ULL, -1ULL};
        }
    };
    
private:

    std::vector<TLBEntry> tlb_cache;

    Memory& memory;

    uint32_t pc;

    bool running = true;
    std::string err = "";
    std::mutex lock;

    std::unique_ptr<std::thread> vm_thread = nullptr;
public:
    const size_t instructions_per_second;
    VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second, uint32_t hart_id);
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&) = delete;
    ~VirtualMachine();
    
    void Start();
    bool IsRunning();
    void Stop();

    void GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<float, REGISTER_COUNT>& fregisters, uint32_t& pc);

    inline uint32_t GetPC() const {
        return pc;
    }

    TLBEntry GetTLBLookup(uint32_t phys_addr, bool bypass_cache = false);

    size_t GetInstructionsPerSecond();
};

#endif