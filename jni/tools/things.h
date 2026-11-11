#pragma once

//CUSTOM SLIDER
float ImLerp(float a, float b, float t) { return a + (b - a) * t; }

float ImClamp(float v, float min, float max) {
    return (v < min) ? min : (v > max) ? max : v;
}

// Custom slider function
bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.0f") {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = ImGui::GetFrameHeight();

    // Compute label and value sizes
    ImVec2 label_size = ImGui::CalcTextSize(label);
    char value_buf[32];
    snprintf(value_buf, sizeof(value_buf), format, *v);
    ImVec2 value_size = ImGui::CalcTextSize(value_buf);

    // Get draw list
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw label (top-left)
    draw_list->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    // Draw value (top-right)
    draw_list->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), value_buf);

    // Move cursor below label/value
    float text_height = (label_size.y > value_size.y) ? label_size.y : value_size.y;
    pos.y += text_height + style.ItemInnerSpacing.y;
    ImGui::SetCursorScreenPos(pos);

    // Slider logic
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

    // Increased track thickness
    float track_height = height * 0.45f; // Previously 0.15f
    float t = (*v - v_min) / (v_max - v_min);
    float grab_pos = pos.x + width * t;

    // Draw slider background (track)
    draw_list->AddRectFilled(
        ImVec2(pos.x, pos.y + height / 2 - track_height / 2),
        ImVec2(pos.x + width, pos.y + height / 2 + track_height / 2),
        ImGui::GetColorU32(ImGuiCol_FrameBg),
        track_height * 0.5f
    );

    // Draw filled portion of the slider
    draw_list->AddRectFilled(
        ImVec2(pos.x, pos.y + height / 2 - track_height / 2),
        ImVec2(grab_pos, pos.y + height / 2 + track_height / 2),
        ImGui::GetColorU32(ImGuiCol_SliderGrabActive),
        track_height * 0.5f
    );

    return changed;
}

#define SLIDER(label, v, v_min, v_max, ...) CustomSliderFloat(label, v, v_min, v_max, ##__VA_ARGS__)
//CUSTOM SLIDER END.

void DrawMacOSHeaderButtons()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winPad = ImGui::GetStyle().WindowPadding;

    float radius = 6.5f;
    float spacing = 18.0f;

    ImVec2 circlePos = ImVec2(
        winPos.x + winPad.x + radius + 2,
        winPos.y + winPad.y + radius + 2
    );

    ImU32 colRed   = IM_COL32(255, 92, 85, 255);
    ImU32 colYellow= IM_COL32(255, 189, 47, 255);
    ImU32 colGreen = IM_COL32(40, 201, 64, 255);

    draw->AddCircleFilled(circlePos, radius, colRed);
    draw->AddCircleFilled(ImVec2(circlePos.x + spacing, circlePos.y), radius, colYellow);
    draw->AddCircleFilled(ImVec2(circlePos.x + spacing * 2, circlePos.y), radius, colGreen);

    // Optional invisible buttons for clicks
    ImGui::SetCursorScreenPos(ImVec2(circlePos.x - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("closeBtn", ImVec2(radius * 2, radius * 2)))
        ; // Action here

    ImGui::SetCursorScreenPos(ImVec2(circlePos.x + spacing - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("minBtn", ImVec2(radius * 2, radius * 2)))
        ; // Action here

    ImGui::SetCursorScreenPos(ImVec2(circlePos.x + spacing * 2 - radius, circlePos.y - radius));
    if (ImGui::InvisibleButton("maxBtn", ImVec2(radius * 2, radius * 2)))
        ; // Action here
}

std::map<std::string, ImVec4> colorStates = {
    {"ENEMY",   ImVec4(1.0f, 0.0f, 0.0f, 1.0f)},
    {"BOT",     ImVec4(0.0f, 1.0f, 0.0f, 1.0f)},
    {"HIDDEN",  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)},
    {"KNOCKED", ImVec4(1.0f, 0.65f, 0.0f, 1.0f)},
    {"DEFAULT", ImVec4(1.0f, 1.0f, 1.0f, 1.0f)}
};


