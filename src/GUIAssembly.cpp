#include "GUIAssembly.hpp"

#include "GUIConstants.hpp"

#include <imgui.h>

#include <stdexcept>
#include <format>

void GUIAssembly::Draw() {
    if (ImGui::Begin("Assembly")) {
        uint32_t pc = vm.GetPC();

        int64_t window_begin = (pc >> 2) - WINDOW / 2 + WINDOW_SLIDE;

        if (window_begin < 0) window_begin = 0;

        int64_t window_end = window_begin + WINDOW;
        int64_t window_end_pc = window_end << 2;

        if (static_cast<uint32_t>(window_end_pc) >= memory.GetTotalMemory()) {
            window_end = memory.GetTotalMemory() >> 2;
            window_end_pc = window_end << 2;
            window_begin = window_end - WINDOW;
        }

        if (window_begin < 0) {
            throw std::runtime_error(std::format("Memory needs to be at least {} bytes in size", WINDOW << 2));
        }

        uint32_t window_pc = static_cast<uint32_t>(window_begin) << 2;

        auto instrs = memory.Read(window_pc, WINDOW);

        for (uint32_t addr = window_pc, i = 0; i < WINDOW; addr += 4, i++) {
            RVInstruction instr = RVInstruction::FromUInt32(instrs[i]);
            if (addr == pc) {
                ImGui::TextColored(gui_pc_highlight_color, "-> 0x%08x %s", addr, std::string(instr).c_str());
            } else if (vm.IsBreakPoint(addr)) {
                ImGui::TextColored(gui_break_highlight_color, "   0x%08x %s", addr, std::string(instr).c_str());
            } else {
                ImGui::Text("   0x%08x %s", addr, std::string(instr).c_str());
            }
        }
    }

    ImGui::End();
}