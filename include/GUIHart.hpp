#ifndef GUI_HART_HPP
#define GUI_HART_HPP

#include "VirtualMachine.hpp"

class GUIHart {
    uint32_t selected_hart = 0;
    const std::vector<uint32_t> harts;
    
public:
    std::shared_ptr<VirtualMachine> vm;

    GUIHart(std::shared_ptr<VirtualMachine> vm, const std::vector<uint32_t>& harts) : vm{vm}, harts{harts} {}
    ~GUIHart() = default;

    inline uint32_t GetSelectedHart() const {
        return harts[selected_hart];
    }

    void Draw();
};

#endif