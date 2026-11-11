#pragma once
#include "BasicStructs/Vector3.hpp"
#include "ANOGS/Tools.h"
#include "ImGui/imgui.h"
#include "ImGui/CLASS.h"

/* UnityEngine.HumanBodyBones values from Dump 1.6.59. These are enum ids, not memory offsets. */
enum BoneIndex : int {
    HIPS, SPINE, CHEST, NECK, HEAD,
    L_SHOULDER, L_UPPER, L_LOWER, L_HAND,
    R_SHOULDER, R_UPPER, R_LOWER, R_HAND,
    L_UPPER_LEG, L_LOWER_LEG, L_FOOT,
    R_UPPER_LEG, R_LOWER_LEG, R_FOOT,
    BONE_COUNT
};

static constexpr int kHumanBone[BONE_COUNT] = {
    0, 7, 8, 9, 10,
    11, 13, 15, 17,
    12, 14, 16, 18,
    1, 3, 5,
    2, 4, 6
};

static constexpr int BoneConnections[][2] = {
    {HEAD, NECK}, {NECK, CHEST}, {CHEST, SPINE}, {SPINE, HIPS},
    {NECK, L_SHOULDER}, {L_SHOULDER, L_UPPER}, {L_UPPER, L_LOWER}, {L_LOWER, L_HAND},
    {NECK, R_SHOULDER}, {R_SHOULDER, R_UPPER}, {R_UPPER, R_LOWER}, {R_LOWER, R_HAND},
    {HIPS, L_UPPER_LEG}, {L_UPPER_LEG, L_LOWER_LEG}, {L_LOWER_LEG, L_FOOT},
    {HIPS, R_UPPER_LEG}, {R_UPPER_LEG, R_LOWER_LEG}, {R_LOWER_LEG, R_FOOT}
};
static constexpr int BoneConnectionCount = sizeof(BoneConnections) / sizeof(BoneConnections[0]);

static inline bool IsValidAddress(uintptr_t address) {
    return (address > 0x10000 && address < 0x7FFFFFFFFFFF);
}

struct BoneApi {
    void *(*getComponent)(void *, Il2CppString *);
    Transform *(*getBone)(void *, int);
    Il2CppString *animatorName;
    uintptr_t head;
    uintptr_t neck;
    uintptr_t spine;
    uintptr_t hips;
    uintptr_t leftAnkle;
    uintptr_t rightAnkle;
    uintptr_t modelRoot;
    bool ready;
};

static inline BoneApi &bone_api() {
    static BoneApi api{};
    if (api.ready) return api;
    api.ready = true;
    const char *unity[] = {"UnityEngine.CoreModule.dll", "UnityEngine.dll", nullptr};
    const char *stringNames[] = {"System.String", "string", nullptr};
    for (int i = 0; unity[i]; ++i) {
        if (!api.getComponent) {
            for (int n = 0; stringNames[n]; ++n) {
                char *args[] = {const_cast<char *>(stringNames[n])};
                api.getComponent = (void *(*)(void *, Il2CppString *))Il2CppGetMethodOffset(
                    unity[i], "UnityEngine", "Component", "GetComponent", args, 1);
                if (api.getComponent) break;
            }
        }
        if (!api.getBone)
            api.getBone = (Transform *(*)(void *, int))Il2CppGetMethodOffset(
                unity[i], "UnityEngine", "Animator", "GetBoneTransform", 1);
    }
    if (api.getComponent)
        api.animatorName = Il2CppString::Create("Animator");

    auto field = [](const char *cls, const char *name) -> uintptr_t {
        size_t off = Il2CppGetFieldOffset("Assembly-CSharp.dll", "GameBase", cls, name);
        if (off == (size_t)-1 || off >= 0x4000) return 0;
        return (uintptr_t)off;
    };
    api.head = field("Pawn", "m_HeadBone");
    api.modelRoot = field("Pawn", "m_ModelRoot");
    api.neck = field("PlayerPawn", "m_NeckBone");
    api.spine = field("PlayerPawn", "m_SpineBone");
    api.hips = field("PlayerPawn", "m_HipsBone");
    api.leftAnkle = field("PlayerPawn", "m_LeftAnkleBone");
    api.rightAnkle = field("PlayerPawn", "m_RightAnkleBone");
    return api;
}

static inline Transform *ReadTransformField(void *object, uintptr_t offset) {
    if (!object || !offset) return nullptr;
    uintptr_t slot = reinterpret_cast<uintptr_t>(object) + offset;
    if (!IsValidAddress(slot)) return nullptr;
    uintptr_t value = *reinterpret_cast<uintptr_t *>(slot);
    if (!IsValidAddress(value)) return nullptr;
    return reinterpret_cast<Transform *>(value);
}

static inline Vector3 TransformWorld(Transform *transform) {
    if (!transform) return Vector3::zero();
    if (FastFuncs::Transform_get_position_Fast)
        return FastFuncs::Transform_get_position_Fast(transform);
    return transform->get_position();
}

static inline void *FindAnimator(Pawn *pawn) {
    BoneApi &api = bone_api();
    if (!api.getComponent || !api.animatorName || !pawn) return nullptr;
    Transform *mesh = pawn->m_Mesh();
    if (mesh) {
        void *animator = api.getComponent(mesh, api.animatorName);
        if (animator) return animator;
    }
    Transform *root = ReadTransformField(pawn, api.modelRoot);
    if (root) return api.getComponent(root, api.animatorName);
    return nullptr;
}

static inline void DrawSkeleton(ImDrawList *draw, Pawn *pawn, Camera *cam, float scrH) {
    if (!draw || !pawn || !cam || scrH <= 0.f) return;
    if (!pawn->m_IsAlive()) return;

    BoneApi &api = bone_api();
    Vector3 screen[BONE_COUNT];
    bool valid[BONE_COUNT] = {};
    int validCount = 0;
    auto w2s = FastFuncs::WorldToScreen_Fast;

    auto accept = [&](int index, Vector3 world) {
        if (world.x == 0.f && world.y == 0.f && world.z == 0.f) return;
        Vector3 sc = w2s ? w2s(cam, world) : cam->WorldToScreenPoint(world);
        if (sc.z <= 0.05f) return;
        screen[index] = sc;
        valid[index] = true;
        validCount++;
    };

    void *animator = FindAnimator(pawn);
    if (animator && api.getBone) {
        for (int i = 0; i < BONE_COUNT; i++)
            accept(i, TransformWorld(api.getBone(animator, kHumanBone[i])));
    }
    if (validCount < 3) {
        validCount = 0;
        for (int i = 0; i < BONE_COUNT; i++) valid[i] = false;
        accept(HEAD, TransformWorld(ReadTransformField(pawn, api.head)));
        accept(NECK, TransformWorld(ReadTransformField(pawn, api.neck)));
        accept(SPINE, TransformWorld(ReadTransformField(pawn, api.spine)));
        accept(HIPS, TransformWorld(ReadTransformField(pawn, api.hips)));
        accept(L_FOOT, TransformWorld(ReadTransformField(pawn, api.leftAnkle)));
        accept(R_FOOT, TransformWorld(ReadTransformField(pawn, api.rightAnkle)));
    }
    if (validCount < 2) return;

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

static inline void DrawSkeleton(ImDrawList *draw, Pawn *pawn) {
    Camera *cam = FastFuncs::Camera_get_main_Fast ? FastFuncs::Camera_get_main_Fast() : Camera::get_main();
    float h = ImGui::GetIO().DisplaySize.y;
    DrawSkeleton(draw, pawn, cam, h);
}
