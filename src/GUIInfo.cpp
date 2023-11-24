#include "GUIInfo.hpp"

#include <imgui.h>

void GUIInfo::Draw() {
    if (ImGui::Begin("Info")) {
        auto vm_kbs = memory.GetTotalMemory() / 1024.0f;
        auto vm_mbs = vm_kbs / 1024.0f;

        if (vm_mbs < 1.0) {
            ImGui::Text("VM memory size: %.2f KiBs", vm_kbs);
        } else {
            ImGui::Text("VM memory size: %.2f MiBs", vm_mbs);
        }

        auto hm_kbs = memory.GetUsedMemory() / 1024.0f;
        auto hm_mbs = hm_kbs / 1024.0f;

        if (hm_mbs < 1.0) {
            ImGui::Text("Host memory size: %.2f KiBs", hm_kbs);
        } else {
            ImGui::Text("Host memory size: %.2f MiBs", hm_mbs);
        }
        
        auto ips = vm.GetInstructionsPerSecond();
        auto k_ips = ips / 1000.0f;
        auto m_ips = k_ips / 1000.0f;

        if (k_ips < 1.0) {
            ImGui::Text("IPC: %llu", ips);
        } else if (m_ips < 1.0) {
            ImGui::Text("IPC: %.2fK", k_ips);
        } else {
            ImGui::Text("IPC: %.2fM", m_ips);
        }
    }

    ImGui::End();
}