#ifndef MEMORY_EDITOR_HPP
#define MEMORY_EDITOR_HPP

#include "Memory.hpp"
#include "VirtualMachine.hpp"

#include "GUIConstants.hpp"

#include <string>
#include <utility>

class GUIMemoryViewer {
    Memory& memory;

    struct Style {
        float glyph_width;
        float line_height;
        
        float scroll_height;

        float hex_start;
        float ascii_start;

        float window_width;
        float window_height;
    };

    std::unique_ptr<Style> style = nullptr;

    void CreateStyle();
    void UpdateBuffer();

    std::vector<std::pair<uint32_t, bool>> data_buffer;
    uint32_t read_address = 0;

    std::string text_input_buffer = "0";

public:
    std::shared_ptr<VirtualMachine> vm;
    
    GUIMemoryViewer(Memory& memory, std::shared_ptr<VirtualMachine> vm, uint32_t read_address = 0);
    ~GUIMemoryViewer() = default;

    void Draw();

    inline void Update() {
        UpdateBuffer();
    }
};

#endif