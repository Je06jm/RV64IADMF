#include "GUIRegs.hpp"

#include "GUIConstants.hpp"

#include <imgui.h>

#include <format>

void GUIRegs::Draw() {
    using VM = VirtualMachine;
    if (ImGui::Begin("Registers")) {
        std::array<VM::Reg, VM::REGISTER_COUNT> regs;
        std::array<Float, VM::REGISTER_COUNT> fregs;
        Long pc;

        vm->GetSnapshot(regs, fregs, pc);

        if (vm->Is32BitMode())
            ImGui::TextColored(gui_pc_highlight_color, "%s", std::format("          pc  : 0x{:0>8x}", pc).c_str());
        
        else
            ImGui::TextColored(gui_pc_highlight_color, "%s", std::format("          pc  : 0x{:0>16x}", pc).c_str());
        
        if (vm->Is32BitMode())
            ImGui::TextColored(gui_pc_highlight_color, "%s", std::format("          sp  : 0x{:0>8x}", regs[VM::REG_SP].u32).c_str());
        
        else
            ImGui::TextColored(gui_pc_highlight_color, "%s", std::format("          sp  : 0x{:0>16x}", regs[VM::REG_SP].u64).c_str());

        ImGui::NewLine();

        std::array<std::string, VM::REGISTER_COUNT> names = {
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

        std::array<std::string, VM::REGISTER_COUNT> fnames = {
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

        union SU {
            struct {
                uint32_t u32;
                uint32_t _unused0;
            };
            struct {
                int32_t s32;
                int32_t _unused1;
            };
            uint64_t u64;
            int64_t s64;
        };

        for (size_t i = 0; i < VM::REGISTER_COUNT; i++) {
            std::string fmt;
            SU su;
            su.u64 = regs[i].u64;
            if (vm->Is32BitMode())
                fmt = std::format("{:<10}x{:<2} : 0x{:0>8x} ({})", names[i].c_str(), i, su.u32, su.s32);
            
            else
                fmt = std::format("{:<10}x{:<2} : 0x{:0>16x} ({})", names[i].c_str(), i, su.u64, su.s64);
            
            ImGui::Text("%s", fmt.c_str());
        }

        ImGui::Text(" ");

        for (size_t i = 0; i < VM::REGISTER_COUNT; i++) {
            if (fregs[i].is_double)
                ImGui::Text("%-10sf%-2u : 0x%016llx (%.8g)", fnames[i].c_str(), static_cast<Word>(i), *reinterpret_cast<Address*>(&fregs[i].d), fregs[i].d);

            else
                ImGui::Text("%-10sf%-2u : 0x%08x (%.8g)", fnames[i].c_str(), static_cast<Word>(i), *reinterpret_cast<Word*>(&fregs[i].f), fregs[i].f);
        }
    }

    ImGui::End();
}