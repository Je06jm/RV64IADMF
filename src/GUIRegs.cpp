#include "GUIRegs.hpp"

#include <imgui.h>

void GUIRegs::Draw() {
    if (ImGui::Begin("Registers")) {
        std::array<uint32_t, VirtualMachine::REGISTER_COUNT> regs;
        std::array<float, VirtualMachine::REGISTER_COUNT> fregs;
        uint32_t pc;

        vm.GetSnapshot(regs, fregs, pc);

        ImGui::Text("          pc  : 0x%08x", pc);
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

        for (size_t i = 0; i < VirtualMachine::REGISTER_COUNT; i++) {
            ImGui::Text("%-10sr%-2u : 0x%08x (%i)", names[i].c_str(), static_cast<uint32_t>(i), regs[i], regs[i]);
        }

        ImGui::Text(" ");

        for (size_t i = 0; i < VirtualMachine::REGISTER_COUNT; i++) {

            ImGui::Text("          f%-2u : 0x%08x (%.8g)", static_cast<uint32_t>(i), *reinterpret_cast<uint32_t*>(&fregs[i]), fregs[i]);
        }
    }

    ImGui::End();
}