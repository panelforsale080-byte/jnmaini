 #pragma once
#include <cstdint>
#include <cmath>
#include "Hacks/Feature/Init_Hacks.h"

// ------------------- Basic Typedefs -------------------
typedef uintptr_t QWORDK;

// ------------------- Safe Memory Read -------------------
template <typename T>
T ReadD(QWORDK address) {
    return *reinterpret_cast<T*>(address);
}


// ------------------- Global Offsets -------------------
uintptr_t Class_Pawn_get_AimRotation; 


//=========={ METHODS }============
uintptr_t Class_Camera_get_main;
uintptr_t Class_Camera_WorldToScreenPoint;
uintptr_t Class_Component_get_transform;
uintptr_t Class_Transform_get_position;
uintptr_t Class_Gameplay_get_MatchGame;
uintptr_t Class_Gameplay_get_LocalPawn;
uintptr_t Class_Pawn_get_IsFiring;
uintptr_t Class_Pawn_IsAiming;
uintptr_t Class_Pawn_set_AimRotation;
uintptr_t Class_Pawn_get_LastPawnPos;
uintptr_t Class_Pawn_get_HeadPosition;
uintptr_t Class_Pawn_get_PlayerName;
uintptr_t Class_Pawn_get_TeamSeatId;
uintptr_t Class_Transform_get_forward;
uintptr_t Class_AttackableTarget_PlayerState;
//=========={ ImGui Rqd }============
uintptr_t Input_GetTouch;
uintptr_t Input_get_touchSupported;
uintptr_t Input_get_touchCount;
uintptr_t Input_get_mousePosition;
uintptr_t Screen_get_width;
uintptr_t Screen_get_height;
uintptr_t Screen_get_dpi;

//=========={ FIELDS }================
uintptr_t Class_AttackableTarget__m_AttackableInfo;
uintptr_t Class_AttackableTarget__m_Health;
uintptr_t Class_AttackableTarget__m_MaxHealth;
uintptr_t Class_BaseGame__m_AllVehicles;
uintptr_t Class_Pawn_m_IsBot;
uintptr_t Class_Pawn_m_IsAlive;
uintptr_t Class_Pawn_m_Mesh;
uintptr_t Class_Pawn_m_HeadBone;
uintptr_t Class_BaseGame_EnemyPawns;
uintptr_t Class_Pawn_m_LastFireWeaponName;
uintptr_t Class_Pawn_LastCanSeeState;
uintptr_t Class_Pawn_m_PlayerInfo;
uintptr_t Class_PlayerInfo_m_VestValue;
uintptr_t Class_PlayerInfo_m_VestMaxValue;

uintptr_t Class_Pawn_m_BoneMappings;
uintptr_t Class_Pawn_m_BoneTable;
uintptr_t Class_Pawn_m_BoneNode;


uintptr_t Pawn_get_CurrentWeapon;
uintptr_t Inventory_get_CategoryName;
// ------------------- Forward Declarations -------------------

class Transform;
class BaseGame;
class Pawn;
class MatchGame;
class Camera;




// ------------------- Component -------------------
class Component {
public:
    Transform* get_transform() {
        auto fn = (Transform* (*)(Component*)) (Class_Component_get_transform);
        return fn(this);
    }
};


// ------------------- Transform -------------------
class Transform : public Component {
public:
    Vector3 get_position() {
        auto fn = (Vector3 (*)(Transform*)) (Class_Transform_get_position);
        return fn(this);
    }

    Vector3 get_forward() { 
        auto fn = (Vector3 (*)(Transform*)) (Class_Transform_get_forward);
        return fn(this);
    }

    
};

// ------------------- Camera -------------------
class Camera : public Component {
public:
    static Camera* get_main() {
        auto fn = (Camera* (*)()) (Class_Camera_get_main);
        return fn();
    }

    Vector3 WorldToScreenPoint(Vector3 position) {
        auto cam = Camera::get_main();
        if (!cam) return {0, 0, 0};

        auto fn = (Vector3 (*)(Camera*, Vector3)) (Class_Camera_WorldToScreenPoint);
        return fn(cam, position);
    }
};

// ------------------- AttackableTarget -------------------
class AttackableTargetInfo {
public:
    float m_Health() {
        return *(float*)((uintptr_t)this + Class_AttackableTarget__m_Health);
    }

    float m_MaxHealth() {
        return *(float*)((uintptr_t)this + Class_AttackableTarget__m_MaxHealth);
    }
    
    int GetPlayerState() {
        return *(int*)((uintptr_t)this + Class_AttackableTarget_PlayerState);
    }
    
};

class AttackableTarget {
public:
    AttackableTargetInfo* m_AttackableInfo() {
        return *(AttackableTargetInfo**)((uintptr_t)this + Class_AttackableTarget__m_AttackableInfo);
    }
};







class GamePlay {
public:
    static Pawn* get_LocalPawn() {
        auto fn = (Pawn* (*)()) (Class_Gameplay_get_LocalPawn);
        return fn();
    }

    static MatchGame* get_MatchGame() {
        auto fn = (MatchGame* (*)()) (Class_Gameplay_get_MatchGame);
        return fn();
    }
};
// ------------------- Pawn -------------------
class Pawn : public AttackableTarget {
public:

    uintptr_t GetPlayerInfo() {
        return *(uintptr_t*)((uintptr_t)this + Class_Pawn_m_PlayerInfo);
    }
    
    
    bool IsKnocked() {
    AttackableTargetInfo* info = m_AttackableInfo();
    if (!info) return false;
    return info->GetPlayerState() == KNOCKDOWN;
    }
    
    
    int GetArmorValue() {
        uintptr_t playerInfo = GetPlayerInfo();
        if (!playerInfo) return 0;
        return *(int*)(playerInfo + Class_PlayerInfo_m_VestValue);
    }
    
    int GetMaxArmor() {
        uintptr_t playerInfo = GetPlayerInfo();
        if (!playerInfo) return 0;
        return *(int*)(playerInfo + Class_PlayerInfo_m_VestMaxValue);
    }

    Vector3 get_LastPawnPos() {
        auto fn = (Vector3 (*)(Pawn*)) (Class_Pawn_get_LastPawnPos);
        return fn(this);
    }
   
    Vector3 get_HeadPosition() {
    auto fn = (Vector3 (*)(Pawn*)) (Class_Pawn_get_HeadPosition);
    return fn(this);
    }

    Transform* m_HeadBone() {
        return *(Transform**)((uintptr_t)this + Class_Pawn_m_HeadBone);
    }

    Transform* m_Mesh() {
        return *(Transform**)((uintptr_t)this + Class_Pawn_m_Mesh);
    }


    bool m_IsBot() {
        return *(bool*)((uintptr_t)this + Class_Pawn_m_IsBot);
    }

    bool m_IsAlive() {
        return *(bool*)((uintptr_t)this + Class_Pawn_m_IsAlive);
    }

    String* get_PlayerName() {
        auto _Pawn__get_PlayerName = (String* (*)(Pawn*)) (Class_Pawn_get_PlayerName);
        return _Pawn__get_PlayerName(this);
    }

    unsigned int* get_TeamSeatId() {
        auto _Pawn__get_TeamSeatId = (unsigned int* (*)(Pawn*)) (Class_Pawn_get_TeamSeatId);
        return _Pawn__get_TeamSeatId(this);
    }

    int m_LastFireWeaponName() {
        return *(int*)((uintptr_t)this + Class_Pawn_m_LastFireWeaponName);
    }



    float m_LastCanSeeState() {
        return *(float*)((uintptr_t)this + Class_Pawn_LastCanSeeState);
    }

    bool isVisible() {
        if (!this) return false;
        // DISABLE_VISCHECK handled in DrawESP (checkboxStates) — no dep here
        float vis = *(float*)((uintptr_t)this + Class_Pawn_LastCanSeeState);
        return (vis >= -0.01f && vis <= 0.01f);
    }

     bool get_IsFiring() {
        auto fn = (bool (*)(Pawn*)) (Class_Pawn_get_IsFiring);
        return fn(this);
    }
    
    bool IsAiming() {
        auto fn = (bool (*)(Pawn*)) (Class_Pawn_IsAiming);
        return fn(this);
    }

    void set_AimRotation(Quaternion rotation) {
        auto fn = (void (*)(Pawn*, Quaternion)) (Class_Pawn_set_AimRotation);
        fn(this, rotation);
    }

    
    // --------- AimRotation ----------
    Quaternion get_AimRotation() {
        auto _Pawn__get_AimRotation = (Quaternion (*)(Pawn*)) (Class_Pawn_get_AimRotation);
        return _Pawn__get_AimRotation(this);
    }
};



//helper
inline uintptr_t GetPlayerInfoFast(Pawn* pawn) {
    if (!pawn) return 0;
    return *(uintptr_t*)((uintptr_t)pawn + Class_Pawn_m_PlayerInfo);
}

inline int GetArmorValueFast(Pawn* pawn) {
    uintptr_t playerInfo = GetPlayerInfoFast(pawn);
    if (!playerInfo) return 0;
    return *(int*)(playerInfo + Class_PlayerInfo_m_VestValue);
}

inline int GetMaxArmorFast(Pawn* pawn) {
    uintptr_t playerInfo = GetPlayerInfoFast(pawn);
    if (!playerInfo) return 0;
    return *(int*)(playerInfo + Class_PlayerInfo_m_VestMaxValue);
}




/*
// Namespace: 
public enum PlayerState.PlayerStatus // TypeDefIndex: 3435
{
    // Fields
    public int value__; // 0x0
    public const int Normal = 1;
    public const int Dead = 2;
    public const int Offline = 3;
    public const int KnockDown = 4;
}
*/





class PhysicsVehicle : public Component {
public:
};

// ------------------- Game Base -------------------
class BaseGame {
public:
    List<Pawn*>* EnemyPawns() {
        return *(List<Pawn*>**)((uintptr_t)this + Class_BaseGame_EnemyPawns);
    }

    List<PhysicsVehicle*>* m_AllVehicles() {
        return *(List<PhysicsVehicle*>**)((uintptr_t)this + Class_BaseGame__m_AllVehicles);
    }

};

    



// ------------------- Match Game -------------------
class MatchGame : public BaseGame {
public:
   static MatchGame* get_MatchGame() {
        auto fn = (MatchGame* (*)()) (Class_Gameplay_get_MatchGame);
        return fn();
    }
    
    
};




uintptr_t m_Il2Cpp;
uintptr_t m_IL2CPP;
bool g_Initialized = false;
bool clearMousePos = true;
int glWidth, glHeight;
float density = -1;
void *m_EGL;

enum TouchPhase : int {
    Began = 0,
    Moved = 1,
    Stationary = 2,
    Ended = 3,
    Canceled = 4,
};

struct UnityEngine_Touch_Fields {
    int32_t m_FingerId;
    Vector2 m_Position;
    Vector2 m_RawPosition;
    Vector2 m_PositionDelta;
    float m_TimeDelta;
    int32_t m_TapCount;
    int32_t m_Phase;
    int32_t m_Type;
    float m_Pressure;
    float m_maximumPossiblePressure;
    float m_Radius;
    float m_RadiusVariance;
    float m_AltitudeAngle;
    float m_AzimuthAngle;
};

bool get_touchSupported(void *instance) {
    return reinterpret_cast<bool(__fastcall *)(void *)>(Input_get_touchSupported)(instance);
}

int get_touchCount() {
    return reinterpret_cast<int(__fastcall *)()>(Input_get_touchCount)();
}

Vector3 get_mousePosition(void *instance) {
    return reinterpret_cast<Vector3(__fastcall *)(void *)>(Input_get_mousePosition)(instance);
}

int get_width() {
    return reinterpret_cast<int(__fastcall*)()>(Screen_get_width)();
}

int get_height() {
    return reinterpret_cast<int(__fastcall*)()>(Screen_get_height)();
}

float get_density() {
    return reinterpret_cast<float(__fastcall*)()>(Screen_get_dpi)();
}
void ImGui_GetTouch(ImGuiIO* io, int screenHeight) {
    if (get_touchCount() > 0) {
       
        void *instance = nullptr; 
        
        if (get_touchSupported(instance)) {
            UnityEngine_Touch_Fields touch = ((UnityEngine_Touch_Fields (*)(void *, int))(Input_GetTouch))(instance, 0);
            
           
            ImVec2 touchPos = ImVec2(get_mousePosition(instance).x, screenHeight - get_mousePosition(instance).y);

            switch (touch.m_Phase) {
            case TouchPhase::Began:
            case TouchPhase::Stationary:
            case TouchPhase::Moved:
                io->MousePos = touchPos;
                io->MouseDown[0] = true;
                break;

            case TouchPhase::Ended:
            case TouchPhase::Canceled:
                io->MouseDown[0] = false;
               
                clearMousePos = true;
                break;

            default:
                break;
            }
        }
    } else {
      
        io->MouseDown[0] = false;
    }
}
