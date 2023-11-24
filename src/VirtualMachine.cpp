#include "VirtualMachine.hpp"

#include <format>

VirtualMachine::VirtualMachine(Memory& memory, uint32_t starting_pc, size_t instructions_per_second) : memory{memory}, pc{starting_pc}, instructions_per_second{instructions_per_second} {
    for (auto& r : regs) {
        r = 0;
    }

    for (auto& f : fregs) {
        f = 0.0;
    }
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

size_t VirtualMachine::GetInstructionsPerSecond() {
    return instructions_per_second;
}