#ifndef GUI_ASSEMBLY_HPP
#define GUI_ASSEMBLY_HPP

#include "VirtualMachine.hpp"
#include "Memory.hpp"
#include "RV32I.hpp"

class GUIAssembly {
    VirtualMachine& vm;
    Memory& memory;

    static constexpr size_t WINDOW = 128;
    static constexpr size_t WINDOW_SLIDE = 12;

public:
    GUIAssembly(VirtualMachine& vm, Memory& memory) : vm{vm}, memory{memory} {}
    ~GUIAssembly() = default;

    void Draw();
};

#endif