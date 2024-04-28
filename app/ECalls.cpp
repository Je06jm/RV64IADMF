#include "ECalls.hpp"

#include "VirtualMachine.hpp"

#include <iostream>
#include <string>
#include <cstdlib>

using VM = VirtualMachine;

void ECallCOut(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    std::string str = "";

    for (uint32_t addr = regs[VM::REG_A1], i = 0; i < regs[VM::REG_A2]; addr++, i++) {
        uint8_t byte = memory.ReadByte(addr);
        str += static_cast<char>(byte);
    }

    std::cout << str << std::flush;
}

void ECallCIn(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    std::string str = "";

    if (!std::getline(std::cin, str)) {
        regs[VM::REG_A0] = 0;
        return;
    }

    uint32_t i, addr;
    for (i = 0, addr = regs[VM::REG_A1]; i < str.size() && i < regs[VM::REG_A2] && addr < memory.GetTotalMemory(); i++, addr++) {
        memory.WriteByte(addr, str[i]);
    }

    regs[VM::REG_A0] = i;
}

void ECallGetMemorySize(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    regs[VM::REG_A0] = memory.GetTotalMemory();
}

void ECallExit(Memory&, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    union S32U32 {
        uint32_t u;
        int32_t s;
    };

    S32U32 value;
    value.u = regs[VM::REG_A1];
    std::exit(value.s);
}

void RegisterECalls() {
    VM::RegisterECall(ECALL_COUT, ECallCOut);
    VM::RegisterECall(ECALL_CIN, ECallCIn);
    VM::RegisterECall(ECALL_GET_MEMORY_SIZE, ECallGetMemorySize);
    VM::RegisterECall(ECALL_EXIT, ECallExit);
}