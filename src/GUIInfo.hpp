#ifndef GUI_INFO_HPP
#define GUI_INFO_HPP

#include "Memory.hpp"
#include "VirtualMachine.hpp"

class GUIInfo {
    Memory& memory;
    VirtualMachine& vm;

public:
    GUIInfo(Memory& memory, VirtualMachine& vm) : memory{memory}, vm{vm} {}
    ~GUIInfo() = default;

    void Draw();
};

#endif