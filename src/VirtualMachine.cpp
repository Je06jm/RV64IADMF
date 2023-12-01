#include "VirtualMachine.hpp"

#include <format>
#include <cassert>

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

    csrs[CSR_MISA] = ISA_32_BITS | ISA_A | ISA_F | ISA_I | ISA_M | ISA_S | ISA_U;
}

VirtualMachine::~VirtualMachine() {
    lock.lock();
    running = false;
    lock.unlock();

    if (vm_thread != nullptr) {
        vm_thread->join();
    }
}

void VirtualMachine::Start() {
    if (vm_thread != nullptr) {
        throw std::runtime_error("Cannot start the VirtualMachine as it is already started");
    }

    running = true;

    vm_thread = std::make_unique<std::thread>([&]() {
        while (true) {
            lock.lock();
            if (!running) {
                lock.unlock();
                break;
            }

            if (pc & 0b11) {
                err = "PC is not aligned to a 32 bit address";
                lock.unlock();
                break;
            }

            uint32_t instr = 0;
            try {
                instr = memory.Read32(pc);
            } catch (std::exception& e) {
                err = e.what();
                break;
            }

            lock.unlock();

            constexpr uint8_t OPCODE_LUI = 0b0110111;
            constexpr uint8_t OPCODE_AUIPC = 0b0010111;
            constexpr uint8_t OPCODE_JAL = 0b1101111;
            constexpr uint8_t OPCODE_JALR = 0b1100111;
            constexpr uint8_t OPCODE_BRANCH = 0b1100011;
            constexpr uint8_t OPCODE_LOAD = 0b0000011;
            constexpr uint8_t OPCODE_STORE = 0b0100011;
            constexpr uint8_t OPCODE_ALU_IMM = 0b0010011;
            constexpr uint8_t OPCODE_ALU = 0b0110011;
            constexpr uint8_t OPCODE_FENCE = 0b0001111;
            constexpr uint8_t OPCODE_ENV = 0b1110011;
            constexpr uint8_t OPCODE_ATOMIC = 0b0101111;
            constexpr uint8_t OPCODE_FLW = 0b0000111;
            constexpr uint8_t OPCODE_FSW = 0b0100111;
            constexpr uint8_t OPCODE_FPU_NM = 0b1000111;
            constexpr uint8_t OPCODE_FPU = 0b1010011;

            constexpr uint8_t OPCODE_MASK = 0b1111111;

            switch (instr & OPCODE_MASK) {
                case OPCODE_LUI:
                    break;
                
                case OPCODE_AUIPC:
                    break;
                
                case OPCODE_JAL:
                    break;
                
                case OPCODE_JALR:
                    break;
                
                case OPCODE_BRANCH:
                    break;
                
                case OPCODE_LOAD:
                    break;
                
                case OPCODE_STORE:
                    break;
                
                case OPCODE_ALU_IMM:
                    break;
                
                case OPCODE_ALU:
                    break;
                
                case OPCODE_FENCE:
                    break;
                
                case OPCODE_ENV:
                    break;
                
                case OPCODE_ATOMIC:
                    break;
                
                case OPCODE_FLW:
                    break;
                
                case OPCODE_FSW:
                    break;
                
                case OPCODE_FPU_NM:
                    break;
                
                case OPCODE_FPU:
                    break;
                
                default: // Error!
                    break;
            }
        }
    });
}

bool VirtualMachine::IsRunning() {
    lock.lock();
    auto is_running = running;
    lock.unlock();
    return is_running;
}

void VirtualMachine::Stop() {
    lock.lock();
    running = false;
    lock.unlock();
}

void VirtualMachine::GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<float, REGISTER_COUNT>& fregisters, uint32_t& pc) {
    lock.lock();
    registers = regs;
    fregisters = fregs;
    pc = this->pc;
    lock.unlock();
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