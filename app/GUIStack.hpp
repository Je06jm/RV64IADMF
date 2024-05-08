#ifndef GUI_STACK_HPP
#define GUI_STACK_HPP

#include "VirtualMachine.hpp"
#include "Memory.hpp"

class GUIStack {
    Memory& memory;

    static constexpr size_t WINDOW = 128;

public:
    std::shared_ptr<VirtualMachine> vm;
    GUIStack(std::shared_ptr<VirtualMachine> vm, Memory& memory) : memory{memory}, vm{vm} {}
    ~GUIStack() = default;

    void Draw();
};

#endif