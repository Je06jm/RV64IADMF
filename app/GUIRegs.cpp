#include "GUIRegs.hpp"

#include "GUIConstants.hpp"

#include <imgui.h>

void GUIRegs::Draw() {
    if (ImGui::Begin("Registers")) {
        std::array<Word, VirtualMachine::REGISTER_COUNT> regs;
        std::array<Float, VirtualMachine::REGISTER_COUNT> fregs;
        Word pc;

        vm->GetSnapshot(regs, fregs, pc);

        ImGui::TextColored(gui_pc_highlight_color, "          pc  : 0x%08x", pc);
        ImGui::TextColored(gui_sp_highlight_color, "          sp  : 0x%08x", regs[2]);
        ImGui::Text(" ");

        std::array<std::string, VirtualMachine::REGISTER_COUNT> names = {
            "zero",
            "ra",
            "sp",
            "gp",
            "tp",
            "t0",
            "t1",
            "t2",
            "s0 / fp",
            "s1",
            "a0",
            "a1",
            "a2",
            "a3",
            "a4",
            "a5",
            "a6",
            "a7",
            "s2",
            "s3",
            "s4",
            "s5",
            "s6",
            "s7",
            "s8",
            "s9",
            "s10",
            "s11",
            "t3",
            "t4",
            "t5",
            "t6"
        };

        std::array<std::string, VirtualMachine::REGISTER_COUNT> fnames = {
            "ft0",
            "ft1",
            "ft2",
            "ft3",
            "ft4",
            "ft5",
            "ft6",
            "ft7",
            "fs0",
            "fs1",
            "fa0",
            "fa1",
            "fa2",
            "fa3",
            "fa4",
            "fa5",
            "fa6",
            "fa7",
            "fs2",
            "fs3",
            "fs4",
            "fs5",
            "fs6",
            "fs7",
            "fs8",
            "fs9",
            "fs10",
            "fs11",
            "ft8",
            "ft9",
            "ft10",
            "ft11"
        };

        for (size_t i = 0; i < VirtualMachine::REGISTER_COUNT; i++) {
            ImGui::Text("%-10sx%-2u : 0x%08x (%i)", names[i].c_str(), static_cast<Word>(i), regs[i], regs[i]);
        }

        ImGui::Text(" ");

        for (size_t i = 0; i < VirtualMachine::REGISTER_COUNT; i++) {
            if (fregs[i].is_double)
                ImGui::Text("%-10sf%-2u : 0x%016llx (%.8g)", fnames[i].c_str(), static_cast<Word>(i), *reinterpret_cast<Address*>(&fregs[i].d), fregs[i].d);

            else
                ImGui::Text("%-10sf%-2u : 0x%08x (%.8g)", fnames[i].c_str(), static_cast<Word>(i), *reinterpret_cast<Word*>(&fregs[i].f), fregs[i].f);
        }
    }

    ImGui::End();
}