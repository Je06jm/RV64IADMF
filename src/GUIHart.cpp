#include <GUIHart.hpp>

#include <imgui.h>

#include <format>

void GUIHart::Draw() {
    using VM = VirtualMachine;

    auto vm = vms[selected_hart];

    if (ImGui::Begin("Hart")) {
        ImGui::BeginChild("Current Hart Child", ImVec2(150, 20));

        if (ImGui::BeginCombo("Hart", std::format("{}", harts[selected_hart]).c_str(), 0)) {
            for (size_t i = 0; i < harts.size(); i++) {
                bool is_selected = selected_hart == i;
                if (ImGui::Selectable(std::format("{}", harts[i]).c_str(), is_selected, 0)) {
                    selected_hart = i;
                    is_selected = true;
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::EndChild();

        switch (vm->privilege_level) {
            case VirtualMachine::PrivilegeLevel::Machine:
                ImGui::Text("Privilege Level: MACHINE");
                break;
            
            case VirtualMachine::PrivilegeLevel::Supervisor:
                ImGui::Text("Privilege Level: SUPERVISOR");
                break;
            
            case VirtualMachine::PrivilegeLevel::User:
                ImGui::Text("Privilege Level: USER");
                break;
            
            default:
                ImGui::Text("Privilege Level: INVALID");
                break;
        }

        if (!vm->IsRunning())
            ImGui::Text("Status: Stopped");

        else if (vm->IsPaused())
            ImGui::Text("Status: Paused");
        
        else if (vm->waiting_for_interrupt)
            ImGui::Text("Status: Waiting for interrupt");
        
        else
            ImGui::Text("Status: Running");
        
        if (vm->IsUsingVirtualMemory())
            ImGui::Text("Memory Addressing: Virtual");
        
        else
            ImGui::Text("Memory Addressing: Physical");
        
        ImGui::Text("");

        {
            auto pending = vm->GetPendingMachineInterrupts();
            ImGui::Text("Pending Machine Interrupts:    %s", std::format("{:0>32b}", pending).c_str());
        }
        {
            auto enabled = vm->GetEnabledMachineInterrupts();
            ImGui::Text("Enabled Machine Interrupts:    %s", std::format("{:0>32b}", enabled).c_str());
        }
        {
            auto delegated = vm->GetDelegatedMachineInterrupts();
            ImGui::Text("Delegated Machine Interrupts:  %s", std::format("{:0>32b}", delegated).c_str());
        }

        ImGui::Text("");

        {
            auto pending = vm->GetPendingSupervisorInterrupts();
            ImGui::Text("Pending Supervisor Interrupts: %s", std::format("{:0>32b}", pending).c_str());
        }
        {
            auto enabled = vm->GetEnabledSupervisorInterrupts();
            ImGui::Text("Enabled Supervisor Interrupts: %s", std::format("{:0>32b}", enabled).c_str());
        }
    }

    ImGui::End();
}