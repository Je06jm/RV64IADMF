#ifndef GUI_HART_HPP
#define GUI_HART_HPP

#include "VirtualMachine.hpp"

class GUIHart {
    uint32_t selected_hart = 0;
    const std::vector<uint32_t> harts;
    
public:
    std::vector<std::shared_ptr<VirtualMachine>> vms;

    GUIHart(std::vector<std::shared_ptr<VirtualMachine>> vms, const std::vector<uint32_t>& harts) : vms{vms}, harts{harts} {}
    ~GUIHart() = default;

    inline uint32_t GetSelectedHart() const {
        return harts[selected_hart];
    }

    void Draw();
};

#endif