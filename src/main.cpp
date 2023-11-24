#include "Window.hpp"
#include "OpenGL.hpp"

#include "Memory.hpp"
#include "VirtualMachine.hpp"

#include "GUIMemoryViewer.hpp"
#include "GUIInfo.hpp"
#include "GUIRegs.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

int main() {
    Window window("RV32IMF", 800, 600);

    Memory memory(2 * 1024 * 1024);
    VirtualMachine vm(memory, 0, 1000000);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    io.Fonts->AddFontDefault();

    window.SetupImGui();
    ImGui_ImplOpenGL3_Init();

    GUIMemoryViewer mem_viewer(memory);
    GUIInfo info(memory, vm);
    GUIRegs state(vm);

    while (!window.ShouldClose()) {
        window.Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Do rendering

        mem_viewer.Draw();
        info.Draw();
        state.Draw();

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