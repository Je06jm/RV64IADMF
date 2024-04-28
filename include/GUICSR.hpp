#ifndef GUI_CSR_HPP
#define GUI_CSR_HPP

#include "VirtualMachine.hpp"

class GUICSR {
    VirtualMachine& vm;

public:
    GUICSR(VirtualMachine& vm) : vm{vm} {}
    ~GUICSR() = default;

    void Draw();
};

#endif