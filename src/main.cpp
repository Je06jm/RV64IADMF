#include "Window.hpp"
#include "OpenGL.hpp"

#include "Memory.hpp"
#include "VirtualMachine.hpp"

#include "GUIMemoryViewer.hpp"
#include "GUIAssembly.hpp"
#include "GUIInfo.hpp"
#include "GUIRegs.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, const char** argv) {
    std::vector<std::string> args;

    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    if (args.size() != 1) {
        std::cout << "Usage: RV32IMF.exe <bios-file.bin>" << std::endl;
        return 0;
    }

    Window window("RV32IMF", 800, 600);

    Memory memory(2 * 1024 * 1024);
    memory.ReadFileInto(args[0], 0);
    VirtualMachine vm(memory, 0, 1000000, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    io.Fonts->AddFontDefault();

    window.SetupImGui();
    ImGui_ImplOpenGL3_Init();

    GUIMemoryViewer mem_viewer(memory, vm);
    GUIAssembly assembly(vm, memory);
    GUIInfo info(memory, vm);
    GUIRegs state(vm);

    while (!window.ShouldClose()) {
        window.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Do rendering

        mem_viewer.Draw();
        assembly.Draw();
        info.Draw();
        state.Draw();

        if (vm.IsRunning()) vm.Step(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            auto ctx = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(ctx);
        }
    }
}