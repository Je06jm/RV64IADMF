#ifndef GUI_HART_HPP
#define GUI_HART_HPP

#include "VirtualMachine.hpp"

class GUIHart {
    Hart selected_hart = 0;
    const std::vector<Hart> harts;
    
public:
    std::vector<std::shared_ptr<VirtualMachine>> vms;

    GUIHart(std::vector<std::shared_ptr<VirtualMachine>> vms, const std::vector<Hart>& harts) : vms{vms}, harts{harts} {}
    ~GUIHart() = default;

    inline Hart GetSelectedHart() const {
        return harts[selected_hart];
    }

    void Draw();
};

#endif