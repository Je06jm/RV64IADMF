#ifndef GUI_INFO_HPP
#define GUI_INFO_HPP

#include "Memory.hpp"
#include "VirtualMachine.hpp"

#include <vector>

class GUIInfo {
    Memory& memory;

public:
    std::shared_ptr<VirtualMachine> vm;

    GUIInfo(Memory& memory, std::shared_ptr<VirtualMachine> vm) : memory{memory}, vm{vm} {}
    ~GUIInfo() = default;

    void Draw();
};

#endif