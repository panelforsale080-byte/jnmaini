#pragma once

#include <cstdint>
#include <string>
//#include "ImGui/telelog.h"

typedef unsigned long long ulong;

uintptr_t get_off(const char* str) {
    return std::stoul(str, nullptr, 16);
}
// Add DOS version
#define GET_OFF(offset) get_off(DOS(offset))

#define BYPASS(offset) do { \
    void *dummy; \
DOSHOOK((void*)(libanogs_base + GET_OFF(offset)), (void*)hook_bypass, &dummy); \
} while(0)

#define PATCH(offset) do { \
    uintptr_t target = libanogs_base + GET_OFF(offset); \
    size_t page = sysconf(_SC_PAGESIZE); \
    uintptr_t start = target & ~(page - 1); \
    mprotect((void*)start, page, PROT_READ | PROT_WRITE | PROT_EXEC); \
    *(uint32_t*)target = 0xD2800000; \
    *(uint32_t*)(target + 4) = 0xD65F03C0; \
    mprotect((void*)start, page, PROT_READ | PROT_EXEC); \
    __builtin___clear_cache((char*)start, (char*)(start + page)); \
} while(0)

struct CSLobby_Offline_Ntf {
    int reason;  // offset 0x18
    int type;    // offset 0x1C  
    const char* message; // offset 0x20
};



typedef uint64_t _BYTE;
typedef uint64_t _QWORD;

auto DOSHOOK = [](void *target, void *replace, void **backup) {
    if (!target) return;
    if (DobbyHook(target, replace, backup) == 0) {
        void *page = (void *)((uintptr_t)(*backup) & ~(getpagesize() - 1));
        mprotect(page, getpagesize(), PROT_READ | PROT_EXEC); // Hide trampoline
    }
};


static uintptr_t libanogs_base = 0;
static uintptr_t libunity_base = 0;

#include "Hacks/Feature/WeaponCategoryConfig.h"

// ================= OFFSET ENCRYPTION =================


//UNLOCK FPS MAX
typedef void (*SetUltraFrameRateDeviceInfo_t)(void *, bool, int, int, int, bool);
static SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

static void hooked_SetUltraFrameRateDeviceInfo(void *thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (!thiz) {
        orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
        return;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, true, 240, 165, 100, true);
}

//fastscope
static float (*orig_get_AimingTime)(void* instance);
static float hook_get_AimingTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        float originalTime = orig_get_AimingTime(instance);
        float minTime = 0.01f;
        float finalTime = originalTime * (1.0f - t) + (minTime * t);
        if (finalTime < 0.01f) finalTime = 0.01f;
        return finalTime;
    }
    return orig_get_AimingTime(instance);
}

static void (*orig_OpenAim)(void* instance);
static void hook_OpenAim(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            orig_OpenAim(instance);
            return;
        }
    }
    orig_OpenAim(instance);
}

static void (*orig_CloseAim)(void* instance);
static void hook_CloseAim(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            orig_CloseAim(instance);
            return;
        }
    }
    orig_CloseAim(instance);
}

static bool (*orig_get_IsAiming)(void* instance);
static bool hook_get_IsAiming(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::SCOPE"];
        float t = slider / 100.0f;
        if (t >= 0.99f) {
            return true;
        }
    }
    return orig_get_IsAiming(instance);
}



//AIM ASSIST
/// AIM ASSIST - Add these function pointer declarations
static float (*new_aim1)(void* instance, float dist, float toQuasiDist, bool isMPPVE, float aimAngle, bool gamepadInput);

static float newaim1(void* instance, float dist, float toQuasiDist, bool isMPPVE, float aimAngle, bool gamepadInput) {
    if (!new_aim1) return 0.0f;  // Add this line
    float assistStrength = seekbarValues["AIM::ASSIST"];
    if (instance && assistStrength > 0.0f) {
        return assistStrength;
    }
    return new_aim1(instance, dist, toQuasiDist, isMPPVE, aimAngle, gamepadInput);
}
static float (*new_aim2)(void* instance, float assistDis, float dis, float angle, float rotateTime, bool gamepadInput);
static float newaim2(void* instance, float assistDis, float dis, float angle, float rotateTime, bool gamepadInput) {
    if (!new_aim2) return 0.0f;  // Add this line
    float assistStrength = seekbarValues["AIM::ASSIST"];
    if (instance && assistStrength > 0.0f) {
        return assistStrength;
    }
    return new_aim2(instance, assistDis, dis, angle, rotateTime, gamepadInput);
}

//AUTO SKIP TUTORIAL
static bool (*orig_IsTutorialEnabled)(void *instance);
static bool hook_IsTutorialEnabled(void *instance)
{
    return false;
}


 
static bool (*orig_get_AdvanceUAVEnabled)(void *instance);
static bool hook_get_AdvanceUAVEnabled(void *instance) {
if (instance != NULL) {
if (checkboxStates["ADVANCE::UAV"]) {
return true;
}
}
return orig_get_AdvanceUAVEnabled(instance);
}


static float (*orig_GetScaleRecoil)(void* instance);
static float hook_GetScaleRecoil(void* instance) {
    if (!orig_GetScaleRecoil) return 0.0f;
    if (!instance) return orig_GetScaleRecoil(instance);
    
    float slider = seekbarValues["NO::RECOIL"];
    float t = slider / 100.0f;
    float originalRecoil = orig_GetScaleRecoil(instance);
    float minRecoil = 0.00001f;
    return originalRecoil * (1.0f - t) + (minRecoil * t);
}

static Vector2 (*orig_GetPunchAngle)(void* instance);
static Vector2 hook_GetPunchAngle(void* instance) {
    if (!orig_GetPunchAngle) return Vector2{0, 0};
    if (!instance) return orig_GetPunchAngle(instance);
    
    float slider = seekbarValues["NO::RECOIL"];
    float t = slider / 100.0f;
    Vector2 originalPunch = orig_GetPunchAngle(instance);
    float minPunch = 0.00001f;
    return Vector2{
        originalPunch.x * (1.0f - t) + (minPunch * t),
        originalPunch.y * (1.0f - t) + (minPunch * t)
    };
}


static float (*orig_get_ChangeClipTime)(void* instance);
static float hook_get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipTime(instance);
}

static float (*orig_get_ChangeClipStartTime)(void* instance);
static float hook_get_ChangeClipStartTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipStartTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipStartTime(instance);
}

static float (*orig_get_ChangeClipEndTime)(void* instance);
static float hook_get_ChangeClipEndTime(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::RELOAD"];
        if (slider >= 99.0f) return 0.01f;
        float t = slider / 100.0f;
        float originalTime = orig_get_ChangeClipEndTime(instance);
        return originalTime * (1.0f - t);
    }
    return orig_get_ChangeClipEndTime(instance);
}



static float (*orig_GetRealSpreadModifier)(void* instance);
static float hook_GetRealSpreadModifier(void* instance) {
    if (instance != NULL) {
        float slider = seekbarValues["NO::SPREAD"];
        if (slider >= 99.0f) return 0.0f;
        float t = slider / 100.0f;
        float originalModifier = orig_GetRealSpreadModifier(instance);
        return originalModifier * (1.0f - t);
    }
    return orig_GetRealSpreadModifier(instance);
}


static Vector2 (*orig_GetShotSpread)(void* instance);
static Vector2 hook_GetShotSpread(void* instance) {
    if (!instance) return orig_GetShotSpread(instance);
    float slider = seekbarValues["NO::SPREAD"];
    Vector2 original = orig_GetShotSpread(instance);
    float t = slider / 100.0f;
    return Vector2{original.x * (1.0f - t), original.y * (1.0f - t)};
}



	
static bool ( * orig_SingleLineCheckPhysics)(void * instance, int hitType, void * hitTarget, void * hitCollider, Vector3 startPos, Vector3 dir, void * impactInfo);
static bool hook_SingleLineCheckPhysics(void * instance, int hitType, void * hitTarget, void * hitCollider, Vector3 startPos, Vector3 dir, void * impactInfo) {
if (instance != NULL) {
    if (instance != nullptr && checkboxStates["HITBOX"]) {
return (true, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}
}
return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

static float (*orig_UpdateCameraShake)(void *instance);
static float hook_UpdateCameraShake(void *instance) {
if (instance != NULL) {
if (checkboxStates["NO::SHAKE"]) {
return 0;
}
}
return orig_UpdateCameraShake(instance);
}

// --- No Parachute ---
static void (*orig_OpenParachute)(void *instance, bool isAuto);
static void hook_OpenParachute(void *instance, bool isAuto) {
    if (instance != NULL) {
        if (checkboxStates["NO::PARACHUTE"]) {
            return; // do nothing
        }
    }
    return orig_OpenParachute(instance, isAuto);
}


static bool (*orig_get_EnableVulkan)(void *instance);
static bool hook_get_EnableVulkan(void *instance) {
    return false; 
}

static bool (*orig_ShouldCurDeviceEnableVK)();
static bool hook_ShouldCurDeviceEnableVK() {
    return false; 
}

static void (*orig_set_EnableVulkan)(void *instance, bool value);
static void hook_set_EnableVulkan(void *instance, bool value) {
    return orig_set_EnableVulkan(instance, false);
}
//unlock all graphics
static void (*orig_SetEnableVSync)(bool enabled);
static void hook_SetEnableVSync(bool enabled) {
    orig_SetEnableVSync(false);
}


    
// --- Swimming Speed Scale ---
static float (*orig_CalcSwimmingSpeedScale)(void *instance) = nullptr;
static float hook_CalcSwimmingSpeedScale(void *instance) {
    if (instance != nullptr && checkboxStates["FAST::SWIM"]) {
        return 3.0f; 
    }
    return orig_CalcSwimmingSpeedScale(instance);
}

//FOV BR
static float (*orig_GetMaxFov_3p)(void *instance) = nullptr;
static float hook_GetMaxFov_3p(void *instance) {
    if (instance != nullptr && seekbarValues["FOV"] > 0.0f) {
        return seekbarValues["FOV"]; // slider value only if user moved it
    }
    return orig_GetMaxFov_3p(instance); // otherwise original
}

//FOV MP
static float (*orig_GetMaxFov_1p)(void *instance) = nullptr;
static float hook_GetMaxFov_1p(void *instance) {
    if (instance != nullptr && seekbarValues["FOV1"] > 0.0f) {
        return seekbarValues["FOV1"]; // slider value only if user moved it
    }
    return orig_GetMaxFov_1p(instance); // otherwise original
}

// -------SNOWB
static float (*orig_get_m_PhysSkisMaxSpeed)(void* instance) = nullptr;
static float hook_get_m_PhysSkisMaxSpeed(void* instance) {
    if (instance != nullptr && seekbarValues["SizeSnow"] > 0.0f) {
        return seekbarValues["SizeSnow"];
    }
    return orig_get_m_PhysSkisMaxSpeed(instance);
}


int hook_bypass(){
return 0;
}


//FPSHIGH
static int (*orig_GetFrameRateValue)(void* instance, int fl, int mode) = nullptr;
static int hook_GetFrameRateValue(void* instance, int fl, int mode) {
    if (instance != nullptr) {
        if (checkboxStates["FRAME120"]) {
            return 120;
        }
        else if (checkboxStates["FRAME40"]) {
            return 40;
        }
    }
    return orig_GetFrameRateValue(instance, fl, mode);
}

static void (*o_OnReSpawnPlayer)(void *, Vector3) = nullptr;
static void h_OnReSpawnPlayer(void *instance, Vector3 serverSyncPos) {
    if (o_OnReSpawnPlayer) {
        o_OnReSpawnPlayer(instance, serverSyncPos);
    }
    if (instance && checkboxStates["WALLHACK"]) {
        ((void (*)(void *, bool))(libunity_base + GET_OFF("0x5565680")))(instance, true);
    }
}

static void (*orig_CheckCrouch)(void *instance);
static void hook_CheckCrouch(void *instance) {
    if (instance != NULL && checkboxStates["NO CROUCH"]) {
        return;  // Block crouch completely
    }
    orig_CheckCrouch(instance);
}

static void (*orig_Tick)(void* instance, float deltaTime);
static void hook_Tick(void* instance, float deltaTime) {
    if (instance != NULL) {
        float slider = seekbarValues["FAST::LOOT"];
        if (slider >= 99.0f) {
            float* finishCountdown = (float*)((uintptr_t)instance + 0x40);
            if (*finishCountdown > 0.0f) {
                *finishCountdown = 0.0f; // Force instant completion
            }
        }
    }
    orig_Tick(instance, deltaTime);
}




void (*orig_GameUIScene_OnKickOff)(void* instance, void* msg);
void hooked_GameUIScene_OnKickOff(void* instance, void* msg) {
    return;
}





// ================= Speedhack =================

static float (*orig_get_deltaTime)() = nullptr;
static float (*orig_get_fixedDeltaTime)() = nullptr;
float GetSpeedMultiplier() {
    float speed = seekbarValues["SPEED_HACK"];
    if (speed > 0.0f) {
        if (speed > 100.0f) speed = 100.0f;
        return 1.0f + (speed / 100.0f) * 10.0f;
    }
    return 1.0f;
}

float hooked_get_deltaTime() { 
    if (!orig_get_deltaTime) return 0.016f;
    return orig_get_deltaTime() * GetSpeedMultiplier(); 
}

float hooked_get_fixedDeltaTime() { 
    if (!orig_get_fixedDeltaTime) return 0.02f;
    return orig_get_fixedDeltaTime() * GetSpeedMultiplier(); 
}

//====================MISDIRECTION=====================
static bool g_IllusionForced = false;
static void (*orig_IllusionTick)(void*, float);
static void hook_IllusionTick(void* instance, float dt) {
    orig_IllusionTick(instance, dt);
    
    if (!instance || !checkboxStates["AUTO::MISDIRECTION"]) {
        g_IllusionForced = false;
        return;
    }
    
    *(bool*)((uintptr_t)instance + 0x41) = true;
    g_IllusionForced = true;
}

static bool (*orig_GetEnableVulkan)(void* instance);
static bool hook_GetEnableVulkan(void* instance) {
    return false;
}

static bool (*orig_GetUseVulkanVRS)(void* instance);
static bool hook_GetUseVulkanVRS(void* instance) {
    return false;
}



static void hooking() {
    static bool init = false; 
    if (init) return; 
    init = true;
   
    if (libanogs_base == 0) {
        libanogs_base = getAbsoluteAddress(DOS("libanogs.so"), 0); 
    }
    if (libunity_base == 0) {
        libunity_base = getAbsoluteAddress(DOS("libunity.so"), 0); 
    }

BYPASS("0x204218");  // crash fixer
BYPASS("0x45B334");  //ban data
BYPASS("0x44A714");  // CRC 
BYPASS("0x48F964");  // kill 7 callers

//FEATURES

DOSHOOK((void*)(libunity_base + GET_OFF("0x8058220")), (void*)hook_IllusionTick, (void**)&orig_IllusionTick);  // MISDIRECTION
//ANTIKICK //public class GameUIScene : UIScene
DOSHOOK((void*)(libunity_base + GET_OFF("0x73669C4")), (void*)hooked_GameUIScene_OnKickOff, (void**)&orig_GameUIScene_OnKickOff);
//ANTIKICK


//FAST LOOT
DOSHOOK((void*)(libunity_base + GET_OFF("0xA7EAB18")), (void*)hook_Tick, (void**)&orig_Tick);

// No Crouch
DOSHOOK((void*)(libunity_base + GET_OFF("0x66B882C")), (void*)hook_CheckCrouch, (void**)&orig_CheckCrouch);

//speedhack
DOSHOOK((void*)(libunity_base + GET_OFF("0x530EF10")), (void*)hooked_get_deltaTime, (void**)&orig_get_deltaTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0x534D8E4")), (void*)hooked_get_fixedDeltaTime, (void**)&orig_get_fixedDeltaTime);

// Wallhack
DOSHOOK((void*)(libunity_base + GET_OFF("0x53D2114")), (void*)&h_OnReSpawnPlayer, (void**)&o_OnReSpawnPlayer);

// FPS
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAC73C")), (void*)hook_GetFrameRateValue, (void**)&orig_GetFrameRateValue);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAB394")), (void*)&hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

// Snowboard Speed
DOSHOOK((void*)(libunity_base + GET_OFF("0x53FE7FC")), (void*)hook_get_m_PhysSkisMaxSpeed, (void**)&orig_get_m_PhysSkisMaxSpeed);

// FOV
DOSHOOK((void*)(libunity_base + GET_OFF("0xC0B472C")), (void*)hook_GetMaxFov_3p, (void**)&orig_GetMaxFov_3p);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC0B4458")), (void*)hook_GetMaxFov_1p, (void**)&orig_GetMaxFov_1p);

// Fast Swim
DOSHOOK((void*)(libunity_base + GET_OFF("0x5597B14")), (void*)hook_CalcSwimmingSpeedScale, (void**)&orig_CalcSwimmingSpeedScale);

// Graphics
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFAD9D4")), (void*)hook_SetEnableVSync, (void**)&orig_SetEnableVSync);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFB9F44")), (void*)hook_get_EnableVulkan, (void**)&orig_get_EnableVulkan);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFBA068")), (void*)hook_ShouldCurDeviceEnableVK, (void**)&orig_ShouldCurDeviceEnableVK);
DOSHOOK((void*)(libunity_base + GET_OFF("0xAFBA18C")), (void*)hook_set_EnableVulkan, (void**)&orig_set_EnableVulkan);


    DOSHOOK((void*)(libunity_base + 0xAFB9F44), (void*)hook_GetEnableVulkan, (void**)&orig_GetEnableVulkan);
    DOSHOOK((void*)(libunity_base + 0x59BE2E0), (void*)hook_GetUseVulkanVRS, (void**)&orig_GetUseVulkanVRS);


// No Parachute
DOSHOOK((void*)(libunity_base + GET_OFF("0x5A88C48")), (void*)hook_OpenParachute, (void**)&orig_OpenParachute);
/*
//Fast scope
DOSHOOK((void*)(libunity_base + GET_OFF("0x5474160")), (void*)hook_get_AimingTime, (void**)&orig_get_AimingTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54B8198")), (void*)hook_OpenAim, (void**)&orig_OpenAim);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54B900C")), (void*)hook_CloseAim, (void**)&orig_CloseAim);
DOSHOOK((void*)(libunity_base + GET_OFF("0x54F60A4")), (void*)hook_get_IsAiming, (void**)&orig_get_IsAiming);
*/
// Aim Assist
DOSHOOK((void*)(libunity_base + GET_OFF("0x54F0E64")), (void*)&newaim1, (void**)&new_aim1);
DOSHOOK((void*)(libunity_base + GET_OFF("0x66BD5E8")), (void*)&newaim2, (void**)&new_aim2);

// Skip Tutorial
DOSHOOK((void*)(libunity_base + GET_OFF("0xCCC3240")), (void*)&hook_IsTutorialEnabled, (void**)&orig_IsTutorialEnabled);

// Advance UAV
DOSHOOK((void*)(libunity_base + GET_OFF("0x6AE9A70")), (void*)&hook_get_AdvanceUAVEnabled, (void**)&orig_get_AdvanceUAVEnabled);


DOSHOOK((void*)(libunity_base + GET_OFF("0xD535A6C")), (void*)hook_GetScaleRecoil, (void**)&orig_GetScaleRecoil);
DOSHOOK((void*)(libunity_base + GET_OFF("0xD5359E4")), (void*)hook_GetPunchAngle, (void**)&orig_GetPunchAngle);

// No Reload
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5B58")), (void*)hook_get_ChangeClipTime, (void**)&orig_get_ChangeClipTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5D88")), (void*)hook_get_ChangeClipStartTime, (void**)&orig_get_ChangeClipStartTime);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2B5F90")), (void*)hook_get_ChangeClipEndTime, (void**)&orig_get_ChangeClipEndTime);

// No Spread
DOSHOOK((void*)(libunity_base + GET_OFF("0xD53408C")), (void*)hook_GetRealSpreadModifier, (void**)&orig_GetRealSpreadModifier);
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2BDF40")), (void*)hook_GetShotSpread, (void**)&orig_GetShotSpread);

// No Shake
DOSHOOK((void*)(libunity_base + GET_OFF("0x669780C")), (void*)&hook_UpdateCameraShake, (void**)&orig_UpdateCameraShake);

// Hitbox
DOSHOOK((void*)(libunity_base + GET_OFF("0xC2BE0C8")), (void*)&hook_SingleLineCheckPhysics, (void**)&orig_SingleLineCheckPhysics);

}