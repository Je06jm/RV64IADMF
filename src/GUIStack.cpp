#include <GUIStack.hpp>

#include <GUIConstants.hpp>

#include <imgui.h>

#include <stdexcept>
#include <format>

void GUIStack::Draw() {
    if (ImGui::Begin("Stack")) {
        uint32_t sp = vm.GetSP();

        int64_t window_begin = sp >> 2;
        int64_t window_end = window_begin + WINDOW;
        int64_t window_end_pc = window_end << 2;

        if (static_cast<uint32_t>(window_end_pc) >= memory.GetTotalMemory()) {
            window_end = memory.GetUsedMemory() >> 2;
            window_end_pc = window_end << 2;
            window_begin = window_end - WINDOW;
        }

        if (window_begin < 0) {
            throw std::runtime_error(std::format("Memory needs to be at least {} bytes in size", WINDOW << 2));
        }

        uint32_t window_pc = static_cast<uint32_t>(window_begin) << 2;

        auto values = memory.PeekWords(window_pc, WINDOW);

        for (uint32_t addr = window_pc, i = 0; i < WINDOW; addr += 4, i++) {
            if (values[i].second) {
                if (addr == sp) {
                    ImGui::TextColored(gui_sp_highlight_color, "-> 0x%08x : 0x%08x (%i)", addr, values[i].first, values[i].first);
                } else {
                    ImGui::Text("   0x%08x : 0x%08x (%i)", addr, values[i].first, values[i].first);
                }
            }
            else {
                if (addr == sp)
                    ImGui::TextColored(gui_sp_highlight_color, "-> Unmapped memory");
                
                else
                    ImGui::Text("   Unmapped memory");
            }
        }
    }

    ImGui::End();
}