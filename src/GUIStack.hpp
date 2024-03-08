#ifndef GUI_STACK_HPP
#define GUI_STACK_HPP

#include "VirtualMachine.hpp"
#include "Memory.hpp"

class GUIStack {
    VirtualMachine& vm;
    Memory& memory;

    static constexpr size_t WINDOW = 128;

public:
    GUIStack(VirtualMachine& vm, Memory& memory) : vm{vm}, memory{memory} {}
    ~GUIStack() = default;

    void Draw();
};

#endif