#include <GUIInfo.hpp>

#include <imgui.h>

#include <format>

void GUIInfo::Draw() {
    if (ImGui::Begin("Info")) {
        auto vm_kbs = memory.GetTotalMemory() / 1024.0f;
        auto vm_mbs = vm_kbs / 1024.0f;
        auto vm_gbs = vm_mbs / 1024.0f;

        if (vm_mbs < 1.0) {
            ImGui::Text("VM memory size: %.2f KiBs", vm_kbs);
        } else if (vm_gbs < 1.0) {
            ImGui::Text("VM memory size: %.2f MiBs", vm_mbs);
        } else {
            ImGui::Text("VM memory size: %.2f GiBs", vm_gbs);
        }

        auto hm_kbs = memory.GetUsedMemory() / 1024.0f;
        auto hm_mbs = hm_kbs / 1024.0f;
        auto hm_gbs = hm_mbs / 1024.0f;

        if (hm_mbs < 1.0) {
            ImGui::Text("Host memory size: %.2f KiBs", hm_kbs);
        } else if (hm_gbs < 1.0) {
            ImGui::Text("Host memory size: %.2f MiBs", hm_mbs);
        } else {
            ImGui::Text("Host memory size: %.2f GiBs", hm_gbs);
        }
    }

    ImGui::End();
}