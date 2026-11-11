#pragma once
#include "Hacks/StructGame/GameClass.h"
#include <cstdint>
#include <string>
#include "Includes/DOSOBF.h"
#ifndef GET_OFF
#define GET_OFF(offset) get_off(offset)
#endif
bool started = false;
namespace FastOffsets {
#define ENC_OFFSET(offset) (get_off(DOS(offset)))
    
    static const uintptr_t offset1 = 0x5024010;      // Camera_get_main
    static const uintptr_t offset2 = 0x5023B54;      // Camera_WorldToScreenPoint
    static const uintptr_t offset3 = 0x50291B0;      // Component_get_transform
    static const uintptr_t offset4 = 0x507C19C;      // Transform_get_position
    static const uintptr_t offset7 = 0x8569B10;      // Gameplay_get_MatchGame
    static const uintptr_t offset8 = 0x856AA08;      // Gameplay_get_LocalPawn
    static const uintptr_t offset9 = ENC_OFFSET("0x51A8A38");      // Pawn_get_IsFiring
    static const uintptr_t offset10 = 0x519BB10;     // Pawn_IsAiming
    static const uintptr_t offset15 = 0x51B8D30;     // Pawn_get_LastPawnPos
    static const uintptr_t offset16 = 0x51B86EC;     // Pawn_get_HeadPosition
    static const uintptr_t offset17 = 0x51B8BDC;     // Pawn_get_PlayerName
    static const uintptr_t offset18 = 0x51B89F4;     // Pawn_get_TeamSeatId
    static const uintptr_t offset19 = 0x5057E18;     // Input_GetTouch
    static const uintptr_t offset20 = 0x5057F00;     // Input_get_touchSupported
    static const uintptr_t offset21 = 0x5057E08;     // Input_get_touchCount
    static const uintptr_t offset22 = 0x50577C0;     // Input_get_mousePosition
    static const uintptr_t offset23 = 0x50504BC;     // Screen_get_width
    static const uintptr_t offset24 = 0x5050520;     // Screen_get_height
    static const uintptr_t offset25 = 0x5087930;     // Screen_get_dpi
    static const uintptr_t offset46 = ENC_OFFSET("0x51B421C");     // Pawn_get_AimRotation
    static const uintptr_t offset12 = ENC_OFFSET("0x51B42E4");     // Pawn_set_AimRotation
    static const uintptr_t offset_forward = ENC_OFFSET("0x509C650");
    
    static const uintptr_t Class_Pawn_m_IsBot_Offset = 0x5B9;
    static const uintptr_t Class_Pawn_m_IsAlive_Offset = 0x548;
    static const uintptr_t Class_Pawn_m_Mesh_Offset = 0x628;
    static const uintptr_t Class_Pawn_m_HeadBone_Offset = 0x308;
    static const uintptr_t Class_AttackableTarget__m_AttackableInfo_Offset = ENC_OFFSET("0x78");
    static const uintptr_t Class_AttackableTarget__m_Health_Offset = 0x34;
    static const uintptr_t Class_AttackableTarget__m_MaxHealth_Offset = 0x38;
    static const uintptr_t Class_BaseGame__m_AllVehicles_Offset = 0x1A0;
    static const uintptr_t Class_BaseGame_EnemyPawns_Offset = 0x178;
    static const uintptr_t Class_Pawn_m_LastFireWeaponName_Offset = 0x570;
    static const uintptr_t Class_Pawn_LastCanSeeState_Offset = 0x980;
    static const uintptr_t Class_AttackableTarget_PlayerState_Offset = 0x404;
    static const uintptr_t Class_Pawn_m_PlayerInfo_Offset = 0x5C0;
    static const uintptr_t Class_PlayerInfo_m_VestValue_Offset = 0x5E8;
    static const uintptr_t Class_PlayerInfo_m_VestMaxValue_Offset = 0x5EC;
    // sksleton
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
}

template<typename T>
inline T ReadFieldFast(void* object, size_t offset) {
    return *reinterpret_cast<T*>((uintptr_t)object + offset);
}

inline void espoffset() {
    started = true;
    static bool initialized = false;
    if (initialized) return;

    static uintptr_t m_unity_cache = 0;
    if (m_unity_cache == 0) {
        m_unity_cache = getAbsoluteAddress("libunity.so", 0);  
    }

    const uintptr_t base = m_unity_cache;

    FastFuncs::GetPlayerState_Fast = [](AttackableTargetInfo* info) -> int {
        if (!info) return 0;
        return ReadFieldFast<int>(info, FastOffsets::Class_AttackableTarget_PlayerState_Offset);
    };

    FastFuncs::Camera_get_main_Fast = (Camera* (*)())(base + FastOffsets::offset1);
    FastFuncs::WorldToScreen_Fast = (Vector3 (*)(Camera*, Vector3))(base + FastOffsets::offset2);
    FastFuncs::Component_get_transform_Fast = (Transform* (*)(Component*))(base + FastOffsets::offset3);
    FastFuncs::Transform_get_position_Fast = (Vector3 (*)(Transform*))(base + FastOffsets::offset4);
    FastFuncs::Gameplay_get_MatchGame_Fast = (MatchGame* (*)())(base + FastOffsets::offset7);
    FastFuncs::Gameplay_get_LocalPawn_Fast = (Pawn* (*)())(base + FastOffsets::offset8);
    FastFuncs::Pawn_get_IsFiring_Fast = (bool (*)(Pawn*))(base + FastOffsets::offset9);
    FastFuncs::Pawn_IsAiming_Fast = (bool (*)(Pawn*))(base + FastOffsets::offset10);
    FastFuncs::Pawn_set_AimRotation_Fast = (void (*)(Pawn*, Quaternion))(base + FastOffsets::offset12);
    FastFuncs::Pawn_get_LastPawnPos_Fast = (Vector3 (*)(Pawn*))(base + FastOffsets::offset15);
    FastFuncs::Pawn_get_HeadPosition_Fast = (Vector3 (*)(Pawn*))(base + FastOffsets::offset16);
    FastFuncs::Pawn_get_PlayerName_Fast = (String* (*)(Pawn*))(base + FastOffsets::offset17);
    FastFuncs::Pawn_get_TeamSeatId_Fast = (unsigned int* (*)(Pawn*))(base + FastOffsets::offset18);
    FastFuncs::Input_GetTouch_Fast = (void* (*)(void*, int))(base + FastOffsets::offset19);
    FastFuncs::Input_get_touchSupported_Fast = (bool (*)(void*))(base + FastOffsets::offset20);
    FastFuncs::Input_get_touchCount_Fast = (int (*)())(base + FastOffsets::offset21);
    FastFuncs::Input_get_mousePosition_Fast = (Vector3 (*)(void*))(base + FastOffsets::offset22);
    FastFuncs::Screen_get_width_Fast = (int (*)())(base + FastOffsets::offset23);
    FastFuncs::Screen_get_height_Fast = (int (*)())(base + FastOffsets::offset24);
    FastFuncs::Screen_get_dpi_Fast = (float (*)())(base + FastOffsets::offset25);
    FastFuncs::Pawn_get_AimRotation_Fast = (Quaternion (*)(Pawn*))(base + FastOffsets::offset46);
    FastFuncs::Transform_get_forward_Fast = (Vector3 (*)(Transform*))(base + FastOffsets::offset_forward);

    Class_Pawn_m_BoneMappings = FastOffsets::BoneMap_m_BoneMappings;
    Class_Pawn_m_BoneTable = FastOffsets::BoneMap_m_BoneTable;
    Class_Pawn_m_BoneNode = FastOffsets::BoneMap_m_BoneNode;
    Class_AttackableTarget_PlayerState = FastOffsets::Class_AttackableTarget_PlayerState_Offset;
    Class_Camera_get_main = base + FastOffsets::offset1;
    Class_Camera_WorldToScreenPoint = base + FastOffsets::offset2;
    Class_Component_get_transform = base + FastOffsets::offset3;
    Class_Transform_get_position = base + FastOffsets::offset4;
    Class_Gameplay_get_MatchGame = base + FastOffsets::offset7;
    Class_Gameplay_get_LocalPawn = base + FastOffsets::offset8;
    Class_Pawn_get_IsFiring = base + FastOffsets::offset9;
    Class_Pawn_IsAiming = base + FastOffsets::offset10;
    Class_Pawn_set_AimRotation = base + FastOffsets::offset12;
    Class_Pawn_get_LastPawnPos = base + FastOffsets::offset15;
    Class_Pawn_get_HeadPosition = base + FastOffsets::offset16;
    Class_Pawn_get_PlayerName = base + FastOffsets::offset17;
    Class_Pawn_get_TeamSeatId = base + FastOffsets::offset18;
    Input_GetTouch = base + FastOffsets::offset19;
    Input_get_touchSupported = base + FastOffsets::offset20;
    Input_get_touchCount = base + FastOffsets::offset21;
    Input_get_mousePosition = base + FastOffsets::offset22;
    Screen_get_width = base + FastOffsets::offset23;
    Screen_get_height = base + FastOffsets::offset24;
    Screen_get_dpi = base + FastOffsets::offset25;
    Class_Pawn_get_AimRotation = base + FastOffsets::offset46;
    Class_Pawn_m_PlayerInfo = FastOffsets::Class_Pawn_m_PlayerInfo_Offset;
    Class_PlayerInfo_m_VestValue = FastOffsets::Class_PlayerInfo_m_VestValue_Offset;
    Class_PlayerInfo_m_VestMaxValue = FastOffsets::Class_PlayerInfo_m_VestMaxValue_Offset;
    Class_Transform_get_forward = base + FastOffsets::offset_forward;
    Class_Pawn_LastCanSeeState = FastOffsets::Class_Pawn_LastCanSeeState_Offset;
    Class_Pawn_m_IsBot = FastOffsets::Class_Pawn_m_IsBot_Offset;
    Class_Pawn_m_IsAlive = FastOffsets::Class_Pawn_m_IsAlive_Offset;
    Class_Pawn_m_Mesh = FastOffsets::Class_Pawn_m_Mesh_Offset;
    Class_Pawn_m_HeadBone = FastOffsets::Class_Pawn_m_HeadBone_Offset;
    Class_AttackableTarget__m_AttackableInfo = FastOffsets::Class_AttackableTarget__m_AttackableInfo_Offset;
    Class_AttackableTarget__m_Health = FastOffsets::Class_AttackableTarget__m_Health_Offset;
    Class_AttackableTarget__m_MaxHealth = FastOffsets::Class_AttackableTarget__m_MaxHealth_Offset;
    Class_BaseGame__m_AllVehicles = FastOffsets::Class_BaseGame__m_AllVehicles_Offset;
    Class_BaseGame_EnemyPawns = FastOffsets::Class_BaseGame_EnemyPawns_Offset;
    Class_Pawn_m_LastFireWeaponName = FastOffsets::Class_Pawn_m_LastFireWeaponName_Offset;

    initialized = true;
}

inline Camera* GetCameraMain_Fast() {
    return FastFuncs::Camera_get_main_Fast ? FastFuncs::Camera_get_main_Fast() : nullptr;
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