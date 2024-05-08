#ifndef GUI_ASSEMBLY_HPP
#define GUI_ASSEMBLY_HPP

#include "VirtualMachine.hpp"
#include "Memory.hpp"
#include "RV32I.hpp"

#include <memory>

class GUIAssembly {
    Memory& memory;

    Address last_pc = 0;

    static constexpr size_t WINDOW = 128;
    static constexpr size_t WINDOW_SLIDE = 12;

public:
    std::shared_ptr<VirtualMachine> vm;

    GUIAssembly(std::shared_ptr<VirtualMachine> vm, Memory& memory) : memory{memory}, vm{vm} {}
    ~GUIAssembly() = default;

    void Draw();
};

#endif