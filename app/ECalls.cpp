#include "ECalls.hpp"

#include <VirtualMachine.hpp>

#include "Framebuffer.hpp"
#include "VirtualMachines.hpp"

#include <iostream>
#include <string>
#include <format>
#include <cstdlib>

using VM = VirtualMachine;
using Regs = std::array<VM::Reg, VM::REGISTER_COUNT>;
using FRegs = std::array<Float, VM::REGISTER_COUNT>;

void ECallCOut(Hart, bool is_32_bit_mode, Memory& memory, Regs& regs, FRegs&) {
    std::string str = "";

    Address addr;
    if (is_32_bit_mode) addr = regs[VM::REG_A1].u32;
    else addr = regs[VM::REG_A1].u64;

    Address size;
    if (is_32_bit_mode) size = regs[VM::REG_A2].u32;
    else size = regs[VM::REG_A2].u64;

    std::vector<Byte> buffer_str;
    buffer_str.resize(size, 0);
    memory.PeekWords

    for (Address i = 0; i < size; addr++, i++) {
        auto [valid, byte] = memory.ReadByte
        Byte byte = memory.ReadByte(addr);
        str += static_cast<char>(byte);
    }

    std::cout << str << std::flush;
}

void ECallCIn(Hart, bool is_32_bit_mode, Memory& memory, Regs& regs, FRegs&) {
    std::string str = "";

    if (!std::getline(std::cin, str)) {
        if (is_32_bit_mode) regs[VM::REG_A0].u32 = 0;
        else regs[VM::REG_A0].u64 = 0;
        return;
    }

    Address i, addr;

    if (is_32_bit_mode) addr = regs[VM::REG_A1].u32;
    else addr = regs[VM::REG_A1].u64;

    Address size;

    if (is_32_bit_mode) size = regs[VM::REG_A2].u32;
    else size = regs[VM::REG_A2].u64;

    for (i = 0, addr = addr; i < str.size() && i < size && addr < memory.GetTotalMemory(); i++, addr++) {
        memory.WriteByte(addr, str[i]);
    }

    if (is_32_bit_mode) regs[VM::REG_A0].u32 = static_cast<uint32_t>(i);
    else regs[VM::REG_A0].u64 = i;
}

void ECallGetKey(Hart, bool, Memory&, Regs& regs, FRegs&) {
    
}

void ECallStartCPU(Hart hart, bool is_32_bit_mode, Memory&, Regs& regs, FRegs&) {
    Hart target_hart;

    if (is_32_bit_mode) target_hart = regs[VM::REG_A1].u32;
    else target_hart = regs[VM::REG_A1].u64;
    
    if (target_hart > vms.size()) {
        std::cerr << std::format("Unknown hart {}", target_hart) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Address start;

    if (is_32_bit_mode) start = regs[VM::REG_A2].u32;
    else start = regs[VM::REG_A2].u64;

    vms[target_hart]->Restart(start, hart);
}

void ECallGetCPUs(Hart, bool is_32_bit_mode, Memory& memory, Regs& regs, FRegs&) {
    Address address;

    if (is_32_bit_mode) address = regs[VM::REG_A1].u32;
    else address = regs[VM::REG_A1].u64;

    if (address != 0) {
        for (Long i = 0; i < vms.size(); i++)
            memory.WriteWord(address + i * sizeof(Hart), i);
    }

    if (is_32_bit_mode) regs[VM::REG_A0].u32 = static_cast<Word>(vms.size());
    else regs[VM::REG_A0].u64 = static_cast<Long>(vms.size());
}

void ECallGetScreenAddress(Hart, bool is_32_bit_mode, Memory&, Regs& regs, FRegs&) {
    if (is_32_bit_mode) regs[VM::REG_A0].u32 = static_cast<Word>(framebuffer_address);
    else regs[VM::REG_A0].u64 = framebuffer_address;
}

void ECallGetScreenSize(Hart, bool is_32_bit_mode, Memory& memory, Regs& regs, FRegs&) {
    Address width_address, height_address;
    
    if (is_32_bit_mode) {
        width_address = regs[VM::REG_A1].u32;
        height_address = regs[VM::REG_A2].u32;
    }
    else {
        width_address = regs[VM::REG_A1].u64;
        height_address = regs[VM::REG_A2].u64;
    }

    memory.WriteWord(static_cast<Address>(width_address), framebuffer_width);
    memory.WriteWord(static_cast<Address>(height_address), framebuffer_height);
}

void ECallGetMemorySize(Hart, bool is_32_bit_mode, Memory& memory, Regs& regs, FRegs&) {
    if (is_32_bit_mode) regs[VM::REG_A0].u32 = static_cast<Word>(memory.GetTotalMemory());
    else regs[VM::REG_A0].u64 = memory.GetTotalMemory();
}

void ECallExit(Hart, bool, Memory&, Regs& regs, FRegs&) {
    union S32U32 {
        Word u;
        SWord s;
    };

    S32U32 value;
    value.u = regs[VM::REG_A1].u32;
    std::exit(value.s);
}

void RegisterECalls() {
    VM::RegisterECall(ECALL_COUT, ECallCOut);
    VM::RegisterECall(ECALL_CIN, ECallCIn);
    VM::RegisterECall(ECALL_START_CPU, ECallStartCPU);
    VM::RegisterECall(ECALL_GET_CPUS, ECallGetCPUs);
    VM::RegisterECall(ECALL_GET_SCREEN_ADDRESS, ECallGetScreenAddress);
    VM::RegisterECall(ECALL_GET_SCREEN_SIZE, ECallGetScreenSize);
    VM::RegisterECall(ECALL_GET_MEMORY_SIZE, ECallGetMemorySize);
    VM::RegisterECall(ECALL_EXIT, ECallExit);
}