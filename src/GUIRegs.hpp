#ifndef GUI_STATE_HPP
#define GUI_STATE_HPP

#include "VirtualMachine.hpp"

class GUIRegs {
    VirtualMachine& vm;

public:
    GUIRegs(VirtualMachine& vm) : vm{vm} {}
    ~GUIRegs() = default;

    void Draw();
};

#endif