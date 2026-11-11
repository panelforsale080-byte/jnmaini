#pragma once
#include "BasicStructs/Vector3.hpp"
#include "ANOGS/Tools.h"
#include "ImGui/imgui.h"
#include "ImGui/CLASS.h"

enum BoneIndex : int {
    HEAD, NECK, BODY, HIPS,
    LEFT_UPPER_ARM, LEFT_ARM, LEFT_HAND,
    RIGHT_UPPER_ARM, RIGHT_ARM, RIGHT_HAND,
    LEFT_PAHA, LEFT_BETIS, LEFT_TOE,
    RIGHT_PAHA, RIGHT_BETIS, RIGHT_TOE,
    BONE_COUNT
};

static uintptr_t BoneOffsets[BONE_COUNT];

static constexpr int BoneConnections[][2] = {
    {HEAD, NECK}, {NECK, BODY}, {BODY, HIPS},
    {NECK, LEFT_UPPER_ARM}, {LEFT_UPPER_ARM, LEFT_ARM}, {LEFT_ARM, LEFT_HAND},
    {NECK, RIGHT_UPPER_ARM}, {RIGHT_UPPER_ARM, RIGHT_ARM}, {RIGHT_ARM, RIGHT_HAND},
    {HIPS, LEFT_PAHA}, {LEFT_PAHA, LEFT_BETIS}, {LEFT_BETIS, LEFT_TOE},
    {HIPS, RIGHT_PAHA}, {RIGHT_PAHA, RIGHT_BETIS}, {RIGHT_BETIS, RIGHT_TOE}
};
static constexpr int BoneConnectionCount = sizeof(BoneConnections) / sizeof(BoneConnections[0]);

static inline void InitBoneOffsets() {
    static bool init = false;
    if (init) return;
    BoneOffsets[HEAD] = FastOffsets::BoneMap_m_Head;
    BoneOffsets[NECK] = FastOffsets::BoneMap_m_Neck;
    BoneOffsets[BODY] = FastOffsets::BoneMap_m_Body;
    BoneOffsets[HIPS] = FastOffsets::BoneMap_m_Hips;
    BoneOffsets[LEFT_UPPER_ARM] = FastOffsets::BoneMap_m_LeftUpperArm;
    BoneOffsets[LEFT_ARM] = FastOffsets::BoneMap_m_LeftArm;
    BoneOffsets[LEFT_HAND] = FastOffsets::BoneMap_m_LeftHand;
    BoneOffsets[RIGHT_UPPER_ARM] = FastOffsets::BoneMap_m_RightUpperArm;
    BoneOffsets[RIGHT_ARM] = FastOffsets::BoneMap_m_RightArm;
    BoneOffsets[RIGHT_HAND] = FastOffsets::BoneMap_m_RightHand;
    BoneOffsets[LEFT_PAHA] = FastOffsets::BoneMap_m_LeftPaha;
    BoneOffsets[LEFT_BETIS] = FastOffsets::BoneMap_m_LeftBetis;
    BoneOffsets[LEFT_TOE] = FastOffsets::BoneMap_m_LeftToe;
    BoneOffsets[RIGHT_PAHA] = FastOffsets::BoneMap_m_RightPaha;
    BoneOffsets[RIGHT_BETIS] = FastOffsets::BoneMap_m_RightBetis;
    BoneOffsets[RIGHT_TOE] = FastOffsets::BoneMap_m_RightToe;
    init = true;
}

static inline bool IsValidAddress(uintptr_t address) {
    return (address > 0x10000 && address < 0x7FFFFFFFFFFF);
}

static inline uintptr_t SafeReadU64(uintptr_t address) {
    if (!IsValidAddress(address)) return 0;
    return *reinterpret_cast<uintptr_t*>(address);
}

static inline Vector3 GetBonePosition(uintptr_t boneMap, uintptr_t boneOffset) {
    uintptr_t table = SafeReadU64(boneMap + Class_Pawn_m_BoneTable);
    if (!IsValidAddress(table)) return Vector3::zero();
    uintptr_t boneNode = SafeReadU64(table + boneOffset);
    if (!IsValidAddress(boneNode)) return Vector3::zero();
    uintptr_t boneTransform = SafeReadU64(boneNode + Class_Pawn_m_BoneNode);
    if (!IsValidAddress(boneTransform)) return Vector3::zero();
    if (FastFuncs::Transform_get_position_Fast)
        return FastFuncs::Transform_get_position_Fast(reinterpret_cast<Transform*>(boneTransform));
    return reinterpret_cast<Transform*>(boneTransform)->get_position();
}

/* Pass cam + scrH from frame cache — no Camera::get_main() per skeleton */
static inline void DrawSkeleton(ImDrawList* draw, Pawn* pawn, Camera* cam, float scrH) {
    if (!draw || !pawn || !cam || scrH <= 0.f) return;
    if (!pawn->m_IsAlive()) return;

    InitBoneOffsets();

    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(pawn);
    if (!IsValidAddress(pawnAddr)) return;

    uintptr_t boneMap = SafeReadU64(pawnAddr + Class_Pawn_m_BoneMappings);
    if (!IsValidAddress(boneMap)) return;

    auto w2s = FastFuncs::WorldToScreen_Fast;

    Vector3 screen[BONE_COUNT];
    bool valid[BONE_COUNT] = {};
    int validCount = 0;

    for (int i = 0; i < BONE_COUNT; i++) {
        Vector3 world = GetBonePosition(boneMap, BoneOffsets[i]);
        if (world.x == 0.f && world.y == 0.f && world.z == 0.f) continue;
        Vector3 sc = w2s ? w2s(cam, world) : cam->WorldToScreenPoint(world);
        if (sc.z <= 0.05f) continue;
        screen[i] = sc;
        valid[i] = true;
        validCount++;
    }
    if (validCount < 3) return;

    ImVec4 col = colorStates["DEFAULT"];
    ImU32 color = IM_COL32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), (int)(col.w * 255));
    const float thickness = 1.5f;
    const float pad = 500.f;

    for (int i = 0; i < BoneConnectionCount; i++) {
        int a = BoneConnections[i][0];
        int b = BoneConnections[i][1];
        if (!valid[a] || !valid[b]) continue;
        float x1 = screen[a].x, y1 = scrH - screen[a].y;
        float x2 = screen[b].x, y2 = scrH - screen[b].y;
        if (x1 < -pad || x2 < -pad || y1 < -pad || y2 < -pad) continue;
        draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
    }
}

/* Back-compat overload */
static inline void DrawSkeleton(ImDrawList* draw, Pawn* pawn) {
    Camera* cam = FastFuncs::Camera_get_main_Fast ? FastFuncs::Camera_get_main_Fast() : Camera::get_main();
    float h = ImGui::GetIO().DisplaySize.y;
    DrawSkeleton(draw, pawn, cam, h);
}
