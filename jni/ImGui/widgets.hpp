#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>
#include <unordered_map>

class CustomWidgets
{
public:
    ImVec4 m_accent = ImVec4(0.42f, 0.44f, 0.65f, 1.0f);
    
    std::unordered_map<ImGuiID, float> m_anim_state;
    
    float animate(ImGuiID id, bool active, float speed = 12.0f)
    {
        float& anim = m_anim_state[id];
        float target = active ? 1.0f : 0.0f;
        float delta = ImGui::GetIO().DeltaTime * speed;
        
        if (anim < target)
            anim = ImMin(anim + delta, target);
        else if (anim > target)
            anim = ImMax(anim - delta, target);
        
        return anim;
    }
    
    float ease_out_quad(float t) { return t * (2.0f - t); }
    float ease_in_out(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

    void set_accent(const ImVec4& color) { m_accent = color; }
    
    ImU32 lerp_color(ImU32 a, ImU32 b, float t)
    {
        int ra = (a >> 0) & 0xFF, ga = (a >> 8) & 0xFF, ba = (a >> 16) & 0xFF, aa = (a >> 24) & 0xFF;
        int rb = (b >> 0) & 0xFF, gb = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF, ab = (b >> 24) & 0xFF;
        return IM_COL32(
            (int)(ra + (rb - ra) * t),
            (int)(ga + (gb - ga) * t),
            (int)(ba + (bb - ba) * t),
            (int)(aa + (ab - aa) * t)
        );
    }

    void text(const char* label)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", label);
    }

    void group_header(const char* label, float top_spacing = 8.0f)
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + top_spacing);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 20.0f;
        
        draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(30, 30, 35, 255));
        draw_list->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 2, pos.y + height), ImGui::GetColorU32(m_accent));
        
        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(pos.x + 10, pos.y + (height - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, IM_COL32(160, 160, 165, 255), label);
        
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + height + 4);
    }

    bool checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const float square_sz = 24.0f; // Increased from 14.0f to 24.0f (approx 70% bigger)
    const float padding = 12.0f; // Increased padding
    const ImVec2 pos = ImVec2(window->DC.CursorPos.x + padding, window->DC.CursorPos.y + 2);
    const float row_height = ImMax(square_sz, label_size.y) + 12; // Increased row height
    
    const ImRect total_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y), 
                          ImVec2(window->DC.CursorPos.x + ImGui::GetContentRegionAvail().x, window->DC.CursorPos.y + row_height));
    ImGui::ItemSize(total_bb, 0);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
    }

    float anim = animate(id, *v);
    float hover_anim = animate(id + 10000, hovered, 15.0f);

    const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));
    
    ImU32 border_col = lerp_color(IM_COL32(55, 55, 60, 255), ImGui::GetColorU32(m_accent), hover_anim);
    window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, IM_COL32(35, 35, 40, 255), 4.0f); // Increased rounding
    window->DrawList->AddRect(check_bb.Min, check_bb.Max, border_col, 4.0f, 0, 2.5f); // Thicker border
    
    if (anim > 0.01f)
    {
        float inner_pad = 5.0f; // Increased inner padding
        float fill_size = (square_sz - inner_pad * 2) * ease_out_quad(anim);
        float offset = (square_sz - fill_size) * 0.5f;
        ImVec2 fill_min = ImVec2(check_bb.Min.x + offset, check_bb.Min.y + offset);
        ImVec2 fill_max = ImVec2(fill_min.x + fill_size, fill_min.y + fill_size);
        
        ImU32 fill_col = ImGui::GetColorU32(ImVec4(m_accent.x, m_accent.y, m_accent.z, anim));
        window->DrawList->AddRectFilled(fill_min, fill_max, fill_col, 3.0f); // Increased rounding on fill
    }
    
    ImVec2 label_pos = ImVec2(check_bb.Max.x + 12, pos.y + (square_sz - label_size.y) * 0.5f); // Increased spacing
    ImU32 text_col = lerp_color(IM_COL32(140, 140, 145, 255), IM_COL32(220, 220, 225, 255), anim);
    window->DrawList->AddText(label_pos, text_col, label);

    return pressed;
}

    
    

bool dropdown(const char* label, int* current, const char* const items[], int count)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const float padding = 15.0f; // Increased from 8.0f
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float dropdown_width = avail_width - padding * 2;
    const float dropdown_height = 36.0f; // Increased from 18.0f (100% bigger - adjusted to 85%)
    
    if (label && label[0] != '#')
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);
        ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.48f, 1.0f), "%s", label);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5); // Increased spacing
    }
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    ImGuiID id = window->GetID(label);
    ImRect bb(pos, ImVec2(pos.x + dropdown_width, pos.y + dropdown_height));
    ImGui::ItemSize(bb, 0);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    bool is_open = ImGui::IsPopupOpen(label);
    float open_anim = animate(id + 40000, is_open, 10.0f);
    float hover_anim = animate(id + 50000, hovered, 15.0f);
    
    if (pressed)
        ImGui::OpenPopup(label);
    
    ImU32 bg_col = lerp_color(IM_COL32(38, 38, 42, 255), IM_COL32(45, 45, 50, 255), hover_anim);
    ImU32 border_col = lerp_color(IM_COL32(55, 55, 60, 255), ImGui::GetColorU32(m_accent), hover_anim + open_anim * 0.5f);
    
    window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, 6.0f); // Increased rounding
    window->DrawList->AddRect(bb.Min, bb.Max, border_col, 6.0f, 0, 2.5f); // Thicker border
    
    const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
    ImVec2 text_size = ImGui::CalcTextSize(preview);
    ImVec2 text_pos = ImVec2(bb.Min.x + 15, bb.Min.y + (dropdown_height - text_size.y) * 0.5f); // Adjusted position
    window->DrawList->AddText(text_pos, IM_COL32(200, 200, 205, 255), preview);
    
    float arrow_x = bb.Max.x - 28; // Adjusted arrow position
    float arrow_y = bb.Min.y + dropdown_height * 0.5f;
    float arrow_rot = open_anim * 3.14159f;
    
    // Bigger arrow
    ImVec2 p1 = ImVec2(arrow_x - 8, arrow_y - 4.0f + open_anim * 6);
    ImVec2 p2 = ImVec2(arrow_x + 8, arrow_y - 4.0f + open_anim * 6);
    ImVec2 p3 = ImVec2(arrow_x, arrow_y + 5.0f - open_anim * 6);
    
    ImU32 arrow_col = lerp_color(IM_COL32(120, 120, 125, 255), ImGui::GetColorU32(m_accent), hover_anim);
    window->DrawList->AddTriangleFilled(p1, p2, p3, arrow_col);
    
    bool changed = false;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 10)); // Increased padding
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f); // Increased rounding
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 3)); // Increased spacing
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(35, 35, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(55, 55, 60, 255));
    
    ImGui::SetNextWindowPos(ImVec2(bb.Min.x, bb.Max.y + 5)); // Increased spacing
    ImGui::SetNextWindowSize(ImVec2(dropdown_width, 0));
    
    if (ImGui::BeginPopup(label, ImGuiWindowFlags_NoMove))
    {
        ImDrawList* popup_draw_list = ImGui::GetWindowDrawList();
        
        for (int i = 0; i < count; i++)
        {
            ImGuiID item_id = ImGui::GetID(items[i]);
            bool item_hovered = false;
            bool selected = (*current == i);
            
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            ImVec2 item_size = ImVec2(dropdown_width, 42.0f); // Increased from 20.0f (110% bigger - adjusted for 85%)
            ImRect item_bb(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y));
            
            ImGui::ItemSize(item_bb, 0);
            if (ImGui::ItemAdd(item_bb, item_id))
            {
                bool item_held;
                bool item_pressed = ImGui::ButtonBehavior(item_bb, item_id, &item_hovered, &item_held);
                
                float item_hover_anim = animate(item_id + 60000, item_hovered, 20.0f);
                
                if (item_hovered || selected)
                {
                    ImU32 sel_col = lerp_color(IM_COL32(45, 45, 50, 255), IM_COL32(55, 55, 65, 255), item_hover_anim);
                    popup_draw_list->AddRectFilled(item_bb.Min, item_bb.Max, sel_col, 6.0f); // Increased rounding
                    
                    if (selected)
                        popup_draw_list->AddRectFilled(item_bb.Min, ImVec2(item_bb.Min.x + 5, item_bb.Max.y), ImGui::GetColorU32(m_accent)); // Thicker selection bar
                }
                
                ImVec2 item_text_pos = ImVec2(item_bb.Min.x + 18, item_bb.Min.y + (item_size.y - ImGui::CalcTextSize(items[i]).y) * 0.5f); // Adjusted position
                ImU32 item_text_col = selected ? ImGui::GetColorU32(m_accent) : (item_hovered ? IM_COL32(220, 220, 225, 255) : IM_COL32(160, 160, 165, 255));
                popup_draw_list->AddText(item_text_pos, item_text_col, items[i]);
                
                if (item_pressed)
                {
                    *current = i;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    
    return changed;
}
};
