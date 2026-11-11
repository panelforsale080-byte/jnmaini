#pragma once
/*
 * Per-category settings — 1 offset only (in CLASS.h):
 *   FastOffsets::offset_Inventory_get_CategoryName = 0xC36358C
 * LocalPawn + CurrentWeapon come from FastFuncs (espoffset).
 * FloatMap returns float& so std::clamp works without editing DrawESP.
 */
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ImGui/CLASS.h"

#ifdef IMGUI_VERSION
#include "ImGui/Iconcpp.h"
#include "ImGui/Icon.h"
#endif

namespace WeaponCfg {

enum Cat : int { AR = 0, SNIPER, SMG, LMG, SHOTGUN, PISTOL, MARKSMAN, COUNT };

inline const char* CatName(int c) {
    static const char* n[] = { "AR", "Sniper", "SMG", "LMG", "Shotgun", "Pistol", "Marksman" };
    return (c >= 0 && c < COUNT) ? n[c] : "AR";
}

inline const char* CatIcon(int c) {
#ifdef IMGUI_VERSION
    static const char* ico[] = {
        ICON_FA_CROSSHAIRS, ICON_FA_BULLSEYE, ICON_FA_BOLT, ICON_FA_FIRE,
        ICON_FA_BOMB, ICON_FA_DOT_CIRCLE, ICON_FA_STAR
    };
    return (c >= 0 && c < COUNT) ? ico[c] : ICON_FA_CROSSHAIRS;
#else
    (void)c;
    return "";
#endif
}

inline int FromECat(int e) {
    switch (e) {
        case 1:  return AR;
        case 2:  return SNIPER;
        case 3:  return SMG;
        case 4:  return LMG;
        case 5:  return SHOTGUN;
        case 6:  return PISTOL;
        case 13: return MARKSMAN;
        default: return AR;
    }
}

struct CatCfg {
    bool  hitbox = false, no_shake = false;
    float aim_assist = 0.f, no_recoil = 0.f, no_reload = 0.f, no_spread = 0.f, fast_scope = 0.f;
};

inline std::array<CatCfg, COUNT>& Store() {
    static std::array<CatCfg, COUNT> s{};
    return s;
}

inline bool IsGated(const std::string& k) {
    static const std::unordered_set<std::string> keys = {
        "HITBOX", "NO::SHAKE",
        "AIM::ASSIST", "NO::RECOIL", "NO::RELOAD", "NO::SPREAD", "FAST::SCOPE"
    };
    return keys.count(k) != 0;
}

/* Held weapon category — only needs CategoryName from CLASS (1 offset) */
inline int Held() {
    auto local  = FastFuncs::Gameplay_get_LocalPawn_Fast;
    auto weapon = FastFuncs::Pawn_get_CurrentWeapon_Fast;
    auto cat    = FastFuncs::Inventory_get_CategoryName_Fast;
    if (!local || !weapon || !cat) return AR;

    Pawn* p = local();
    if (!p) return AR;
    void* w = weapon(p);  // Pawn* OK
    if (!w) return AR;

    int e = cat(w, nullptr);
    if (e < 0 || e > 14) return AR;
    return FromECat(e);
}

inline bool* BoolField(const std::string& k, int cat) {
    CatCfg& c = Store()[cat];
    if (k == "HITBOX")    return &c.hitbox;
    if (k == "NO::SHAKE") return &c.no_shake;
    return nullptr;
}

inline float* FloatField(const std::string& k, int cat) {
    CatCfg& c = Store()[cat];
    if (k == "AIM::ASSIST") return &c.aim_assist;
    if (k == "NO::RECOIL")  return &c.no_recoil;
    if (k == "NO::RELOAD")  return &c.no_reload;
    if (k == "NO::SPREAD")  return &c.no_spread;
    if (k == "FAST::SCOPE") return &c.fast_scope;
    return nullptr;
}

inline bool ReadBool(const std::string& k) {
    bool* p = BoolField(k, Held());
    return p ? *p : false;
}

inline float ReadFloat(const std::string& k) {
    float* p = FloatField(k, Held());
    return p ? *p : 0.f;
}

struct BoolMap {
    std::unordered_map<std::string, bool> m;
    struct Ref {
        BoolMap* s; std::string k;
        operator bool() const {
            if (IsGated(k)) return ReadBool(k);
            auto it = s->m.find(k);
            return it != s->m.end() && it->second;
        }
        Ref& operator=(bool v) {
            if (IsGated(k)) {
                if (bool* p = BoolField(k, Held())) *p = v;
            } else {
                s->m[k] = v;
            }
            return *this;
        }
        bool* ptr() {
            if (IsGated(k)) {
                if (bool* p = BoolField(k, Held())) return p;
            }
            return &s->m[k];
        }
    };
    Ref operator[](const std::string& k) { return { this, k }; }
    Ref operator[](const char* k)        { return { this, std::string(k) }; }
    auto& raw() { return m; }
    auto begin() { return m.begin(); }
    auto end()   { return m.end(); }
    auto begin() const { return m.begin(); }
    auto end()   const { return m.end(); }
};
inline bool* operator&(BoolMap::Ref r) { return r.ptr(); }

/* float& → std::clamp(seekbarValues["x"], ...) works in DrawESP */
struct FloatMap {
    std::unordered_map<std::string, float> m;

    float& operator[](const std::string& k) {
        if (IsGated(k)) {
            if (float* p = FloatField(k, Held())) return *p;
        }
        return m[k];
    }
    float& operator[](const char* k) { return (*this)[std::string(k)]; }

    float operator[](const std::string& k) const {
        if (IsGated(k)) return ReadFloat(k);
        auto it = m.find(k);
        return it != m.end() ? it->second : 0.f;
    }
    float operator[](const char* k) const { return (*this)[std::string(k)]; }

    auto& raw() { return m; }
    auto begin() { return m.begin(); }
    auto end()   { return m.end(); }
    auto begin() const { return m.begin(); }
    auto end()   const { return m.end(); }
};

inline int& UiCat() {
    static int c = AR;
    return c;
}

#ifdef IMGUI_VERSION
inline void WCSlider(const char* label, float* v, float v_min, float v_max) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = ImGui::GetFrameHeight();

    ImVec2 label_size = ImGui::CalcTextSize(label);
    char value_buf[32];
    snprintf(value_buf, sizeof(value_buf), "%.0f", *v);
    ImVec2 value_size = ImGui::CalcTextSize(value_buf);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), label);
    draw->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), value_buf);

    float text_h = (label_size.y > value_size.y) ? label_size.y : value_size.y;
    pos.y += text_h + style.ItemInnerSpacing.y;
    ImGui::SetCursorScreenPos(pos);

    ImGui::InvisibleButton(label, ImVec2(width, height));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float t = (ImGui::GetMousePos().x - pos.x) / width;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        *v = v_min + (v_max - v_min) * t;
    }

    float track_h = height * 0.45f;
    float t = (*v - v_min) / (v_max - v_min);
    float grab = pos.x + width * t;

    draw->AddRectFilled(
        ImVec2(pos.x, pos.y + height * 0.5f - track_h * 0.5f),
        ImVec2(pos.x + width, pos.y + height * 0.5f + track_h * 0.5f),
        ImGui::GetColorU32(ImGuiCol_FrameBg), track_h * 0.5f);
    draw->AddRectFilled(
        ImVec2(pos.x, pos.y + height * 0.5f - track_h * 0.5f),
        ImVec2(grab, pos.y + height * 0.5f + track_h * 0.5f),
        ImGui::GetColorU32(ImGuiCol_SliderGrabActive), track_h * 0.5f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + height + style.ItemSpacing.y));
}
#endif

inline void DrawTab() {
#ifdef IMGUI_VERSION
    if (ImGui::BeginTabBar("##wc_tabs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
        for (int c = 0; c < COUNT; ++c) {
            char lab[48];
            snprintf(lab, sizeof(lab), "%s %s", CatIcon(c), CatName(c));
            if (ImGui::BeginTabItem(lab)) {
                UiCat() = c;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    CatCfg& cfg = Store()[UiCat()];

    ImGui::Columns(2, "##wc_bool", false);
    ImGui::Checkbox("Extend Hitbox", &cfg.hitbox);
    ImGui::NextColumn();
    ImGui::Checkbox("No Shake", &cfg.no_shake);
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    WCSlider("[Change Aim To Current Gun",  &cfg.aim_assist, 0.f, 100.f);
    WCSlider("[Change Recoil To Current Gun]", &cfg.no_recoil,  0.f, 100.f);
    WCSlider("[change Reload To Current Gun]", &cfg.no_reload,  0.f, 100.f);
    WCSlider("[change Spread To Current Gun]", &cfg.no_spread,  0.f, 100.f);
    WCSlider("[change ADS To Current Gun]",    &cfg.fast_scope, 0.f, 100.f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Copy to All", ImVec2(120, 28))) {
        const CatCfg src = cfg;
        for (int i = 0; i < COUNT; ++i) Store()[i] = src;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset This", ImVec2(120, 28))) {
        cfg = CatCfg{};
    }
#endif
}

} // namespace WeaponCfg

inline WeaponCfg::BoolMap  checkboxStates;
inline WeaponCfg::FloatMap seekbarValues;

struct _WCInit {
    _WCInit() { seekbarValues.raw()["MAXDIST"] = 200.f; }
};
inline _WCInit _wcInit;
