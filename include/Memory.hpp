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

#include "Types.hpp"

class MemoryRegion {
public:
    const Word type;
    const Word flags;
    const Address base, size;
    const bool readable, writable;

    static constexpr Word TYPE_UNKNOWN = -1U;
    static constexpr Word TYPE_UNUSED = 0;
    static constexpr Word TYPE_PMA_ROM = 1;
    static constexpr Word TYPE_MAPPED_CSRS = 2;
    static constexpr Word TYPE_BIOS_ROM = 4;
    static constexpr Word TYPE_GENERAL_RAM = 5;
    static constexpr Word TYPE_FRAMEBUFFER = 8;

    MemoryRegion(Word type, Word flags, Address base, Address size, bool readable, bool writable) : type{type}, flags{flags}, base{base}, size{size}, readable{readable}, writable{writable} {}
    virtual ~MemoryRegion() = default;

    virtual Long ReadLong(Address) const { return 0; }
    virtual Word ReadWord(Address) const;
    virtual Half ReadHalf(Address) const;
    virtual Byte ReadByte(Address) const;

    virtual void WriteLong(Address, Long) {}
    virtual void WriteWord(Address, Word);
    virtual void WriteHalf(Address, Half);
    virtual void WriteByte(Address, Byte);

    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;

    virtual Long SizeInMemory() const { return size; }
};

class MemoryROM : public MemoryRegion {
private:
    const std::vector<Long> longs;

    MemoryROM(const std::vector<Long>& longs, Address base) : MemoryRegion(TYPE_BIOS_ROM, 0, base, longs.size() * sizeof(Long), true, false), longs{longs} {}
public:
    Long ReadLong(Address address) const override { return longs[address >> 3]; }
    
    void Lock() const override {}
    void Unlock() const override {}

    static std::unique_ptr<MemoryROM> Create(const std::vector<Long>& longs, Address base);
};

class MemoryRAM : public MemoryRegion {
public:
    static constexpr Long PAGE_SIZE = 0x1000;
    static constexpr Long LONGS_PER_PAGE = PAGE_SIZE / sizeof(Long);

private:
    using Page = std::array<Long, LONGS_PER_PAGE>;
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

    MemoryRAM(Address base, Address size);

    mutable std::mutex lock;
public:
    Long ReadLong(Address address) const override;
    void WriteLong(Address address, Long vlong) override;

    void Lock() const override { lock.lock(); }
    void Unlock() const override { lock.unlock(); }

    Long SizeInMemory() const override { return loaded_pages * PAGE_SIZE; }

    static std::unique_ptr<MemoryRAM> Create(Address base, Address size);
};

class Memory {
public:
    static constexpr Long TOTAL_MEMORY = 0x100000000;
    static constexpr Long PAGE_SIZE = 0x1000;
    static constexpr Long TOTAL_PAGES = TOTAL_MEMORY / PAGE_SIZE;
    static constexpr Long WORDS_PER_PAGE = PAGE_SIZE / sizeof(Word);

private:
    std::vector<std::shared_ptr<MemoryRegion>> regions;

    mutable std::unordered_map<Hart, Address> reservations;
    mutable std::mutex lock;

    MemoryRegion* GetMemoryRegion(Address address);
    const MemoryRegion* GetMemoryRegion(Address address) const;

    Address max_address = 0;
    Address memory_size = 0;

    class MemoryPMARom : public MemoryRegion {
    private:
        const std::vector<std::shared_ptr<MemoryRegion>>& regions;

    public:
        MemoryPMARom(std::vector<std::shared_ptr<MemoryRegion>>& regions) : MemoryRegion{TYPE_PMA_ROM, 0, 0, 0x200, true, false}, regions{regions} {}

        Long ReadLong(Address address) const override {
            address >>= 3;
            auto index = address >> 2;
            if (index >= regions.size()) return 0;

            auto region = regions[index];

            switch (address & 3) {
                case 0:
                    return region->type;
                
                case 1:
                    return region->base;
                
                case 2:
                    return region->size;
                
                case 3: {
                    Word flags = 0;

                    if (region->readable) flags |= (1<<0);
                    if (region->writable) flags |= (1<<1);

                    return flags;
                }
            }

            return 0;
        }

        void Lock() const override {}
        void Unlock() const override {}
        Long SizeInMemory() const { return sizeof(std::vector<std::shared_ptr<MemoryRegion>>&); }
    };
public:
    Memory() {
        auto pma = std::make_unique<MemoryPMARom>(regions);
        AddMemoryRegion(std::move(pma));
    };

    Memory(const Memory&) = delete;
    Memory(Memory&&) = delete;
    ~Memory() = default;

    Memory& operator=(const Memory&) = delete;
    Memory& operator=(Memory&&) = delete;

    Long ReadLong(Address address) const;
    Word ReadWord(Address address) const;
    Half ReadHalf(Address address) const;
    Byte ReadByte(Address address) const;

    std::pair<Long, bool> PeekLong(Address address) const;
    std::pair<Word, bool> PeekWord(Address address) const;
    bool TryWriteLong(Address address, Long vlong);
    bool TryWriteWord(Address address, Word word);

    void WriteLong(Address address, Long vlong);
    void WriteWord(Address address, Word word);
    void WriteHalf(Address address, Half half);
    void WriteByte(Address address, Byte byte);

    Long AtomicSwapL(Address address, Long vlong);
    Long AtomicAddL(Address address, Long vlong);
    Long AtomicAndL(Address address, Long vlong);
    Long AtomicOrL(Address address, Long vlong);
    Long AtomicXorL(Address address, Long vlong);
    SLong AtomicMinL(Address address, SLong vlong);
    Long AtomicMinUL(Address address, Long vlong);
    SLong AtomicMaxL(Address address, SLong vlong);
    Long AtomicMaxUL(Address address, Long vlong);

    Word AtomicSwapW(Address address, Word word);
    Word AtomicAddW(Address address, Word word);
    Word AtomicAndW(Address address, Word word);
    Word AtomicOrW(Address address, Word word);
    Word AtomicXorW(Address address, Word word);
    SWord AtomicMinW(Address address, SWord word);
    Word AtomicMinUW(Address address, Word word);
    SWord AtomicMaxW(Address address, SWord word);
    Word AtomicMaxUW(Address address, Word word);

    void WriteLongs(Address address, const std::vector<Long>& longs);
    void WriteWords(Address address, const std::vector<Word>& words);

    std::vector<Long> ReadLongs(Address address, Address const) const;
    std::vector<Word> ReadWords(Address address, Address count) const;

    std::vector<std::pair<Long, bool>> PeekLongs(Address address, Address cont) const;
    std::vector<std::pair<Word, bool>> PeekWords(Address address, Address count) const;

    Long ReadLongReserved(Address address, Hart hart_id) const;
    Word ReadWordReserved(Address address, Hart hart_id) const;

    bool WriteLongConditional(Address address, Long vlong, Hart hart_id);
    bool WriteWordConditional(Address address, Word word, Hart hart_id);

    Address ReadFileInto(const std::string& path, Address address);
    void WriteToFile(const std::string& path, Address address, Address bytes);

    inline Address GetMaxAddress() const {
        return max_address;
    }

    inline Address GetTotalMemory() const {
        Address total_memory = 0;
        for (auto& region : regions)
            total_memory += region->size;
        
        return total_memory;
    }

    inline Address GetUsedMemory() const {
        Address used = 0;
        for (auto& region : regions)
            used += region->SizeInMemory();
        
        return used;
    }

    template <typename T>
    void AddMemoryRegion(std::shared_ptr<T> region) {
        static_assert(std::is_base_of_v<MemoryRegion, T>);

        auto mem_region = std::shared_ptr<MemoryRegion>(region);

        Address end = mem_region->base + mem_region->size;
        memory_size += mem_region->size;

        regions.emplace_back(std::move(mem_region));

        if (end > max_address)
            max_address = end;
    }

    template <typename T>
    void AddMemoryRegion(std::unique_ptr<T>&& region) {
        static_assert(std::is_base_of_v<MemoryRegion, T>);

        auto mem_region = std::shared_ptr<MemoryRegion>(region.release());

        Address end = mem_region->base + mem_region->size;
        memory_size += mem_region->size;

        regions.emplace_back(std::move(mem_region));

        if (end > max_address)
            max_address = end;
    }
};

#endif