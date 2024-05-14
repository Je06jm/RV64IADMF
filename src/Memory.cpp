#include "Memory.hpp"

#include <stdexcept>
#include <format>
#include <fstream>

Word MemoryRegion::ReadWord(Address address) const {
    auto vlong = ReadLong(address & ~7);
    return static_cast<Long>(vlong >> ((address & 4) * 8));
}

Half MemoryRegion::ReadHalf(Address address) const {
    auto vlong = ReadLong(address & ~7);
    return static_cast<Half>(vlong >> ((address & 6) * 8));
}

Byte MemoryRegion::ReadByte(Address address) const {
    auto vlong = ReadLong(address & ~7);
    return static_cast<Byte>(vlong >> ((address & 7) * 8));
}

void MemoryRegion::WriteWord(Address address, Word word) {
    auto vlong = ReadLong(address & ~7);
    auto shift = (address & 4) * 8;
    vlong &= ~(0xffffffffLL << shift);
    vlong |= static_cast<Long>(word) << shift;
    WriteLong(address, vlong);
}

void MemoryRegion::WriteHalf(Address address, Half half) {
    auto vlong = ReadLong(address & ~3);
    auto shift = (address & 6) * 8;
    vlong &= ~(0xffffLL << shift);
    vlong |= static_cast<Long>(half) << shift;
    WriteLong(address & ~3, vlong);
}

void MemoryRegion::WriteByte(Address address, Byte byte) {
    auto vlong = ReadLong(address & ~3);
    auto shift = (address & 7) * 8;
    vlong &= ~(0xffLL << shift);
    vlong |= static_cast<Long>(byte) << shift;
    WriteLong(address & ~3, vlong);
}

std::unique_ptr<MemoryROM> MemoryROM::Create(const std::vector<Long>& longs, Address base) {
    return std::unique_ptr<MemoryROM>(new MemoryROM(longs, base & ~7));
}

MemoryRAM::MemoryRAM(Address base, Address size) : MemoryRegion(TYPE_GENERAL_RAM, 0, base, size, true, true) {
    size_t pages_count = size / LONGS_PER_PAGE;

    for (size_t i = 0; i < pages_count; i++)
        pages.push_back(nullptr);
}

Long MemoryRAM::ReadLong(Address address) const {
    size_t page = address / LONGS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    return (*pages[page])[address >> 3];
}

void MemoryRAM::WriteLong(Address address, Long vlong) {
    size_t page = address / LONGS_PER_PAGE;
    address %= PAGE_SIZE;

    EnsurePageIsLoaded(page);

    (*pages[page])[address >> 3] = vlong;
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

Long Memory::ReadLong(Address address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned read of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#18} as it's unreadable", address));

    auto vlong = region->ReadLong(address - region->base);

    return vlong;
}

Word Memory::ReadWord(Address address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned read of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#18} as it's unreadable", address));

    auto word = region->ReadWord(address - region->base);

    return word;
}

Half Memory::ReadHalf(Address address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 1)
        throw std::runtime_error(std::format("Unaligned read of half at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#18} as it's unreadable", address));

    auto half = region->ReadHalf(address - region->base);

    return half;
}

Byte Memory::ReadByte(Address address) const {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot read address {:#18} as it's unreadable", address));

    auto byte = region->ReadByte(address - region->base);

    return byte;
}

std::pair<Word, bool> Memory::PeekWord(Address address) const {
    if (address & 3)
        throw std::runtime_error(std::format("Unaligned read of word at {:#18}", address));

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
        throw std::runtime_error(std::format("Unaligned write of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        return false;
    
    if (!region->writable)
        return false;

    region->WriteWord(address - region->base, word);
    return true;
}

void Memory::WriteLong(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned write of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#18} as it's unwritable", address));

    region->WriteLong(address - region->base, vlong);
}

void Memory::WriteWord(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned write of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#18} as it's unwritable", address));

    region->WriteWord(address - region->base, word);
}

void Memory::WriteHalf(Address address, Half half) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 1)
        throw std::runtime_error(std::format("Unaligned write of half at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#18} as it's unwritable", address));
    
    region->WriteHalf(address - region->base, half);
}

void Memory::WriteByte(Address address, Byte byte) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot write address {:#18} as it's unwritable", address));
    
    region->WriteByte(address - region->base, byte);
}

Long Memory::AtomicSwapL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    region->WriteLong(address - region->base, vlong);
    region->Lock();

    return old_long;
}

Long Memory::AtomicAddL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    region->WriteLong(address - region->base, old_long + vlong);
    region->Unlock();

    return old_long;
}

Long Memory::AtomicAndL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    region->WriteLong(address - region->base, old_long & vlong);
    region->Unlock();

    return old_long;
}

Long Memory::AtomicOrL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    region->WriteLong(address - region->base, old_long | vlong);
    region->Unlock();

    return old_long;
}

Long Memory::AtomicXorL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    region->WriteLong(address - region->base, old_long ^ vlong);
    region->Unlock();

    return old_long;
}

SLong Memory::AtomicMinL(Address address, SLong vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    v.u = old_long;
    if (vlong < v.s) v.s = vlong;
    region->WriteLong(address - region->base, v.u);
    region->Unlock();

    return old_long;
}

Long Memory::AtomicMinUL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    if (vlong < old_long) region->WriteLong(address - region->base, vlong);
    region->Unlock();

    return old_long;
}

SLong Memory::AtomicMaxL(Address address, SLong vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    v.u = old_long;
    if (vlong > v.s) v.s = vlong;
    region->WriteLong(address - region->base, v.u);
    region->Unlock();

    return old_long;
}

Long Memory::AtomicMaxUL(Address address, Long vlong) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 7)
        throw std::runtime_error(std::format("Unaligned atomic of long at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_long = region->ReadLong(address - region->base);
    if (vlong > old_long) region->WriteLong(address - region->base, vlong);
    region->Unlock();

    return old_long;
}

Word Memory::AtomicSwapW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadLong(address - region->base);
    region->WriteWord(address - region->base, word);
    region->Lock();

    return old_word;
}

Word Memory::AtomicAddW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word + word);
    region->Unlock();

    return old_word;
}

Word Memory::AtomicAndW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word & word);
    region->Unlock();

    return old_word;
}

Word Memory::AtomicOrW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word | word);
    region->Unlock();

    return old_word;
}

Word Memory::AtomicXorW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    region->WriteWord(address - region->base, old_word ^ word);
    region->Unlock();

    return old_word;
}

int32_t Memory::AtomicMinW(Address address, int32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    v.u = old_word;
    if (word < v.s) v.s = word;
    region->WriteWord(address - region->base, v.u);
    region->Unlock();

    return old_word;
}

Word Memory::AtomicMinUW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    if (word < old_word) region->WriteWord(address - region->base, word);
    region->Unlock();

    return old_word;
}

int32_t Memory::AtomicMaxW(Address address, int32_t word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    U32S32 v;

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    v.u = old_word;
    if (word > v.s) v.s = word;
    region->WriteWord(address - region->base, v.u);
    region->Unlock();

    return old_word;
}

Word Memory::AtomicMaxUW(Address address, Word word) {
    if (address >= max_address)
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));

    if (address & 3)
        throw std::runtime_error(std::format("Unaligned atomic of word at {:#18}", address));

    auto region = GetMemoryRegion(address);

    if (!region)
        throw std::runtime_error(std::format("Address {:#18} is not mapped to any memory", address));
    
    if (!region->readable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unreadable", address));
    
    if (!region->writable)
        throw std::runtime_error(std::format("Cannot atomic address {:#18} as it's unwritable", address));

    region->Lock();
    auto old_word = region->ReadWord(address - region->base);
    if (word > old_word) region->WriteWord(address - region->base, word);
    region->Unlock();

    return old_word;
}

void Memory::WriteLongs(Address address, const std::vector<Long>& longs) {
    for (Address head = address, i = 0; i < longs.size(); i++, head += 8) {
        WriteLong(head, longs[i]);
    }
}

void Memory::WriteWords(Address address, const std::vector<Word>& words) {
    for (Address head = address, i = 0; i < words.size(); i++, head += 4) {
        WriteWord(head, words[i]);
    }
}

std::vector<Long> Memory::ReadLongs(Address address, Address count) const {
    std::vector<Long> data;
    for (Address head = address, i = 0; i < count; i++, head += 8) {
        data.push_back(ReadLong(head));
    }

    return data;
}

std::vector<Word> Memory::ReadWords(Address address, Address count) const {
    std::vector<Word> data;
    for (Address head = address, i = 0; i < count; i++, head += 4) {
        data.push_back(ReadWord(head));
    }

    return data;
}

std::vector<std::pair<Long, bool>> Memory::PeekLongs(Address address, Address count) const {
    std::vector<std::pair<Long, bool>> data;

    const MemoryRegion* region = nullptr;
    Address end;
    for (Address head = address, i = 0; i < count; i++, head += 8) {
        if (!region) {
            region = GetMemoryRegion(head);
            if (region) {
                end = region->base + region->size;
                data.push_back({region->ReadLong(head - region->base), true});
            }
            else
                data.push_back({0, false});
        }
        else {
            if (!(head >= region->base && head < end))
                region = GetMemoryRegion(head);
            
            if (region) {
                data.push_back({region->ReadLong(head - region->base), true});
            }
            else
                data.push_back({0, false});
        }
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

Long Memory::ReadLongReserved(Address address, Hart hart_id) const {
    lock.lock();
    for (auto& item : reservations) {
        if (item.second == (address & ~0b11)) {
            reservations.erase(item.first);
            break;
        }
    }
    
    reservations[hart_id] = address & ~0b11;
    lock.unlock();

    return ReadLong(address);
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

bool Memory::WriteLongConditional(Address address, Long value, Hart hart_id) {
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

    WriteLong(address, value);
    return true;
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
    Word longs = (size + 7) / 8;
    
    file.seekg(0);

    std::vector<Long> data(longs);
    data[longs - 1] = 0;

    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();

    WriteLongs(address, data);

    return size;
}

void Memory::WriteToFile(const std::string& path, Address address, Address bytes) {
    std::ofstream file(path, std::ios_base::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for writing", path));
    }

    Address count = (bytes + 7) / 8;
    
    auto data = ReadLongs(address, count);

    file.write(reinterpret_cast<char*>(data.data()), bytes);
    file.close();
}