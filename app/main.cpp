#include <Window.hpp>
#include <OpenGL.hpp>
#include <DeltaTime.hpp>

#include <Memory.hpp>
#include <RV32I.hpp>

#include <GUIMemoryViewer.hpp>
#include <GUIAssembly.hpp>
#include <GUIInfo.hpp>
#include <GUIRegs.hpp>
#include <GUIHart.hpp>
#include <GUIStack.hpp>
#include <GUICSR.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "GDB.hpp"
#include "ECalls.hpp"
#include "ArgsParser.hpp"
#include "Framebuffer.hpp"
#include "VirtualMachines.hpp"

int main(int argc, const char** argv) {
    constexpr Word window_width = 800;
    constexpr Word window_height = 600;
    
    framebuffer_width = 800;
    framebuffer_height = 600;
    framebuffer_address = 0xffe00000;

    Hart cores = 2;

    std::vector<std::string> args;

    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    // Process command line args
    ArgsParser args_parser(args);

    cores = args_parser.GetValueOr<Hart>("cores", 1);
    
    if (!args_parser.HasValue("bios_file")) {
        std::cerr << "--bios_file is required" << std::endl;
        return -1;
    }

    auto bios_path = args_parser.GetValue<std::string>("bios_file");

    if (cores == 0) cores = 1;

    RVInstruction::SetupCSRNames();
    
    RegisterECalls();

    Window window("RV32IMF", window_width, window_height);
    {
        constexpr Address BIOS_RAM_ADDRESS = 0x1000;

        Memory memory;
        {
            auto ram = MemoryRAM::Create(BIOS_RAM_ADDRESS, 16 * 1024 * 1024);
            memory.AddMemoryRegion(std::move(ram));
        }
        memory.ReadFileInto(bios_path, BIOS_RAM_ADDRESS);

        auto framebuffer = MemoryFramebuffer::Create(framebuffer_address, framebuffer_width, framebuffer_height);
        memory.AddMemoryRegion(framebuffer);

        std::vector<Hart> harts;
        for (Hart i = 0; i < cores; i++) {
            vms.push_back(std::make_shared<VirtualMachine>(memory, BIOS_RAM_ADDRESS, i));
            harts.push_back(i);
        }

        //GDBServer gdb_server(memory);
        //gdb_server.Run(8000);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        io.Fonts->AddFontDefault();

        window.SetupImGui();
        ImGui_ImplOpenGL3_Init();

        GUIMemoryViewer mem_viewer(memory, vms[0], 0x0);
        GUIAssembly assembly(vms[0], memory);
        GUIInfo info(memory, vms[0]);
        GUIHart gui_harts(vms, harts);
        GUIRegs state(vms[0]);
        GUIStack stack(vms[0], memory);
        GUICSR csr(vms[0]);

        for (auto vm : vms) {
            vm->Pause();

            if (args_parser.HasFlag("pause_on_break"))
                vm->SetPauseOnBreak(true);
            
            if (args_parser.HasFlag("pause_on_restart"))
                vm->SetPauseOnRestart(true);

            vm->Start();
        }

        if (!args_parser.HasFlag("p"))
            vms[0]->Unpause();

        delta_time.Update();

        std::vector<std::jthread> workers;
        for (size_t i = 0; i < vms.size(); i++) {
            workers.emplace_back(std::jthread([](size_t i) {
                vms[i]->Run();
            }, i));
        }

        while (!window.ShouldClose()) {
            window.Update();
            delta_time.Update();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            for (auto& vm : vms)
                vm->UpdateTime();

            auto vm = vms[gui_harts.GetSelectedHart()];
            mem_viewer.vm = vm;
            assembly.vm = vm;
            info.vm = vm;
            state.vm = vm;
            stack.vm = vm;
            csr.vm = vm;

            if (ImGui::IsKeyPressed(ImGuiKey_Space, true) && vm->IsPaused())
                vm->Step(1);
            
            if (ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
                if (vm->IsPaused())
                    vm->Unpause();
                
                else
                    vm->Pause();
            }

            // Do rendering

            mem_viewer.Draw();
            assembly.Draw();
            info.Draw();
            gui_harts.Draw();
            state.Draw();
            stack.Draw();
            csr.Draw();
            
            assembly.Draw();
            
            framebuffer->DrawBuffer();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                auto ctx = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(ctx);
            }
        }

        for (auto& vm : vms)
            vm->Stop();
    }
}