#include <Window.hpp>
#include <OpenGL.hpp>
#include <DeltaTime.hpp>

#include <Memory.hpp>
#include <RV32I.hpp>

#include <GUIMemoryViewer.hpp>
#include <GUIAssembly.hpp>
#include <GUIInfo.hpp>
#include <GUIRegs.hpp>
#include <GUIStack.hpp>
#include <GUICSR.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "ECalls.hpp"
#include "Framebuffer.hpp"
#include "VirtualMachines.hpp"

int main(int argc, const char** argv) {
    constexpr size_t window_width = 800;
    constexpr size_t window_height = 600;
    
    framebuffer_width = 800;
    framebuffer_height = 600;

    std::vector<std::string> args;

    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    if (args.size() != 1) {
        std::cout << "Usage: RV32IMF.exe <bios-file.bin>" << std::endl;
        return 0;
    }

    RVInstruction::SetupCSRNames();
    
    RegisterECalls();

    Window window("RV32IMF", window_width, window_height);
    {
        Memory memory;
        {
            auto rom = MemoryROM::Create({0x12345678}, 0);
            memory.AddMemoryRegion(std::move(rom));
        }
        {
            auto ram = MemoryRAM::Create(0x1000, 2 * 1024 * 1024);
            memory.AddMemoryRegion(std::move(ram));
        }
        memory.ReadFileInto(args[0], 0x1000);

        auto framebuffer = MemoryFramebuffer::Create(0x10000000, framebuffer_width, framebuffer_height);
        memory.AddMemoryRegion(framebuffer);

        vms.emplace_back(memory, 0x1000, 0);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        io.Fonts->AddFontDefault();

        window.SetupImGui();
        ImGui_ImplOpenGL3_Init();

        GUIMemoryViewer mem_viewer(memory, vms[0], 0x1000);
        GUIAssembly assembly(vms[0], memory);
        GUIInfo info(memory, vms[0]);
        GUIRegs state(vms[0]);
        GUIStack stack(vms[0], memory);
        GUICSR csr(vms[0]);

        vms[0].Start();
        delta_time.Update();

        auto worker = std::jthread([&]() {
            while (vms[0].IsRunning())
                vms[0].Step();
        });

        bool auto_run = false;

        while (!window.ShouldClose()) {
            window.Update();
            delta_time.Update();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            vms[0].UpdateTime();

            // Do rendering

            mem_viewer.Draw();
            assembly.Draw();
            info.Draw();
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

        vms[0].Stop();
    }
}