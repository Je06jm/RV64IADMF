#ifndef GUI_STATE_HPP
#define GUI_STATE_HPP

#include "VirtualMachine.hpp"

class GUIRegs {
public:
    std::shared_ptr<VirtualMachine> vm;
    
    GUIRegs(std::shared_ptr<VirtualMachine> vm) : vm{vm} {}
    ~GUIRegs() = default;

    void Draw();
};

#endif