#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <chrono>
#include <atomic>
#include <jni.h>
#include <iomanip>
#include <map>
#include <sys/stat.h> 
#include <dirent.h>     
#include <unistd.h>      
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <unordered_map>
#include "Includes/Utils.h"
#include "ImGui/Call_ImGui.h"
#include "BasicStructs/Call_BasicStructs.h"
#include "ANOGS/Call_IL2Cpp.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_additional.h"
#include "ImGui/imgui_impl_android.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/android_native_app_glue.h"
#include "Includes/DOSOBF.h"
#include "ImGui/hooks.h"
#include "ImGui/CLASS.h"
#include "ImGui/Icon.h"
#include "tools/things.h"
#include "ImGui/widgets.hpp"
#include "ImGui/Iconcpp.h"
#include "Hacks/Hacks.h"
#include "Hacks/Feature/WeaponCategoryConfig.h"
#include "codm_attach.h"
#include "codm_esp.h"


static CustomWidgets g_widgets;


ImFont* LOGIN = nullptr;
ImFont* F48 = nullptr;
ImFont* Crozn = nullptr;
static bool checked = false;
static bool isToggled = false;
static void copy_attach_logs(); 

void DrawAndTouch(ImTextureID textureID, ImVec2& position, float radius, int opacity, bool& isToggled) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImVec2(position.x - radius, position.y - radius);
    ImVec2 max = ImVec2(position.x + radius, position.y + radius);
    bool isHovered = ImGui::IsMouseHoveringRect(min, max);

    if (isHovered && ImGui::IsMouseDown(0)) {
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        position.x += mouseDelta.x; 
        position.y += mouseDelta.y; 
    }

    drawList->AddImage(textureID, min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, opacity));

    if (ImGui::IsMouseReleased(0) && isHovered)
        isToggled = !isToggled; 
}



static void draw_imgui_frame(bool gameTouch) {
if (!gameTouch) {
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
}
if (!g_Initialized) {
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Your exact colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.145f, 0.145f, 0.16f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.145f, 0.145f, 0.16f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.135f, 0.135f, 0.15f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.165f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.135f, 0.135f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.135f, 0.135f, 0.15f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.135f, 0.135f, 0.15f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.42f, 0.44f, 0.65f, 0.4f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.42f, 0.44f, 0.65f, 0.6f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.42f, 0.44f, 0.65f, 0.8f);
    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    
    // Add these missing colors
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.42f, 0.44f, 0.65f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.44f, 0.65f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 0.54f, 0.75f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.165f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.165f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.135f, 0.135f, 0.15f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.165f, 1.0f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.42f, 0.44f, 0.65f, 0.6f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.42f, 0.44f, 0.65f, 0.3f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

    style.WindowRounding = 20.0f;
    style.ChildRounding = 20.0f;
    style.FrameRounding = 20.0f;
    style.PopupRounding = 20.0f;
    style.ScrollbarRounding = 20.0f;
    style.GrabRounding = 20.0f;
    style.TabRounding = 20.0f;
    style.LogSliderDeadzone = 20.0f;

    style.Alpha = 1.0f;

    style.FrameBorderSize = 3.58f;
    style.WindowBorderSize = 3.58f;
    style.ChildBorderSize = 3.58f;
    style.PopupBorderSize = 3.58f;

    style.ScrollbarSize = 24.673f;

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;

    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");

ImFontConfig font_cfg;
font_cfg.SizePixels = 20.0f;
LOGIN = io.Fonts->AddFontDefault(&font_cfg);
font_cfg.SizePixels = 48.0f;
F48 = io.Fonts->AddFontDefault(&font_cfg);
font_cfg.SizePixels = 24.0f;
Crozn = io.Fonts->AddFontDefault(&font_cfg);
static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
ImFontConfig icons_config;
icons_config.MergeMode = true;
icons_config.PixelSnapH = true;

io.Fonts->AddFontFromMemoryCompressedTTF(
    font_awesome_data,
    font_awesome_size,
    24.0f,        
    &icons_config,
    icons_ranges
);

io.Fonts->Build();

    g_Initialized = true;
}




ImGuiIO* io = &ImGui::GetIO();
if (gameTouch) ImGui_GetTouch(io, get_height());
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
ImGui::NewFrame();
{
    static bool hookNote = false;
    if (!hookNote && (checkboxStates["AIMBOT 360"] || checkboxStates["WALLHACK"] || checkboxStates["ADVANCE::UAV"] ||
                      checkboxStates["NO::PARACHUTE"] || checkboxStates["FAST::SWIM"] || checkboxStates["HITBOX"] ||
                      checkboxStates["NO::SHAKE"] || checkboxStates["NO CROUCH"] || checkboxStates["AUTO::MISDIRECTION"])) {
        hookNote = true;
        attach_log_line("[esp] combat toggles need the game process. the overlay draws lines and boxes");
    }
}
codm_esp_draw(ImGui::GetBackgroundDrawList(), glWidth, glHeight, checkboxStates["ESP ENABLE"],
              checkboxStates["ESP LINE"], checkboxStates["ESP BOX"] || checkboxStates["3D::BOX"],
              checkboxStates["ESP NAME"], checkboxStates["ESP HEALTH"], checkboxStates["ESP DISTANCE"],
              checkboxStates["ENEMY::COUNTER"]);
if (started) DrawESP(ImGui::GetBackgroundDrawList());


if (checkboxStates["HAND::CAM"]) {
    isToggled = false;
}


if (!checkboxStates["HAND::CAM"]) 
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));

    if (ImGui::Begin("LogoOverlay", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        
        static float fps = 0.0f;
        static float fpsSmoothing = 0.9f;
        float frameFps = 1.0f / ImGui::GetIO().DeltaTime;
        fps = fps * fpsSmoothing + frameFps * (1.0f - fpsSmoothing);

        float size = 40.0f;
        float radius = size * 0.5f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        
        if (ImGui::InvisibleButton("logo_btn", ImVec2(size, size))) {
            isToggled = !isToggled;
        }

        ImVec2 center(pos.x + radius, pos.y + radius);
        draw_list->AddCircleFilled(center, radius, IM_COL32(32, 32, 36, 255));
        draw_list->AddCircle(center, radius, IM_COL32(220, 220, 220, 255), 32, 1.6f);
        const char *mark = "JN";
        ImVec2 markSize = ImGui::CalcTextSize(mark);
        draw_list->AddText(ImVec2(center.x - markSize.x * 0.5f, center.y - markSize.y * 0.5f), IM_COL32_WHITE, mark);

        // Diagonal line
        float lineStartX = pos.x + size + 4.0f;
        float lineStartY = pos.y + size;
        float lineEndX   = lineStartX + 14.0f;
        float lineEndY   = pos.y + 2.0f;
        draw_list->AddLine(
            ImVec2(lineStartX, lineStartY),
            ImVec2(lineEndX, lineEndY),
            IM_COL32(255, 255, 255, 255),
            1.4f
        );

        float textX = lineEndX + 3.0f;
        float textY = pos.y;
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        ImGui::BeginGroup();
        ImGui::Text("FPS %.0f", fps);
        ImGui::SetWindowFontScale(0.72f);
        ImGui::Text(DOS("ECLIPSE"));
        ImGui::Text("%s", attach_status_text());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::EndGroup();
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
///°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°MAIN MENU= OPTIMIZED 
if (isToggled)
{
   
ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
ImVec2 display = ImGui::GetIO().DisplaySize;
float menuW = 830.f;
float menuH = 500.f;
if (display.x > 48.f && menuW > display.x - 24.f) menuW = display.x - 24.f;
if (display.y > 48.f && menuH > display.y - 24.f) menuH = display.y - 24.f;
ImGui::SetNextWindowPos(center2, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always); 
ImGui::PushFont(Crozn);
if (ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoTitleBar))
{
    DrawMacOSHeaderButtons();
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::BeginChild("##ScrollablePage", ImVec2(-1, -1), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    static int tab = 8;
    
    ImGui::Columns(2);
    ImGui::SetColumnOffset(1, 200); 
    
    {
        float buttonWidth = 170.0f;
        float buttonHeight = 66.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 5.0f));
        
        struct TabButton {
            const char* icon;
            int id;
            float anim;
        };
        
        static TabButton buttons[] = {
            { ICON_FA_EYE,        1, 0.0f },
            { ICON_FA_CROSSHAIRS, 2, 0.0f },
            { ICON_FA_MICROCHIP,  3, 0.0f },
         { ICON_FA_BULLSEYE,   7, 0.0f },
            { ICON_FA_COGS,       5, 0.0f },
            { "LOG",              8, 0.0f },
        };
        
        ImGuiIO& io = ImGui::GetIO();
        float speed = 8.0f * io.DeltaTime; 
        
        auto Lerp = [](float a, float b, float t) {
            return a + (b - a) * t;
        };
        
        for (auto& b : buttons) {
            b.anim = Lerp(b.anim, (tab == b.id) ? 1.0f : 0.0f, speed);
            
            ImVec4 iconColor = ImVec4(
                0.42f + 0.20f * b.anim,
                0.44f + 0.20f * b.anim,
                0.65f + 0.15f * b.anim,
                1.0f
            );
            
            ImVec4 btnColor = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
            ImVec4 btnHover = ImVec4(0.22f, 0.22f, 0.26f, 1.0f);
            ImVec4 btnActive = ImVec4(0.26f, 0.26f, 0.30f, 1.0f);
            
            if (tab == b.id) {
                btnColor = ImVec4(0.42f, 0.44f, 0.65f, 0.15f);
                btnHover = ImVec4(0.42f, 0.44f, 0.65f, 0.25f);
                btnActive = ImVec4(0.42f, 0.44f, 0.65f, 0.35f);
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);
            ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
            
            if (ImGui::Button(b.icon, ImVec2(buttonWidth, buttonHeight))) tab = b.id;
            
            ImGui::PopStyleColor(4);
            ImGui::Spacing();
        }
        
        ImGui::PopStyleVar();
        
        ImGui::NextColumn();
        ImGui::BeginChild("right_content", ImVec2(0, 0), true);

        if (tab == 1) {
            if (checkboxStates["ESP ENABLE"]) {
                ImGui::PushStyleColor(ImGuiCol_Button, g_widgets.m_accent);
                if (ImGui::Button("ESP ENABLED", ImVec2(-1, 80))) {
                    checkboxStates["ESP ENABLE"] = false;
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
                if (ImGui::Button("ESP DISABLED", ImVec2(-1, 80))) {
                    checkboxStates["ESP ENABLE"] = true;
                }
                ImGui::PopStyleColor();
            }
            
            ImGui::Columns(2);
            g_widgets.checkbox("Line", &checkboxStates["ESP LINE"]);
            g_widgets.checkbox("2D Box", &checkboxStates["ESP BOX"]);
            g_widgets.checkbox("Name", &checkboxStates["ESP NAME"]);
            g_widgets.checkbox("Vehicle", &checkboxStates["ESP::VEHICLE"]);
            g_widgets.checkbox("3D Box", &checkboxStates["3D::BOX"]);
            g_widgets.checkbox("Armor", &checkboxStates["ESP ARMOR"]);
            
            ImGui::NextColumn();
            g_widgets.checkbox("Alert", &checkboxStates["ESP ALERT 360"]);
            g_widgets.checkbox("Health", &checkboxStates["ESP HEALTH"]);
            g_widgets.checkbox("Distance", &checkboxStates["ESP DISTANCE"]);
            g_widgets.checkbox("Circle", &checkboxStates["ESP CIRCLE"]);
            g_widgets.checkbox("Skeleton", &checkboxStates["SKELETON"]);
            g_widgets.checkbox("Enemy Counter", &checkboxStates["ENEMY::COUNTER"]);
            ImGui::Columns(1);
            
            ImGui::Spacing();
            ImGui::Columns(2, nullptr, false);
            
            ImGui::BeginGroupPanel("Line Position", ImVec2(-1, 0));
            static const char* linePositions[] = { "Bottom", "Up" };
            for (int i = 0; i < IM_ARRAYSIZE(linePositions); i++) {
                ImGui::RadioButton(linePositions[i], &dropdownStates["#LINE_POS"], i);
                if (i < IM_ARRAYSIZE(linePositions) - 1)
                    ImGui::SameLine();
            }
            ImGui::EndGroupPanel();
            
            ImGui::NextColumn();
            ImGui::BeginGroupPanel("2D Box Style", ImVec2(-1, 0));
            static const char* boxStyles[] = { "Outline", "Filled" };
            for (int i = 0; i < IM_ARRAYSIZE(boxStyles); i++) {
                ImGui::RadioButton(boxStyles[i], &dropdownStates["#BOX_STYLE"], i);
                if (i < IM_ARRAYSIZE(boxStyles) - 1)
                    ImGui::SameLine();
            }
            ImGui::EndGroupPanel();
        }
        
        if (tab == 2)
        {
            ImGui::Text(ICON_FA_EXCLAMATION_CIRCLE "  Aim");
            ImGui::Separator();
            
            ImGui::BeginGroupPanel("", ImVec2(-1, 0));
            g_widgets.checkbox("SILENT AIM", &checkboxStates["AIMBOT 360"]);
            g_widgets.checkbox("Ignore Wall", &checkboxStates["DISABLE::VIS"]);
            g_widgets.checkbox("Ignore Knocked Enemy", &checkboxStates["IGNORE KNOCKED"]);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("TUNING", ImVec2(-1, 0));
            SLIDER("AIM SMOOTHNESS", &seekbarValues["AIM SMOOTHNESS"], 1.0f, 100.0f);
            SLIDER("AIM SPEED", &seekbarValues["AIM SPEED"], 1.0f, 100.0f);
            SLIDER("AIM FOV", &seekbarValues["FOV500"], 1.0f, 500.0f);
            SLIDER("[Change Recoil To Current Gun]", &seekbarValues["NO::RECOIL"], 0.0f, 100.0f);
            SLIDER("[change ADS To Current Gun]", &seekbarValues["FAST::SCOPE"], 0.0f, 100.0f);
            SLIDER("[change Reload To Current Gun]", &seekbarValues["NO::RELOAD"], 0.0f, 100.0f);
            SLIDER("[change Spread To Current Gun]", &seekbarValues["NO::SPREAD"], 0.0f, 100.0f);
            SLIDER("Aim Range", &seekbarValues["MAXDIST"], 0.0f, 200.0f);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Aiming Location", ImVec2(-1, 0));
            static const char* targets[] = { "BODY", "HEAD", "CHEST", "FEET" };
            g_widgets.dropdown("##target", &dropdownStates["#TARG"], targets, 4);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Aim Priority", ImVec2(-1, 0));
            static const char* distances[] = { "NEAREST", "FOV" };
            g_widgets.dropdown("##priority", &dropdownStates["#DIST"], distances, 2);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Trigger Condition", ImVec2(-1, 0));
            static const char* triggers[] = { "NONE", "SHOOTING", "SCOPING", "HYBRID" };
            g_widgets.dropdown("##trigger", &dropdownStates["#TRIG"], triggers, 4);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
        }
        
        if (tab == 3)
        {
            ImGui::Text(ICON_FA_MICROCHIP "  Memory");
            ImGui::Separator();
            
            ImGui::BeginGroupPanel("", ImVec2(-1, 0));
            ImGui::Columns(2, nullptr, false);
            g_widgets.checkbox("Wallhack", &checkboxStates["WALLHACK"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Maphack", &checkboxStates["ADVANCE::UAV"]);
            ImGui::NextColumn();
            g_widgets.checkbox("No Parachute", &checkboxStates["NO::PARACHUTE"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Fast Swim", &checkboxStates["FAST::SWIM"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Extend Hitbox", &checkboxStates["HITBOX"]);
            ImGui::NextColumn();
            g_widgets.checkbox("No Shake", &checkboxStates["NO::SHAKE"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Disable Crouch", &checkboxStates["NO CROUCH"]);
            ImGui::NextColumn();
            g_widgets.checkbox("Misdirection Effect", &checkboxStates["AUTO::MISDIRECTION"]);
            ImGui::Columns(1);
            ImGui::EndGroupPanel();
            
            ImGui::Spacing();
            
            ImGui::BeginGroupPanel("Movement Adjustments", ImVec2(-1, 0));
            ImGui::Spacing();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            SLIDER("Aim Assist", &seekbarValues["AIM::ASSIST"], 0.0f, 100.0f);
            SLIDER("Fov BR", &seekbarValues["FOV"], 0.0f, 200.0f);
            SLIDER("Fov MP", &seekbarValues["FOV1"], 0.0f, 200.0f);
            SLIDER("Snowb Speed", &seekbarValues["SizeSnow"], 0.0f, 70.0f); 
            SLIDER("Loot Speed", &seekbarValues["FAST::LOOT"], 0.0f, 100.0f);
            SLIDER("Game Speed", &seekbarValues["SPEED_HACK"], 0.0f, 70.0f); 
            ImGui::PopStyleVar();
            ImGui::EndGroupPanel();
        }
        
        if (tab == 5)
        {
            ImGui::Spacing();
            
            ImGui::BeginChild("##SettingsContent", ImVec2(0, 0), true);
            {
                ImGui::BeginGroupPanel("Performance", ImVec2(-1, 0));
                ImGui::Columns(2, nullptr, false);
                g_widgets.checkbox("Force 120 / 60 Fps", &checkboxStates["FRAME120"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Force 40 Fps", &checkboxStates["FRAME40"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Hand Cam Mode", &checkboxStates["HAND::CAM"]);
                ImGui::NextColumn();
                g_widgets.checkbox("Disable VisCheck", &checkboxStates["DISABLE_VISCHECK"]);
                
                ImGui::Columns(1);
                ImGui::EndGroupPanel();
                
                ImGui::Spacing();
                
                ImGui::BeginGroupPanel("Esp Colors", ImVec2(-1, 0));
                static ImVec4 enemyVisibleColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); 
                static ImVec4 botVisibleColor   = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  
                static ImVec4 notVisibleColor   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  
                static ImVec4 knockedColor      = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
                static ImVec4 defaultEspColor   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                ImGui::ColorEdit4("Enemy",     (float*)&enemyVisibleColor, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine(); 
                ImGui::ColorEdit4("Bot",       (float*)&botVisibleColor,   ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Hidden",    (float*)&notVisibleColor,   ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit4("Knocked",   (float*)&knockedColor,      ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Default",   (float*)&defaultEspColor,   ImGuiColorEditFlags_NoInputs);

                for (auto& color : {&enemyVisibleColor, &botVisibleColor, &notVisibleColor, &knockedColor, &defaultEspColor})
                    color->w = 1.0f;

                colorStates = {
                    {"ENEMY", enemyVisibleColor}, {"BOT", botVisibleColor}, {"HIDDEN", notVisibleColor},
                    {"KNOCKED", knockedColor}, {"DEFAULT", defaultEspColor}
                };
                ImGui::EndGroupPanel();
            }
            ImGui::EndChild();
        }
        if (tab == 7)
        {
            ImGui::Spacing();
            WeaponCfg::DrawTab();
        }
        if (tab == 8)
        {
            ImGui::TextUnformatted("LOG");
            ImGui::TextWrapped("%s", attach_status_text());
            if (ImGui::Button("Copy all logs", ImVec2(-1, 48)))
                copy_attach_logs();
            static char logBuf[20000];
            attach_log_copy(logBuf, sizeof(logBuf));
            ImGui::BeginChild("##attachlog", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(logBuf[0] ? logBuf : "Waiting for scan");
            ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }
        ImGui::EndChild();
        ImGui::Columns(1);
    }
    
    ImGui::EndChild();
}
ImGui::PopFont();
ImGui::End();
}

ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
if (clearMousePos && !io->MouseDown[0]) {
    io->MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    clearMousePos = false;
}
}

static JavaVM *g_vm = nullptr;
static jclass g_bridge = nullptr;
static jmethodID g_copyText = nullptr;

static void copy_attach_logs() {
    char buf[20000];
    attach_log_copy(buf, sizeof(buf));
    if (!g_vm || !g_bridge || !g_copyText) {
        attach_log_line("[log] copy failed, clipboard is not ready");
        return;
    }
    JNIEnv *env = nullptr;
    bool detach = false;
    jint st = g_vm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (st == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != 0) return;
        detach = true;
    } else if (st != JNI_OK || !env) {
        return;
    }
    jstring s = env->NewStringUTF(buf);
    env->CallStaticVoidMethod(g_bridge, g_copyText, s);
    env->DeleteLocalRef(s);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (detach) g_vm->DetachCurrentThread();
}

EGLBoolean (*orig_eglSwapBuffers)(...);
EGLBoolean _eglSwapBuffers (EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
    draw_imgui_frame(true);
    return orig_eglSwapBuffers(dpy, surface);
}

extern "C" JNIEXPORT void JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeSetDisplaySize(JNIEnv *, jclass, jint w, jint h) {
    if (w > 0) glWidth = w;
    if (h > 0) glHeight = h;
}

extern "C" JNIEXPORT void JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeGlDrawFrame(JNIEnv *, jclass) {
    draw_imgui_frame(false);
}

extern "C" JNIEXPORT void JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeTouch(JNIEnv *, jclass, jint action, jfloat x, jfloat y) {
    ImGuiIO &io = ImGui::GetIO();
    if (action == 1 || action == 3) {
        io.MouseDown[0] = false;
        clearMousePos = true;
    } else {
        io.MousePos = ImVec2(x, y);
        io.MouseDown[0] = true;
        clearMousePos = false;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeSetMenuOpen(JNIEnv *, jclass, jboolean open) {
    isToggled = open;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeIsMenuOpen(JNIEnv *, jclass) {
    return isToggled;
}



extern "C" JNIEXPORT jint JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativePollAttach(JNIEnv *, jclass) {
    return attach_state();
}

extern "C" JNIEXPORT void JNICALL
Java_com_jnmaini_overlay_NativeBridge_nativeHintProcess(JNIEnv *env, jclass, jint pid, jstring name) {
    const char *s = name ? env->GetStringUTFChars(name, nullptr) : nullptr;
    attach_hint_pid((int)pid, s ? s : "");
    if (name && s) env->ReleaseStringUTFChars(name, s);
}

void Init_Thread() {
    for (;;) {
        attach_tick();
        sleep(1);
    }
}

__attribute__((constructor))
void native_Init() {
    std::thread(Init_Thread).detach();
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_vm = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK || !env) return JNI_VERSION_1_6;
    jclass cls = env->FindClass("com/jnmaini/overlay/NativeBridge");
    if (cls) {
        g_bridge = (jclass)env->NewGlobalRef(cls);
        g_copyText = env->GetStaticMethodID(cls, "copyText", "(Ljava/lang/String;)V");
        env->DeleteLocalRef(cls);
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    return JNI_VERSION_1_6;
}
