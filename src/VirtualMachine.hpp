#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

#include "Memory.hpp"

#include <cstdint>
#include <array>
#include <thread>
#include <mutex>
#include <memory>
#include <string>

class VirtualMachine {
public:
    static constexpr size_t REGISTER_COUNT = 32;

private:
    std::array<uint32_t, REGISTER_COUNT> regs;
    std::array<float, REGISTER_COUNT> fregs;

    Memory& memory;

    uint32_t pc;

    bool running = true;
    std::string err = "";
    std::mutex lock;

    std::unique_ptr<std::thread> vm_thread = nullptr;
public:
    const size_t instructions_per_second;
    VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second);
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine(VirtualMachine&&) = delete;
    ~VirtualMachine();
    
    void Start();
    bool IsRunning();
    void Stop();

    void GetSnapshot(std::array<uint32_t, REGISTER_COUNT>& registers, std::array<float, REGISTER_COUNT>& fregisters, uint32_t& pc);

    size_t GetInstructionsPerSecond();
};

#endif