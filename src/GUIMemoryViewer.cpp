#include <GUIMemoryViewer.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <cstdio>
#include <cmath>
#include <string>

constexpr size_t COLUMNS = 16;
constexpr float CELL_PADDING = 1.25;
constexpr float PADDING = 1.5;

void GUIMemoryViewer::CreateStyle() {
    if (style != nullptr) return;

    style = std::make_unique<Style>();
    style->line_height = ImGui::GetTextLineHeight();
    style->glyph_width = ImGui::CalcTextSize("f").x;

    constexpr size_t ADDRESS_DIGITS = 8;
    constexpr size_t ROWS = 16;

    style->scroll_height = ROWS * style->line_height;

    style->hex_start = ADDRESS_DIGITS * style->glyph_width;
    style->hex_start += style->glyph_width * PADDING;

    style->ascii_start = style->hex_start;
    style->ascii_start += style->glyph_width * COLUMNS * 2 * CELL_PADDING;
    style->ascii_start += style->glyph_width * (CELL_PADDING - 1);
    style->ascii_start += style->glyph_width * PADDING;

    style->window_width = style->ascii_start;
    style->window_width += style->glyph_width * (COLUMNS + 1);
    style->window_width += style->glyph_width * PADDING;

    style->window_height = (ROWS + 1) * style->line_height;
    style->window_height += style->line_height * PADDING * 2;

    ImGuiStyle& s = ImGui::GetStyle();
    style->window_width += s.ScrollbarSize + style->glyph_width * PADDING;
}

void GUIMemoryViewer::UpdateBuffer() {
    uint64_t end_address = static_cast<uint64_t>(read_address) + Memory::PAGE_SIZE;
    end_address = end_address < memory.GetMaxAddress() ? end_address : memory.GetMaxAddress();
    
    if (read_address >= memory.GetMaxAddress()) {
        data_buffer.clear();
        return;
    }
    
    data_buffer = memory.PeekWords(read_address, (end_address - read_address) / sizeof(uint32_t));
}

GUIMemoryViewer::GUIMemoryViewer(Memory& memory, VirtualMachine& vm, uint32_t read_address) : memory{memory}, vm{vm}, read_address{read_address} {
    UpdateBuffer();
}

void GUIMemoryViewer::Draw() {
    if (ImGui::Begin("Memory Viewer")) {
        bool update_window = style == nullptr;
        CreateStyle();

        if (update_window) {
            ImGui::SetWindowSize({ style->window_width, style->window_height });
        }

        ImGui::BeginChild("##scrolling", ImVec2(0, style->scroll_height), 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        ImGuiListClipper clipper;

        size_t lines = static_cast<size_t>(ceilf(static_cast<float>(data_buffer.size()) / sizeof(uint32_t)));
        clipper.Begin(lines, style->line_height);

        ImVec2 window_pos = ImGui::GetWindowPos();
        {
            float hex_start = window_pos.x + style->hex_start + style->glyph_width;

            draw_list->AddLine(
                ImVec2(hex_start, window_pos.y),
                ImVec2(hex_start, window_pos.y + 9999),
                ImGui::GetColorU32(ImGuiCol_Border)
            );

            float ascii_start = window_pos.x + style->ascii_start - style->glyph_width / 2;

            draw_list->AddLine(
                ImVec2(ascii_start, window_pos.y),
                ImVec2(ascii_start, window_pos.y + 9999),
                ImGui::GetColorU32(ImGuiCol_Border)
            );
        }

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                uint32_t index = i * COLUMNS;
                uint32_t addr = index + read_address;

                ImGui::Text("0x%08x", addr);

                for (size_t c = 0; c < COLUMNS; c++) {
                    float cell_pos_x = c * style->glyph_width * 2;
                    cell_pos_x *= CELL_PADDING;
                    cell_pos_x += style->glyph_width * PADDING;
                    cell_pos_x += style->hex_start;
                    ImGui::SameLine(cell_pos_x);

                    float ascii_pos_x = c * style->glyph_width + style->ascii_start;

                    uint32_t byte_index = index + c;
                    if ((byte_index >> 2) >= data_buffer.size()) {
                        ImGui::Text("..");
                        ImGui::SameLine(ascii_pos_x);
                        ImGui::Text(" ");
                    } else {
                        if (data_buffer[byte_index >> 2].second) {
                            uint32_t value = data_buffer[byte_index >> 2].first;
                            uint8_t byte = value >> (8 * (byte_index & 0b11));

                            if (addr + c == vm.GetPC()) {
                                ImGui::TextColored(gui_pc_highlight_color, "%02x", byte);
                            } else if (addr + c == vm.GetSP()) {
                                ImGui::TextColored(gui_sp_highlight_color, "%02x", byte);
                            } else {
                                ImGui::Text("%02x", byte);
                            }

                            ImGui::SameLine(ascii_pos_x);
                            if (byte >= 32 && byte < 127) {
                                ImGui::Text("%c", byte);
                            } else {
                                ImGui::Text(".");
                            }
                        }
                        else {
                            if (addr + c == vm.GetPC())
                                ImGui::TextColored(gui_pc_highlight_color, "xx");
                            else if (addr + c == vm.GetSP())
                                ImGui::TextColored(gui_sp_highlight_color, "xx");
                            else
                                ImGui::Text("xx");
                        }
                    }
                }
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        auto flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
        ImGui::Text("Address: 0x");
        ImGui::SameLine(ImGui::CalcTextSize("Address: 0x ").x);
        
        ImGui::PushItemWidth(12 * style->glyph_width);
        bool enter = ImGui::InputText("##Address", &text_input_buffer, flags);
        ImGui::PopItemWidth();

        ImGui::SameLine(ImGui::CalcTextSize("Address: 0x 0000000000000").x);
        bool view_button = ImGui::Button("View");
        if (text_input_buffer.empty()) {
            char buffer[16];
            sprintf(buffer, "%x", read_address);
            text_input_buffer = buffer;
        } else if (view_button || enter) {
            uint32_t aligned_address = read_address;
            try {
                aligned_address = std::stoul(text_input_buffer, nullptr, 16);
            } catch (...) {

            }
            aligned_address = aligned_address < memory.GetMaxAddress() ? aligned_address : (memory.GetMaxAddress() - COLUMNS);
            aligned_address &= ~0xf;

            read_address = aligned_address;
            UpdateBuffer();
            
            char buffer[16];
            sprintf(buffer, "%x", aligned_address);
            text_input_buffer = buffer;
        }
    }

    ImGui::End();
}