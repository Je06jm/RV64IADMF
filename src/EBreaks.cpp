#include "EBreaks.hpp"

#include "VirtualMachine.hpp"

#include <iostream>
#include <string>

using VM = VirtualMachine;

void BuiltinEBreakPrint(Memory& memory, std::array<uint32_t, VM::REGISTER_COUNT>& regs, std::array<Float, VM::REGISTER_COUNT>&) {
    std::string str = "";

    for (uint32_t addr = regs[VM::REG_A1], i = 0; i < regs[VM::REG_A2]; addr++, i++) {
        uint8_t byte = memory.Read8(addr);
        str += static_cast<const char>(byte);
    }

    std::cout << str << std::endl;
}

void RegisterBuiltinEBreaks() {
    VM::RegisterECall(EBREAK_PRINT, BuiltinEBreakPrint);
}