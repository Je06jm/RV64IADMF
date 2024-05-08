#include "ECalls.hpp"

#include <VirtualMachine.hpp>

#include "Framebuffer.hpp"
#include "VirtualMachines.hpp"

#include <iostream>
#include <string>
#include <format>
#include <cstdlib>

using VM = VirtualMachine;
using Regs = std::array<Word, VM::REGISTER_COUNT>;
using FRegs = std::array<Float, VM::REGISTER_COUNT>;

void ECallCOut(Hart, Memory& memory, Regs& regs, FRegs&) {
    std::string str = "";

    for (Address addr = regs[VM::REG_A1], i = 0; i < regs[VM::REG_A2]; addr++, i++) {
        Byte byte = memory.ReadByte(addr);
        str += static_cast<char>(byte);
    }

    std::cout << str << std::flush;
}

void ECallCIn(Hart, Memory& memory, Regs& regs, FRegs&) {
    std::string str = "";

    if (!std::getline(std::cin, str)) {
        regs[VM::REG_A0] = 0;
        return;
    }

    Address i, addr;
    for (i = 0, addr = regs[VM::REG_A1]; i < str.size() && i < regs[VM::REG_A2] && addr < memory.GetTotalMemory(); i++, addr++) {
        memory.WriteByte(addr, str[i]);
    }

    regs[VM::REG_A0] = i;
}

void ECallStartCPU(Hart hart, Memory&, Regs& regs, FRegs&) {
    Hart target_hart = regs[VM::REG_A1];
    if (target_hart > vms.size()) {
        std::cerr << std::format("Unknown hart {}", regs[target_hart]) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    vms[target_hart]->Restart(regs[VM::REG_A2], hart);
}

void ECallGetCPUs(Hart, Memory& memory, Regs& regs, FRegs&) {
    if (regs[VM::REG_A1] != 0) {
        for (Word i = 0; i < vms.size(); i++)
            memory.WriteWord(regs[VM::REG_A1] + i * sizeof(Word), i);
    }

    regs[VM::REG_A0] = static_cast<Word>(vms.size());
}

void ECallGetScreenAddress(Hart, Memory&, Regs& regs, FRegs&) {
    regs[VM::REG_A0] = framebuffer_address;
}

void ECallGetScreenSize(Hart, Memory& memory, Regs& regs, FRegs&) {
    memory.WriteWord(static_cast<Address>(regs[VM::REG_A1]), framebuffer_width);
    memory.WriteWord(static_cast<Address>(regs[VM::REG_A2]), framebuffer_height);
}

void ECallGetMemorySize(Hart, Memory& memory, Regs& regs, FRegs&) {
    regs[VM::REG_A0] = memory.GetTotalMemory();
}

void ECallExit(Hart, Memory&, Regs& regs, FRegs&) {
    union S32U32 {
        Word u;
        SWord s;
    };

    S32U32 value;
    value.u = regs[VM::REG_A1];
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