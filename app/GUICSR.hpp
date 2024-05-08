#ifndef GUI_CSR_HPP
#define GUI_CSR_HPP

#include "VirtualMachine.hpp"

#include <memory>

class GUICSR {
public:
    std::shared_ptr<VirtualMachine> vm;
    
    GUICSR(std::shared_ptr<VirtualMachine> vm) : vm{vm} {}
    ~GUICSR() = default;

    void Draw();
};

#endif