#pragma once
#include <cstdint>
#include <string>
#include <unistd.h>
#include "ANOGS/Il2Cpp.h"
#include "Includes/DOSOBF.h"
#include "Hacks/StructGame/GameClass.h"
#ifndef GET_OFF
#define GET_OFF(offset) get_off(offset)
#endif
bool started = false;
namespace FastOffsets {
/* Skeleton bone chain. Dump 1.6.59 has no field named m_BoneMappings. */
static const uintptr_t BoneMap_m_BoneMappings = 0xA8;
static const uintptr_t BoneMap_m_BoneTable = 0x10;
static const uintptr_t BoneMap_m_BoneNode = 0x40;
static const uintptr_t BoneMap_m_Head = 0x70;
static const uintptr_t BoneMap_m_Neck = 0x90;
static const uintptr_t BoneMap_m_Body = 0x98;
static const uintptr_t BoneMap_m_Hips = 0x50;
static const uintptr_t BoneMap_m_LeftUpperArm = 0x68;
static const uintptr_t BoneMap_m_LeftArm = 0x60;
static const uintptr_t BoneMap_m_LeftHand = 0x58;
static const uintptr_t BoneMap_m_RightUpperArm = 0x88;
static const uintptr_t BoneMap_m_RightArm = 0x80;
static const uintptr_t BoneMap_m_RightHand = 0x78;
static const uintptr_t BoneMap_m_LeftPaha = 0x30;
static const uintptr_t BoneMap_m_LeftBetis = 0x28;
static const uintptr_t BoneMap_m_LeftToe = 0x20;
static const uintptr_t BoneMap_m_RightPaha = 0x48;
static const uintptr_t BoneMap_m_RightBetis = 0x40;
static const uintptr_t BoneMap_m_RightToe = 0x38;
}


extern uintptr_t Class_Camera_get_main;
extern uintptr_t Class_Camera_WorldToScreenPoint;
extern uintptr_t Class_Component_get_transform;
extern uintptr_t Class_Transform_get_position;
extern uintptr_t Class_Gameplay_get_MatchGame;
extern uintptr_t Class_Gameplay_get_LocalPawn;
extern uintptr_t Class_Pawn_get_IsFiring;
extern uintptr_t Class_Pawn_IsAiming;
extern uintptr_t Class_Pawn_set_AimRotation;
extern uintptr_t Class_Pawn_get_LastPawnPos;
extern uintptr_t Class_Pawn_get_HeadPosition;
extern uintptr_t Class_Pawn_get_PlayerName;
extern uintptr_t Class_Pawn_get_TeamSeatId;
extern uintptr_t Input_GetTouch;
extern uintptr_t Input_get_touchSupported;
extern uintptr_t Input_get_touchCount;
extern uintptr_t Input_get_mousePosition;
extern uintptr_t Screen_get_width;
extern uintptr_t Screen_get_height;
extern uintptr_t Screen_get_dpi;
extern uintptr_t Class_Pawn_get_AimRotation;
extern uintptr_t Class_Pawn_m_IsBot;
extern uintptr_t Class_Pawn_m_IsAlive;
extern uintptr_t Class_Pawn_m_Mesh;
extern uintptr_t Class_Pawn_m_HeadBone;
extern uintptr_t Class_AttackableTarget__m_AttackableInfo;
extern uintptr_t Class_AttackableTarget__m_Health;
extern uintptr_t Class_AttackableTarget__m_MaxHealth;
extern uintptr_t Class_BaseGame__m_AllVehicles;
extern uintptr_t Class_BaseGame_EnemyPawns;
extern uintptr_t Class_Pawn_m_LastFireWeaponName;
extern uintptr_t Class_Pawn_LastCanSeeState;
extern uintptr_t Class_Transform_get_forward;
extern uintptr_t Class_AttackableTarget_PlayerState;
extern uintptr_t Class_Pawn_m_PlayerInfo;
extern uintptr_t Class_PlayerInfo_m_VestValue;
extern uintptr_t Class_PlayerInfo_m_VestMaxValue;
extern uintptr_t Class_Pawn_m_BoneMappings;
extern uintptr_t Class_Pawn_m_BoneTable;
extern uintptr_t Class_Pawn_m_BoneNode;
extern uintptr_t Pawn_get_CurrentWeapon;
extern uintptr_t Inventory_get_CategoryName;
extern uintptr_t Weapon_GetWeaponId;
namespace FastFuncs {
    inline Camera* (*Camera_get_main_Fast)() = nullptr;
    inline Vector3 (*WorldToScreen_Fast)(Camera*, Vector3) = nullptr;
    inline Transform* (*Component_get_transform_Fast)(Component*) = nullptr;
    inline Vector3 (*Transform_get_position_Fast)(Transform*) = nullptr;
    inline MatchGame* (*Gameplay_get_MatchGame_Fast)() = nullptr;
    inline Pawn* (*Gameplay_get_LocalPawn_Fast)() = nullptr;
    inline bool (*Pawn_get_IsFiring_Fast)(Pawn*) = nullptr;
    inline bool (*Pawn_IsAiming_Fast)(Pawn*) = nullptr;
    inline void (*Pawn_set_AimRotation_Fast)(Pawn*, Quaternion) = nullptr;
    inline Vector3 (*Pawn_get_LastPawnPos_Fast)(Pawn*) = nullptr;
    inline Vector3 (*Pawn_get_HeadPosition_Fast)(Pawn*) = nullptr;
    inline String* (*Pawn_get_PlayerName_Fast)(Pawn*) = nullptr;
    inline unsigned int* (*Pawn_get_TeamSeatId_Fast)(Pawn*) = nullptr;
    inline void* (*Input_GetTouch_Fast)(void*, int) = nullptr;
    inline bool (*Input_get_touchSupported_Fast)(void*) = nullptr;
    inline int (*Input_get_touchCount_Fast)() = nullptr;
    inline Vector3 (*Input_get_mousePosition_Fast)(void*) = nullptr;
    inline int (*Screen_get_width_Fast)() = nullptr;
    inline int (*Screen_get_height_Fast)() = nullptr;
    inline float (*Screen_get_dpi_Fast)() = nullptr;
    inline Quaternion (*Pawn_get_AimRotation_Fast)(Pawn*) = nullptr;
    inline Vector3 (*Transform_get_forward_Fast)(Transform*) = nullptr;
    inline int (*GetPlayerState_Fast)(AttackableTargetInfo*) = nullptr;
    inline void* (*Pawn_get_CurrentWeapon_Fast)(Pawn*) = nullptr;
    inline int (*Inventory_get_CategoryName_Fast)(void*, const void*) = nullptr;
    inline int (*Weapon_GetWeaponId_Fast)(void*) = nullptr;
}

template<typename T>
inline T ReadFieldFast(void* object, size_t offset) {
    return *reinterpret_cast<T*>((uintptr_t)object + offset);
}

namespace {
static void *resolve_method(const char *const *images, const char *ns, const char *cls,
                            const char *name, int argc) {
    for (int i = 0; images[i]; ++i) {
        void *fn = Il2CppGetMethodOffset(images[i], ns, cls, name, argc);
        if (fn) return fn;
    }
    return nullptr;
}

static uintptr_t resolve_field(const char *const *images, const char *ns, const char *cls,
                               const char *name) {
    for (int i = 0; images[i]; ++i) {
        size_t off = Il2CppGetFieldOffset(images[i], ns, cls, name);
        if (off != (size_t)-1 && off < 0x2000) return (uintptr_t)off;
    }
    return 0;
}

static bool attach_il2cpp() {
    const char *libs[] = {"libil2cpp.so", "libunity.so", nullptr};
    for (int i = 0; libs[i]; ++i) {
        void *handle = dlopen(libs[i], RTLD_NOW);
        if (!handle) continue;
        void *sym = dlsym(handle, "il2cpp_domain_get");
        dlclose(handle);
        if (!sym) continue;
        Il2CppAttach(libs[i]);
        return true;
    }
    return false;
}
} // namespace

inline void espoffset() {
    static bool initialized = false;
    if (initialized) return;
    if (!attach_il2cpp()) return;
    for (int i = 0; i < 30 && !Il2CppIsAssembliesLoaded(); ++i) sleep(1);
    if (!Il2CppIsAssembliesLoaded()) return;

    static const char *kUnity[] = {"UnityEngine.CoreModule.dll", "UnityEngine.dll", nullptr};
    static const char *kGame[] = {"Assembly-CSharp.dll", nullptr};

    auto um = [&](const char *cls, const char *name, int argc) {
        return (uintptr_t)resolve_method(kUnity, "UnityEngine", cls, name, argc);
    };
    auto gm = [&](const char *ns, const char *cls, const char *name, int argc) {
        return (uintptr_t)resolve_method(kGame, ns, cls, name, argc);
    };
    auto gf = [&](const char *ns, const char *cls, const char *name) {
        return resolve_field(kGame, ns, cls, name);
    };

    Class_Camera_get_main = um("Camera", "get_main", 0);
    Class_Camera_WorldToScreenPoint = um("Camera", "WorldToScreenPoint", 1);
    Class_Component_get_transform = um("Component", "get_transform", 0);
    Class_Transform_get_position = um("Transform", "get_position", 0);
    Class_Transform_get_forward = um("Transform", "get_forward", 0);
    Input_GetTouch = um("Input", "GetTouch", 1);
    Input_get_touchSupported = um("Input", "get_touchSupported", 0);
    Input_get_touchCount = um("Input", "get_touchCount", 0);
    Input_get_mousePosition = um("Input", "get_mousePosition", 0);
    Screen_get_width = um("Screen", "get_width", 0);
    Screen_get_height = um("Screen", "get_height", 0);
    Screen_get_dpi = um("Screen", "get_dpi", 0);

    Class_Gameplay_get_MatchGame = gm("GameEngine", "GamePlay", "get_MatchGame", 0);
    Class_Gameplay_get_LocalPawn = gm("GameEngine", "GamePlay", "get_LocalPawn", 0);
    Class_Pawn_get_IsFiring = gm("GameBase", "Pawn", "get_IsFiring", 0);
    Class_Pawn_IsAiming = gm("GameBase", "Pawn", "IsAiming", 0);
    Class_Pawn_set_AimRotation = gm("GameBase", "Pawn", "set_AimRotation", 1);
    Class_Pawn_get_AimRotation = gm("GameBase", "Pawn", "get_AimRotation", 0);
    Class_Pawn_get_LastPawnPos = gm("GameBase", "Pawn", "get_LastPawnPos", 0);
    Class_Pawn_get_HeadPosition = gm("GameBase", "Pawn", "get_HeadPosition", 0);
    Class_Pawn_get_PlayerName = gm("GameBase", "Pawn", "get_PlayerName", 0);
    Class_Pawn_get_TeamSeatId = gm("GameBase", "Pawn", "get_TeamSeatId", 0);
    Pawn_get_CurrentWeapon = gm("GameBase", "Pawn", "get_CurrentWeapon", 0);
    Inventory_get_CategoryName = gm("GameEngine", "Inventory", "get_CategoryName", 0);

    Class_Pawn_m_IsBot = gf("GameBase", "Pawn", "m_IsBot");
    Class_Pawn_m_IsAlive = gf("GameBase", "Pawn", "m_IsAlive");
    Class_Pawn_m_Mesh = gf("GameBase", "Pawn", "m_Mesh");
    Class_Pawn_m_HeadBone = gf("GameBase", "Pawn", "m_HeadBone");
    Class_Pawn_m_LastFireWeaponName = gf("GameBase", "Pawn", "m_LastFireWeaponName");
    Class_Pawn_LastCanSeeState = gf("GameBase", "Pawn", "LastCanSeeState");
    Class_Pawn_m_PlayerInfo = gf("GameBase", "Pawn", "m_PlayerInfo");
    Class_AttackableTarget__m_AttackableInfo = gf("GameEngine", "AttackableTarget", "m_AttackableInfo");
    Class_AttackableTarget__m_Health = gf("GameEngine", "AttackableTargetInfo", "m_Health");
    Class_AttackableTarget__m_MaxHealth = gf("GameEngine", "AttackableTargetInfo", "m_MaxHealth");
    Class_AttackableTarget_PlayerState = gf("GameEngine", "PlayerInfo", "m_PlayerState");
    Class_PlayerInfo_m_VestValue = gf("GameEngine", "PlayerInfo", "m_VestValue");
    Class_PlayerInfo_m_VestMaxValue = gf("GameEngine", "PlayerInfo", "m_VestMaxValue");
    Class_BaseGame__m_AllVehicles = gf("GameBase", "BaseGame", "m_AllVehicles");
    Class_BaseGame_EnemyPawns = gf("GameBase", "BaseGame", "EnemyPawns");

    Class_Pawn_m_BoneMappings = FastOffsets::BoneMap_m_BoneMappings;
    Class_Pawn_m_BoneTable = FastOffsets::BoneMap_m_BoneTable;
    Class_Pawn_m_BoneNode = FastOffsets::BoneMap_m_BoneNode;

    FastFuncs::Camera_get_main_Fast = (Camera* (*)())Class_Camera_get_main;
    FastFuncs::WorldToScreen_Fast = (Vector3 (*)(Camera*, Vector3))Class_Camera_WorldToScreenPoint;
    FastFuncs::Component_get_transform_Fast = (Transform* (*)(Component*))Class_Component_get_transform;
    FastFuncs::Transform_get_position_Fast = (Vector3 (*)(Transform*))Class_Transform_get_position;
    FastFuncs::Transform_get_forward_Fast = (Vector3 (*)(Transform*))Class_Transform_get_forward;
    FastFuncs::Gameplay_get_MatchGame_Fast = (MatchGame* (*)())Class_Gameplay_get_MatchGame;
    FastFuncs::Gameplay_get_LocalPawn_Fast = (Pawn* (*)())Class_Gameplay_get_LocalPawn;
    FastFuncs::Pawn_get_IsFiring_Fast = (bool (*)(Pawn*))Class_Pawn_get_IsFiring;
    FastFuncs::Pawn_IsAiming_Fast = (bool (*)(Pawn*))Class_Pawn_IsAiming;
    FastFuncs::Pawn_set_AimRotation_Fast = (void (*)(Pawn*, Quaternion))Class_Pawn_set_AimRotation;
    FastFuncs::Pawn_get_AimRotation_Fast = (Quaternion (*)(Pawn*))Class_Pawn_get_AimRotation;
    FastFuncs::Pawn_get_LastPawnPos_Fast = (Vector3 (*)(Pawn*))Class_Pawn_get_LastPawnPos;
    FastFuncs::Pawn_get_HeadPosition_Fast = (Vector3 (*)(Pawn*))Class_Pawn_get_HeadPosition;
    FastFuncs::Pawn_get_PlayerName_Fast = (String* (*)(Pawn*))Class_Pawn_get_PlayerName;
    FastFuncs::Pawn_get_TeamSeatId_Fast = (unsigned int* (*)(Pawn*))Class_Pawn_get_TeamSeatId;
    FastFuncs::Input_GetTouch_Fast = (void* (*)(void*, int))Input_GetTouch;
    FastFuncs::Input_get_touchSupported_Fast = (bool (*)(void*))Input_get_touchSupported;
    FastFuncs::Input_get_touchCount_Fast = (int (*)())Input_get_touchCount;
    FastFuncs::Input_get_mousePosition_Fast = (Vector3 (*)(void*))Input_get_mousePosition;
    FastFuncs::Screen_get_width_Fast = (int (*)())Screen_get_width;
    FastFuncs::Screen_get_height_Fast = (int (*)())Screen_get_height;
    FastFuncs::Screen_get_dpi_Fast = (float (*)())Screen_get_dpi;
    FastFuncs::Pawn_get_CurrentWeapon_Fast = (void* (*)(Pawn*))Pawn_get_CurrentWeapon;
    FastFuncs::Inventory_get_CategoryName_Fast = (int (*)(void*, const void*))Inventory_get_CategoryName;
    FastFuncs::GetPlayerState_Fast = [](AttackableTargetInfo* info) -> int {
        if (!info || !Class_AttackableTarget_PlayerState) return 0;
        return ReadFieldFast<int>(info, Class_AttackableTarget_PlayerState);
    };

    started = true;
    initialized = true;
}
inline void* GetPawnCurrentWeapon_Fast(Pawn* pawn) {
    return FastFuncs::Pawn_get_CurrentWeapon_Fast ? FastFuncs::Pawn_get_CurrentWeapon_Fast(pawn) : nullptr;
}


inline Vector3 WorldToScreen_Fast(Camera* cam, Vector3 pos) {
    return FastFuncs::WorldToScreen_Fast ? FastFuncs::WorldToScreen_Fast(cam, pos) : Vector3{};
}

inline Transform* ComponentGetTransform_Fast(Component* comp) {
    return FastFuncs::Component_get_transform_Fast ? FastFuncs::Component_get_transform_Fast(comp) : nullptr;
}

inline Vector3 TransformGetPosition_Fast(Transform* trans) {
    return FastFuncs::Transform_get_position_Fast ? FastFuncs::Transform_get_position_Fast(trans) : Vector3{};
}

inline Pawn* GetLocalPawn_Fast() {
    return FastFuncs::Gameplay_get_LocalPawn_Fast ? FastFuncs::Gameplay_get_LocalPawn_Fast() : nullptr;
}

inline MatchGame* GetMatchGame_Fast() {
    return FastFuncs::Gameplay_get_MatchGame_Fast ? FastFuncs::Gameplay_get_MatchGame_Fast() : nullptr;
}

inline bool IsPawnFiring_Fast(Pawn* pawn) {
    return FastFuncs::Pawn_get_IsFiring_Fast ? FastFuncs::Pawn_get_IsFiring_Fast(pawn) : false;
}

inline void SetPawnAimRotation_Fast(Pawn* pawn, Quaternion rotation) {
    if (FastFuncs::Pawn_set_AimRotation_Fast) {
        FastFuncs::Pawn_set_AimRotation_Fast(pawn, rotation);
    }
}

inline Vector3 GetPawnHeadPosition_Fast(Pawn* pawn) {
    return FastFuncs::Pawn_get_HeadPosition_Fast ? FastFuncs::Pawn_get_HeadPosition_Fast(pawn) : Vector3{};
}

inline Quaternion GetPawnAimRotation_Fast(Pawn* pawn) {
    return FastFuncs::Pawn_get_AimRotation_Fast ? FastFuncs::Pawn_get_AimRotation_Fast(pawn) : Quaternion{};
}

#define INIT_OFFSETS() espoffset()
#define USE_FAST_FUNCS 1