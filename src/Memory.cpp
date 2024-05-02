#include "Memory.hpp"

#include <stdexcept>
#include <format>
#include <fstream>

uint16_t MemoryRegion::ReadHalf(uint32_t address) const {
    auto word = ReadWord(address & ~3);
    return static_cast<uint16_t>(word >> ((address & 2) * 8));
}

uint8_t MemoryRegion::ReadByte(uint32_t address) const {
    auto word = ReadWord(address & ~3);
    return static_cast<uint8_t>(word >> ((address & 3) * 8));
}

void MemoryRegion::WriteHalf(uint32_t address, uint16_t half) {
    auto word = ReadWord(address & ~3);
    auto shift = (address & 2) * 8;
    word &= ~(0xffff << shift);
    word |= half << shift;
    WriteWord(address & ~3, word);
}

void MemoryRegion::WriteByte(uint32_t address, uint8_t byte) {
    auto word = ReadWord(address & ~3);
    auto shift = (address & 3) * 8;
    word &= ~(0xff << shift);
    word |= byte << shift;
    WriteWord(address & ~3, word);
}

std::unique_ptr<MemoryROM> MemoryROM::Create(const std::vector<uint32_t>& words, uint32_t base) {
    return std::unique_ptr<MemoryROM>(new MemoryROM(words, base & ~3));
}

MemoryRAM::MemoryRAM(uint32_t base, uint32_t size) : MemoryRegion(TYPE_GENERAL_RAM, base, size, true, true) {
    size_t pages_count = size / WORDS_PER_PAGE;

    for (size_t i = 0; i < pages_count; i++)
        pages.push_back(nullptr);
}

uint32_t MemoryRAM::ReadWord(uint32_t address) const {
    size_t page = address / WORDS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    return (*pages[page])[address >> 2];
}

void MemoryRAM::WriteWord(uint32_t address, uint32_t word) {
    size_t page = address / WORDS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    (*pages[page])[address >> 2] = word;
}

std::unique_ptr<MemoryRAM> MemoryRAM::Create(uint32_t base, uint32_t size) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    return std::unique_ptr<MemoryRAM>(new MemoryRAM(base & ~3, size));
}

union U32S32 {
    uint32_t u;
    int32_t s;
};

MemoryRegion* Memory::GetMemoryRegion(uint32_t address) {
    for (auto& region : regions) {
        uint32_t end = region->base + region->size;
        if (address >= region->base && address < end)
            return region.get();
    }
    return nullptr;
}

const MemoryRegion* Memory::GetMemoryRegion(uint32_t address) const {
    for (const auto& region : regions) {
        uint32_t end = region->base + region->size;
        if (address >= region->base && address < end)
            return region.get();
    }
    return nullptr;
}

uint32_t Memory::ReadWord(uint32_t address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned read of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#10x} as it's unreadable", address));

    auto word = region->ReadWord(address - region->base);

    return word;
}

uint16_t Memory::ReadHalf(uint32_t address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 1)
        throw std::runtime_error(std::format("Unaligned read of half at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#10x} as it's unreadable", address));

    auto half = region->ReadHalf(address - region->base);

    return half;
}

uint8_t Memory::ReadByte(uint32_t address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#10x} as it's unreadable", address));

    auto byte = region->ReadByte(address - region->base);

    return byte;
}

std::pair<uint32_t, bool> Memory::PeekWord(uint32_t address) const {
    if (address & 3)
        throw std::runtime_error(std::format("Unaligned read of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        return {0, false};
    
    if (!region->readable)
        return {0, false};
    
    auto word = region->ReadWord(address - region->base);

    return {word, true};
}

void Memory::WriteWord(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned write of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#10x} as it's unwritable", address));

    region->WriteWord(address - region->base, word);
}

void Memory::WriteHalf(uint32_t address, uint16_t half) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 1)
        throw std::runtime_error(std::format("Unaligned write of half at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#10x} as it's unwritable", address));
    
    region->WriteHalf(address - region->base, half);
}

void Memory::WriteByte(uint32_t address, uint8_t byte) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#10x} as it's unwritable", address));
    
    region->WriteByte(address - region->base, byte);
}

uint32_t Memory::AtomicSwap(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, word);
    region->Lock();

    return old_word;
}

uint32_t Memory::AtomicAdd(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word + word);
    region->Unlock();

    return old_word;
}

uint32_t Memory::AtomicAnd(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word & word);
    region->Unlock();

    return old_word;
}

uint32_t Memory::AtomicOr(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word | word);
    region->Unlock();

    return old_word;
}

uint32_t Memory::AtomicXor(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word ^ word);
    region->Unlock();

    return old_word;
}

int32_t Memory::AtomicMin(uint32_t address, int32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    v.u = old_word;
    if (word < v.s) v.s = word;
    region->WriteWord(address - region->base, v.u);
    region->Unlock();

    return old_word;
}

uint32_t Memory::AtomicMinU(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    if (word < old_word) region->WriteWord(address - region->base, word);
    region->Unlock();

    return old_word;
}

int32_t Memory::AtomicMax(uint32_t address, int32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    v.u = old_word;
    if (word > v.s) v.s = word;
    region->WriteWord(address - region->base, v.u);
    region->Unlock();

    return old_word;
}

uint32_t Memory::AtomicMaxU(uint32_t address, uint32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#10x} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    if (word > old_word) region->WriteWord(address - region->base, word);
    region->Unlock();

    return old_word;
}

void Memory::WriteWords(uint32_t address, const std::vector<uint32_t>& words) {
    for (uint32_t head = address, i = 0; i < words.size(); i++, head += 4) {
        WriteWord(head, words[i]);
    }
}

std::vector<uint32_t> Memory::ReadWords(uint32_t address, uint32_t count) const {
    std::vector<uint32_t> data;
    for (uint32_t head = address, i = 0; i < count; i++, head += 4) {
        data.push_back(ReadWord(head));
    }

    return data;
}

std::vector<std::pair<uint32_t, bool>> Memory::PeekWords(uint32_t address, uint32_t count) const {
    std::vector<std::pair<uint32_t, bool>> data;

    const MemoryRegion* region = nullptr;
    uint32_t end;
    for (uint32_t head = address, i = 0; i < count; i++, head += 4) {
        if (!region) {
            region = GetMemoryRegion(head);
            if (region) {
                end = region->base + region->size;
                data.push_back({region->ReadWord(head - region->base), true});
            }
            else
                data.push_back({0, false});
        }
        else {
            if (!(head >= region->base && head < end))
                region = GetMemoryRegion(head);
            
            if (region) {
                region->Lock();
                data.push_back({region->ReadWord(head - region->base), true});
                region->Unlock();
            }
            else
                data.push_back({0, false});
        }
    }

    return data;
}

uint32_t Memory::ReadWordReserved(uint32_t address, uint32_t cpu_id) const {
    lock.lock();
    for (auto& item : reservations) {
        if (item.second == (address & ~0b11)) {
            reservations.erase(item.first);
            break;
        }
    }
    
    reservations[cpu_id] = address & ~0b11;
    lock.unlock();

    return ReadWord(address);
}

bool Memory::WriteWordConditional(uint32_t address, uint32_t value, uint32_t cpu_id) {
    lock.lock();
    if (reservations.find(cpu_id) == reservations.end()) {
        lock.unlock();
        return false;
    }

    if (reservations[cpu_id] != (address & ~0b11)) {
        auto addr = reservations[cpu_id];
        lock.unlock();
        return false;
    }

    reservations.erase(cpu_id);

    lock.unlock();

    WriteWord(address, value);
    return true;
}

uint32_t Memory::ReadFileInto(const std::string& path, uint32_t address) {
    std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for reading", path));
    }

    uint32_t size = file.tellg();
    uint32_t words = size / 4;
    if (size % 4) words++;
    
    file.seekg(0);

    std::vector<uint32_t> data(words);
    data[words - 1] = 0;

    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    WriteWords(address, data);

    return size;
}

void Memory::WriteToFile(const std::string& path, uint32_t address, uint32_t bytes) {
    std::ofstream file(path, std::ios_base::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for writing", path));
    }

    uint32_t count = (bytes + 3) / 4;
    
    auto data = ReadWords(address, count);

    file.write(reinterpret_cast<char*>(data.data()), bytes);
    file.close();
}