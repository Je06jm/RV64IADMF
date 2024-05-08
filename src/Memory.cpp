#include "Memory.hpp"

#include <stdexcept>
#include <format>
#include <fstream>

Half MemoryRegion::ReadHalf(Address address) const {
    auto word = ReadWord(address & ~3);
    return static_cast<Half>(word >> ((address & 2) * 8));
}

Byte MemoryRegion::ReadByte(Address address) const {
    auto word = ReadWord(address & ~3);
    return static_cast<Byte>(word >> ((address & 3) * 8));
}

void MemoryRegion::WriteHalf(Address address, Half half) {
    auto word = ReadWord(address & ~3);
    auto shift = (address & 2) * 8;
    word &= ~(0xffff << shift);
    word |= half << shift;
    WriteWord(address & ~3, word);
}

void MemoryRegion::WriteByte(Address address, Byte byte) {
    auto word = ReadWord(address & ~3);
    auto shift = (address & 3) * 8;
    word &= ~(0xff << shift);
    word |= byte << shift;
    WriteWord(address & ~3, word);
}

std::unique_ptr<MemoryROM> MemoryROM::Create(const std::vector<Word>& words, Address base) {
    return std::unique_ptr<MemoryROM>(new MemoryROM(words, base & ~3));
}

MemoryRAM::MemoryRAM(Address base, Address size) : MemoryRegion(TYPE_GENERAL_RAM, base, size, true, true) {
    size_t pages_count = size / WORDS_PER_PAGE;

    for (size_t i = 0; i < pages_count; i++)
        pages.push_back(nullptr);
}

Word MemoryRAM::ReadWord(Address address) const {
    size_t page = address / WORDS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    return (*pages[page])[address >> 2];
}

void MemoryRAM::WriteWord(Address address, Word word) {
    size_t page = address / WORDS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    (*pages[page])[address >> 2] = word;
}

std::unique_ptr<MemoryRAM> MemoryRAM::Create(Address base, Address size) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    return std::unique_ptr<MemoryRAM>(new MemoryRAM(base & ~3, size));
}

union U32S32 {
    Word u;
    SWord s;
};

MemoryRegion* Memory::GetMemoryRegion(Address address) {
    for (auto& region : regions) {
        Address end = region->base + region->size;
        if (address >= region->base && address < end)
            return region.get();
    }
    return nullptr;
}

const MemoryRegion* Memory::GetMemoryRegion(Address address) const {
    for (const auto& region : regions) {
        Address end = region->base + region->size;
        if (address >= region->base && address < end)
            return region.get();
    }
    return nullptr;
}

Word Memory::ReadWord(Address address) const {
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

Half Memory::ReadHalf(Address address) const {
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

Byte Memory::ReadByte(Address address) const {
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

std::pair<Word, bool> Memory::PeekWord(Address address) const {
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

bool Memory::TryWriteWord(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned write of word at {:#10x}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        return false;
    
    if (!region->writable)
        return false;

    region->WriteWord(address - region->base, word);
    return true;
}

void Memory::WriteWord(Address address, Word word) {
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

void Memory::WriteHalf(Address address, Half half) {
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

void Memory::WriteByte(Address address, Byte byte) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#10x} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#10x} as it's unwritable", address));
    
    region->WriteByte(address - region->base, byte);
}

Word Memory::AtomicSwap(Address address, Word word) {
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

Word Memory::AtomicAdd(Address address, Word word) {
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

Word Memory::AtomicAnd(Address address, Word word) {
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

Word Memory::AtomicOr(Address address, Word word) {
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

Word Memory::AtomicXor(Address address, Word word) {
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

int32_t Memory::AtomicMin(Address address, int32_t word) {
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

Word Memory::AtomicMinU(Address address, Word word) {
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

int32_t Memory::AtomicMax(Address address, int32_t word) {
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

Word Memory::AtomicMaxU(Address address, Word word) {
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

void Memory::WriteWords(Address address, const std::vector<Word>& words) {
    for (Address head = address, i = 0; i < words.size(); i++, head += 4) {
        WriteWord(head, words[i]);
    }
}

std::vector<Word> Memory::ReadWords(Address address, Address count) const {
    std::vector<Word> data;
    for (Address head = address, i = 0; i < count; i++, head += 4) {
        data.push_back(ReadWord(head));
    }

    return data;
}

std::vector<std::pair<Word, bool>> Memory::PeekWords(Address address, Address count) const {
    std::vector<std::pair<Word, bool>> data;

    const MemoryRegion* region = nullptr;
    Address end;
    for (Address head = address, i = 0; i < count; i++, head += 4) {
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
                data.push_back({region->ReadWord(head - region->base), true});
            }
            else
                data.push_back({0, false});
        }
    }

    return data;
}

Word Memory::ReadWordReserved(Address address, Hart hart_id) const {
    lock.lock();
    for (auto& item : reservations) {
        if (item.second == (address & ~0b11)) {
            reservations.erase(item.first);
            break;
        }
    }
    
    reservations[hart_id] = address & ~0b11;
    lock.unlock();

    return ReadWord(address);
}

bool Memory::WriteWordConditional(Address address, Word value, Hart hart_id) {
    lock.lock();
    if (reservations.find(hart_id) == reservations.end()) {
        lock.unlock();
        return false;
    }

    if (reservations[hart_id] != (address & ~0b11)) {
        auto addr = reservations[hart_id];
        lock.unlock();
        return false;
    }

    reservations.erase(hart_id);

    lock.unlock();

    WriteWord(address, value);
    return true;
}

Address Memory::ReadFileInto(const std::string& path, Address address) {
    std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for reading", path));
    }

    Address size = file.tellg();
    Word words = size / 4;
    if (size % 4) words++;
    
    file.seekg(0);

    std::vector<Word> data(words);
    data[words - 1] = 0;

    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    WriteWords(address, data);

    return size;
}

void Memory::WriteToFile(const std::string& path, Address address, Address bytes) {
    std::ofstream file(path, std::ios_base::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for writing", path));
    }

    Address count = (bytes + 3) / 4;
    
    auto data = ReadWords(address, count);

    file.write(reinterpret_cast<char*>(data.data()), bytes);
    file.close();
}