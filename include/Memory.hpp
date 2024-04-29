#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <mutex>
#include <utility>

class MemoryRegion {
public:
    const uint32_t base, size;
    const bool readable, writable;

    MemoryRegion(uint32_t base, uint32_t size, bool readable, bool writable) : base{base}, size{size}, readable{readable}, writable{writable} {}
    virtual ~MemoryRegion() = default;

    virtual uint32_t ReadWord(uint32_t address) const { return 0; }
    virtual uint16_t ReadHalf(uint32_t address) const;
    virtual uint8_t ReadByte(uint32_t address) const;

    virtual void WriteWord(uint32_t address, uint32_t word) {}
    virtual void WriteHalf(uint32_t address, uint16_t half);
    virtual void WriteByte(uint32_t address, uint8_t byte);

    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;

    virtual size_t SizeInMemory() const { return size; }
};

class MemoryROM : public MemoryRegion {
private:
    const std::vector<uint32_t> words;

    MemoryROM(const std::vector<uint32_t>& words, uint32_t base) : MemoryRegion(base, words.size() * sizeof(uint32_t), true, false), words{words} {}
public:
    uint32_t ReadWord(uint32_t address) const override { return words[address >> 2]; }
    
    void Lock() const override {}
    void Unlock() const override {}

    static std::unique_ptr<MemoryROM> Create(const std::vector<uint32_t>& words, uint32_t base);
};

class MemoryRAM : public MemoryRegion {
public:
    static constexpr size_t PAGE_SIZE = 0x1000;
    static constexpr size_t WORDS_PER_PAGE = PAGE_SIZE / sizeof(uint32_t);

private:
    using Page = std::array<uint32_t, WORDS_PER_PAGE>;
    using PagePtr = std::unique_ptr<Page>;
    mutable std::vector<PagePtr> pages;
    mutable size_t loaded_pages = 0;

    inline void EnsurePageIsLoaded(size_t page) const {
        if (pages[page] == nullptr) {
            lock.lock();
            if (pages[page]) {
                lock.unlock();
                return;
            }

            pages[page] = std::unique_ptr<Page>(new Page);
            for (auto& word : *pages[page])
                word = 0;
            loaded_pages++;
            lock.unlock();
        }
    }

    MemoryRAM(uint32_t base, uint32_t size);

    mutable std::mutex lock;
public:
    uint32_t ReadWord(uint32_t address) const override;
    void WriteWord(uint32_t address, uint32_t word) override;

    void Lock() const override { lock.lock(); }
    void Unlock() const override { lock.unlock(); }

    size_t SizeInMemory() const override { return loaded_pages * PAGE_SIZE; }

    static std::unique_ptr<MemoryRAM> Create(uint32_t base, uint32_t size);
};

class Memory {
public:
    static constexpr size_t TOTAL_MEMORY = 0x100000000;
    static constexpr size_t PAGE_SIZE = 0x1000;
    static constexpr size_t TOTAL_PAGES = TOTAL_MEMORY / PAGE_SIZE;
    static constexpr size_t WORDS_PER_PAGE = PAGE_SIZE / sizeof(uint32_t);

private:
    std::vector<std::shared_ptr<MemoryRegion>> regions;

    mutable std::unordered_map<uint32_t, uint32_t> reservations;
    mutable std::mutex lock;

    MemoryRegion* GetMemoryRegion(uint32_t address);
    const MemoryRegion* GetMemoryRegion(uint32_t address) const;

    uint32_t max_address = 0;
    uint32_t memory_size = 0;

public:
    Memory() = default;

    Memory(const Memory&) = delete;
    Memory(Memory&&) = delete;
    ~Memory() = default;

    Memory& operator=(const Memory&) = delete;
    Memory& operator=(Memory&&) = delete;

    uint32_t ReadWord(uint32_t address) const;
    uint16_t ReadHalf(uint32_t address) const;
    uint8_t ReadByte(uint32_t address) const;

    std::pair<uint32_t, bool> PeekWord(uint32_t address) const;

    void WriteWord(uint32_t address, uint32_t word);
    void WriteHalf(uint32_t address, uint16_t half);
    void WriteByte(uint32_t address, uint8_t byte);

    uint32_t AtomicSwap(uint32_t address, uint32_t word);
    uint32_t AtomicAdd(uint32_t address, uint32_t word);
    uint32_t AtomicAnd(uint32_t address, uint32_t word);
    uint32_t AtomicOr(uint32_t address, uint32_t word);
    uint32_t AtomicXor(uint32_t address, uint32_t word);
    int32_t AtomicMin(uint32_t address, int32_t word);
    uint32_t AtomicMinU(uint32_t address, uint32_t word);
    int32_t AtomicMax(uint32_t address, int32_t word);
    uint32_t AtomicMaxU(uint32_t address, uint32_t word);

    void WriteWords(uint32_t address, const std::vector<uint32_t>& words);
    std::vector<uint32_t> ReadWords(uint32_t address, uint32_t count) const;
    std::vector<std::pair<uint32_t, bool>> PeekWords(uint32_t address, uint32_t count) const;

    uint32_t ReadWordReserved(uint32_t address, uint32_t cpu_id) const;
    bool WriteWordConditional(uint32_t address, uint32_t word, uint32_t cpu_id);

    uint32_t ReadFileInto(const std::string& path, uint32_t address);
    void WriteToFile(const std::string& path, uint32_t address, uint32_t bytes);

    inline size_t GetMaxAddress() const {
        return max_address;
    }

    inline size_t GetTotalMemory() const {
        return max_address;
    }

    inline size_t GetUsedMemory() const {
        size_t used = 0;
        for (auto& region : regions)
            used += region->SizeInMemory();
        
        return used;
    }

    template <typename T>
    void AddMemoryRegion(std::shared_ptr<T> region) {
        static_assert(std::is_base_of_v<MemoryRegion, T>);

        auto mem_region = std::shared_ptr<MemoryRegion>(region);

        uint32_t end = mem_region->base + mem_region->size;
        memory_size += mem_region->size;

        regions.emplace_back(std::move(mem_region));

        if (end > max_address)
            max_address = end;
    }

    template <typename T>
    void AddMemoryRegion(std::unique_ptr<T>&& region) {
        static_assert(std::is_base_of_v<MemoryRegion, T>);

        auto mem_region = std::shared_ptr<MemoryRegion>(region.release());

        uint32_t end = mem_region->base + mem_region->size;
        memory_size += mem_region->size;

        regions.emplace_back(std::move(mem_region));

        if (end > max_address)
            max_address = end;
    }
};

#endif