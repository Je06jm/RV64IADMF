#include "Memory.hpp"

#include <stdexcept>
#include <format>
#include <fstream>

Memory::Memory(uint64_t max_address) : max_address{max_address} {
    for (size_t i = 0; i < TOTAL_PAGES; i++) {
        pages.push_back(nullptr);
    }
}

void Memory::EnsureMemoryIsLoadedIn(uint32_t address) {
    auto page = (address & ~0b11) / PAGE_SIZE;

    lock.lock();
    if (pages[page] == nullptr) {
        pages[page] = std::make_unique<std::array<uint32_t, WORDS_PER_PAGE>>();
        loaded_pages++;
    }
    lock.unlock();
}

uint32_t Memory::Read32(uint32_t address) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));
    }

    EnsureMemoryIsLoadedIn(address);

    if (address & 0b11) {
        throw std::runtime_error(std::format("Invalid memory location for a 32 bit read {:#010x}", address));
    }
    
    for (auto& callbacks : memory_access_callbacks) {
        if (address >= callbacks.base && address < (callbacks.base + callbacks.size)) {
            callbacks.callback(address, true);
        }
    }

    auto page = address / PAGE_SIZE;
    auto index = (address % PAGE_SIZE) / sizeof(uint32_t);
    
    lock.lock();
    auto data = (*pages[page])[index];
    lock.unlock();
    return data;
}

uint16_t Memory::Read16(uint32_t address) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));
    }

    if (address & 0b1) {
        throw std::runtime_error(std::format("Invalid memory location for a 16 bit read {:#010x}", address));
    }

    auto data = Read32(address & ~0b11);

    if (address & 0b10) return data >> 16;
    return data;
}

uint8_t Memory::Read8(uint32_t address) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried reading from memory past max_address"));
    }

    auto data = Read32(address & ~0b11);

    return data >> (8 & (address & 0b11));
}

void Memory::Write32(uint32_t address, uint32_t data) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried writing to memory past max_address"));
    }

    EnsureMemoryIsLoadedIn(address);

    for (auto& callbacks : memory_access_callbacks) {
        if (address >= callbacks.base && address < (callbacks.base + callbacks.size)) {
            callbacks.callback(address, false);
        }
    }

    if (address & 0b11) {
        throw std::runtime_error(std::format("Invalid memory location for a 32 bit write {:#010x}", address));
    }

    auto page = address / PAGE_SIZE;
    auto index = (address % PAGE_SIZE) / sizeof(uint32_t);

    lock.lock();
    (*pages[page])[index] = data;
    lock.unlock();
}

void Memory::Write16(uint32_t address, uint16_t data) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried writing to memory past max_address"));
    }

    if (address & 0b1) {
        throw std::runtime_error(std::format("Invalid memory location for a 16 bit write {:#010x}", address));
    }

    auto existing_data = Read32(address & ~0b11);

    if (address & 0b10) {
        existing_data &= 0x0000ffff;
        existing_data |= static_cast<uint32_t>(data) << 16;
    } else {
        existing_data &= 0xffff0000;
        existing_data |= data;
    }

    Write32(address & ~0b11, existing_data);
}

void Memory::Write8(uint32_t address, uint8_t data) {
    if (address >= max_address) {
        throw std::runtime_error(std::format("Tried writing to memory past max_address"));
    }

    auto existing_data = Read32(address & ~0b11);

    uint32_t mask = 0xff << (8 * (address & 0b11));
    existing_data &= ~mask;
    existing_data |= data << (8 * (address & 0b11));

    Write32(address & ~0b11, existing_data);
}

void Memory::Write(uint32_t address, const std::vector<uint32_t>& data) {
    for (uint32_t head = address, i = 0; i < data.size(); i++) {
        Write32(head, data[i]);
        head += 4;
    }
}

std::vector<uint32_t> Memory::Read(uint32_t address, uint32_t words) {
    std::vector<uint32_t> data;
    for (uint32_t head = address, i = 0; i < words; i++) {
        data.push_back(Read32(head));
        head += 4;
    }

    return data;
}

uint32_t Memory::Read32Reserved(uint32_t address, uint32_t cpu_id) {
    lock.lock();
    for (auto& item : reservations) {
        if (item.second == (address & ~0b11)) {
            reservations.erase(item.first);
            break;
        }
    }
    
    reservations[cpu_id] = address & ~0b11;
    lock.unlock();

    return Read32(address);
}

bool Memory::Write32Conditional(uint32_t address, uint32_t value, uint32_t cpu_id) {
    lock.lock();
    if (reservations.find(cpu_id) == reservations.end()) {
        lock.unlock();
        return false;
    }

    if (reservations[cpu_id] != (address & ~0b11)) {
        lock.unlock();
        return false;
    }

    reservations.erase(cpu_id);

    lock.unlock();

    Write32(address, value);
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

    Write(address, data);

    return size;
}

void Memory::WriteToFile(const std::string& path, uint32_t address, uint32_t bytes) {
    std::ofstream file(path, std::ios_base::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open {} for writing", path));
    }

    uint32_t words = bytes / 4;
    if (bytes % 4) words++;
    
    auto data = Read(address, words);

    file.write(reinterpret_cast<char*>(data.data()), bytes);
    file.close();
}