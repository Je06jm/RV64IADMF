#include "EBreaks.hpp"

#include "VirtualMachine.hpp"

#include <iostream>
#include <string>

using VM = VirtualMachine;

void BuiltinEBreakCOut(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    std::string str = "";

    for (uint32_t addr = regs[VM::REG_A1], i = 0; i < regs[VM::REG_A2]; addr++, i++) {
        uint8_t byte = memory.Read8(addr);
        str += static_cast<const char>(byte);
    }

    std::cout << str << std::flush;
}

void BuiltinEBreakCIn(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    std::string str = "";

    if (!std::getline(std::cin, str)) {
        regs[VM::REG_A0] = 0;
        return;
    }

    uint32_t i, addr;
    for (i = 0, addr = regs[VM::REG_A1]; i < str.size() && i < regs[VM::REG_A2] && addr < memory.GetTotalMemory(); i++, addr++) {
        memory.Write8(addr, str[i]);
    }

    regs[VM::REG_A0] = i;
}

void RegisterBuiltinEBreaks() {
    VM::RegisterECall(EBREAK_COUT, BuiltinEBreakCOut);
    VM::RegisterECall(EBREAK_CIN, BuiltinEBreakCIn);
}