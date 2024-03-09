#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

class Memory {
public:
    static constexpr size_t TOTAL_MEMORY = 0x100000000;
    static constexpr size_t PAGE_SIZE = 0x1000;
    static constexpr size_t TOTAL_PAGES = TOTAL_MEMORY / PAGE_SIZE;
    static constexpr size_t WORDS_PER_PAGE = PAGE_SIZE / sizeof(uint32_t);

private:
    using Page = std::unique_ptr<std::array<uint32_t, WORDS_PER_PAGE>>;
    std::vector<Page> pages;
    size_t loaded_pages = 0;

    void EnsureMemoryIsLoadedIn(uint32_t address);

    struct MemoryAccessCallback {
        uint32_t base;
        uint32_t size;
        std::function<void(uint32_t address, bool is_read)> callback;
    };
    std::vector<MemoryAccessCallback> memory_access_callbacks;

    mutable std::unordered_map<uint32_t, uint32_t> reservations;

    mutable std::mutex lock;

public:
    const uint64_t max_address;

    Memory(uint64_t max_address = 0x100000000);

    Memory(const Memory&) = delete;
    Memory(Memory&&) = delete;
    ~Memory() = default;

    Memory& operator=(const Memory&) = delete;
    Memory& operator=(Memory&&) = delete;

    uint32_t Read32(uint32_t address);
    uint16_t Read16(uint32_t address);
    uint8_t Read8(uint32_t address);

    void Write32(uint32_t address, uint32_t value);
    void Write16(uint32_t address, uint16_t value);
    void Write8(uint32_t address, uint8_t value);

    uint32_t AtomicSwap(uint32_t address, uint32_t value);
    uint32_t AtomicAdd(uint32_t address, uint32_t value);
    uint32_t AtomicAnd(uint32_t address, uint32_t value);
    uint32_t AtomicOr(uint32_t address, uint32_t value);
    uint32_t AtomicXor(uint32_t address, uint32_t value);
    int32_t AtomicMin(uint32_t address, int32_t value);
    uint32_t AtomicMinU(uint32_t address, uint32_t value);
    int32_t AtomicMax(uint32_t address, int32_t value);
    uint32_t AtomicMaxU(uint32_t address, uint32_t value);

    void Write(uint32_t address, const std::vector<uint32_t>& data);
    std::vector<uint32_t> Read(uint32_t address, uint32_t words);

    uint32_t Read32Reserved(uint32_t address, uint32_t cpu_id);
    bool Write32Conditional(uint32_t address, uint32_t value, uint32_t cpu_id);

    uint32_t ReadFileInto(const std::string& path, uint32_t address);
    void WriteToFile(const std::string& path, uint32_t address, uint32_t bytes);

    inline size_t GetTotalMemory() {
        return max_address;
    }

    inline size_t GetUsedMemory() {
        lock.lock();
        size_t pages = loaded_pages;
        lock.unlock();
        return pages * PAGE_SIZE;
    }

    inline void AddMemoryAccessCallback(uint32_t base, uint32_t size, std::function<void(uint32_t address, bool is_read)> callback) {
        memory_access_callbacks.push_back({ base, size, callback });
    }
};

#endif