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

    csrs[CSR_MISA] = ISA_32_BITS | ISA_F | ISA_I | ISA_M;
}

VirtualMachine::~VirtualMachine() {
    running = false;
}

void VirtualMachine::Step(uint32_t steps) {
    for (uint32_t i = 0; i < steps && running; i++, pc += 4) {
        // Do step here
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