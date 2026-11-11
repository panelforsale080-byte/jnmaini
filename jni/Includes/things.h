#pragma once

float ImLerp(float a, float b, float t) { return a + (b - a) * t; }

float ImClamp(float v, float min, float max) {
    return (v < min) ? min : (v > max) ? max : v;
}

bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.0f") {
    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float width = ImGui::GetContentRegionAvail().x;
    float height = 28.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    char value_buf[32];
    snprintf(value_buf, sizeof(value_buf), format, *v);
    ImVec2 value_size = ImGui::CalcTextSize(value_buf);
    
    draw_list->AddText(ImVec2(pos.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), label);
    draw_list->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), value_buf);
    
    pos.y += ImGui::CalcTextSize(label).y + 4.0f;
    ImGui::SetCursorScreenPos(pos);
    
    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();
    bool changed = false;
    
    if (active && ImGui::IsMouseDragging(0)) {
        float t = (ImGui::GetMousePos().x - pos.x) / width;
        t = ImClamp(t, 0.0f, 1.0f);
        *v = v_min + (v_max - v_min) * t;
        changed = true;
    }
    
    float trackHeight = 6.0f;
    float trackY = pos.y + height / 2.0f - trackHeight / 2.0f;
    float t = (*v - v_min) / (v_max - v_min);
    float grabX = pos.x + width * t;
    
    draw_list->AddRectFilled(
        ImVec2(pos.x, trackY),
        ImVec2(pos.x + width, trackY + trackHeight),
        ImGui::GetColorU32(ImGuiCol_FrameBg),
        3.0f
    );
    
    ImU32 fillColor = hovered || active ? ImGui::GetColorU32(ImGuiCol_SliderGrabActive) : ImGui::GetColorU32(ImGuiCol_SliderGrab);
    draw_list->AddRectFilled(
        ImVec2(pos.x, trackY),
        ImVec2(grabX, trackY + trackHeight),
        fillColor,
        3.0f
    );
    
    float grabRadius = active ? 7.0f : (hovered ? 6.5f : 6.0f);
    draw_list->AddCircleFilled(
        ImVec2(grabX, pos.y + height / 2.0f),
        grabRadius,
        ImGui::GetColorU32(ImGuiCol_SliderGrab)
    );
    draw_list->AddCircle(
        ImVec2(grabX, pos.y + height / 2.0f),
        grabRadius,
        ImGui::GetColorU32(ImGuiCol_Border),
        0,
        1.5f
    );
    
    return changed;
}

#define SLIDER(label, v, v_min, v_max, ...) CustomSliderFloat(label, v, v_min, v_max, ##__VA_ARGS__)

void DrawMacOSHeaderButtons()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winPad = ImGui::GetStyle().WindowPadding;
    
    float radius = 7.0f;
    float spacing = 20.0f;
    
    ImVec2 circlePos = ImVec2(
        winPos.x + winPad.x + radius + 4,
        winPos.y + winPad.y + radius + 4
    );
    
    draw->AddCircleFilled(circlePos, radius, IM_COL32(255, 92, 85, 255));
    draw->AddCircleFilled(ImVec2(circlePos.x + spacing, circlePos.y), radius, IM_COL32(255, 189, 47, 255));
    draw->AddCircleFilled(ImVec2(circlePos.x + spacing * 2, circlePos.y), radius, IM_COL32(40, 201, 64, 255));
    
    draw->AddCircle(circlePos, radius, IM_COL32(0, 0, 0, 40), 0, 1.0f);
    draw->AddCircle(ImVec2(circlePos.x + spacing, circlePos.y), radius, IM_COL32(0, 0, 0, 40), 0, 1.0f);
    draw->AddCircle(ImVec2(circlePos.x + spacing * 2, circlePos.y), radius, IM_COL32(0, 0, 0, 40), 0, 1.0f);
    
    ImGui::SetCursorScreenPos(ImVec2(circlePos.x - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("closeBtn", ImVec2(radius * 2, radius * 2))) {}
    
    ImGui::SetCursorScreenPos(ImVec2(circlePos.x + spacing - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("minBtn", ImVec2(radius * 2, radius * 2))) {}
    
    ImGui::SetCursorScreenPos(ImVec2(circlePos.x + spacing * 2 - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("maxBtn", ImVec2(radius * 2, radius * 2))) {}
}
std::map<std::string, ImVec4> colorStates = {
    {"ENEMY",   ImVec4(1.0f, 0.0f, 0.0f, 1.0f)},
    {"BOT",     ImVec4(0.0f, 1.0f, 0.0f, 1.0f)},
    {"HIDDEN",  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)},
    {"KNOCKED", ImVec4(1.0f, 0.65f, 0.0f, 1.0f)},
    {"DEFAULT", ImVec4(1.0f, 1.0f, 1.0f, 1.0f)}
};