#ifndef GUI_INFO_HPP
#define GUI_INFO_HPP

#include "Memory.hpp"
#include "VirtualMachine.hpp"

#include <vector>

class GUIInfo {
    Memory& memory;

    uint32_t selected_hart = 0;
    const std::vector<uint32_t> harts;

public:
    std::shared_ptr<VirtualMachine> vm;

    GUIInfo(Memory& memory, std::shared_ptr<VirtualMachine> vm, const std::vector<uint32_t>& harts) : memory{memory}, harts{harts}, vm{vm} {}
    ~GUIInfo() = default;

    inline uint32_t GetSelectedHart() const {
        return harts[selected_hart];
    }

    void Draw();
};

#endif