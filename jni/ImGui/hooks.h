#pragma once

#include <cstdint>
#include <string>
//#include "ImGui/telelog.h"

typedef unsigned long long ulong;

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


#include "Hacks/Feature/WeaponCategoryConfig.h"
#include "ANOGS/Il2Cpp.h"

static const char *kGameImages[] = {"Assembly-CSharp.dll", nullptr};
static const char *kUnityImages[] = {"UnityEngine.CoreModule.dll", "UnityEngine.dll", nullptr};

static void *find_method(const char *const *images, const char *ns, const char *cls, const char *name, int argc) {
    for (int i = 0; images && images[i]; ++i) {
        void *fn = Il2CppGetMethodOffset(images[i], ns, cls, name, argc);
        if (fn) return fn;
    }
    return nullptr;
}

static void hook_method(const char *const *images, const char *ns, const char *cls, const char *name, int argc, void *replace, void **backup) {
    void *fn = find_method(images, ns, cls, name, argc);
    if (fn) DOSHOOK(fn, replace, backup);
}

static size_t find_field(const char *ns, const char *cls, const char *name) {
    size_t off = Il2CppGetFieldOffset("Assembly-CSharp.dll", ns, cls, name);
    if (off == (size_t)-1 || off >= 0x4000) return (size_t)-1;
    return off;
}

static size_t s_finishCountdownOff = (size_t)-1;
static size_t s_skillActiveOff = (size_t)-1;
static void (*s_showFreeView)(void *, bool) = nullptr;

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
    if (instance && checkboxStates["WALLHACK"] && s_showFreeView) {
        s_showFreeView(instance, true);
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
        if (slider >= 99.0f && s_finishCountdownOff != (size_t)-1) {
            float* finishCountdown = (float*)((uintptr_t)instance + s_finishCountdownOff);
            if (*finishCountdown > 0.0f) {
                *finishCountdown = 0.0f;
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
    
    if (s_skillActiveOff != (size_t)-1)
        *(bool*)((uintptr_t)instance + s_skillActiveOff) = true;
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
    if (!Il2CppIsAssembliesLoaded()) return;
    init = true;

    s_finishCountdownOff = find_field("", "SimpleInteractionComponent", "_finishCountdown");
    s_skillActiveOff = find_field("GameEngine", "IllusionSkillPawnComponent", "bSkillActive");
    s_showFreeView = (void (*)(void *, bool))find_method(
        kGameImages, "GameBase", "PlayerPawn", "ShowFreeViewOcclustionEffect", 1);

    hook_method(kGameImages, "GameEngine", "IllusionSkillPawnComponent", "Tick", 1, (void *)hook_IllusionTick, (void **)&orig_IllusionTick);
    hook_method(kGameImages, "GameBase", "GameUIScene", "OnKickOff", 1, (void *)hooked_GameUIScene_OnKickOff, (void **)&orig_GameUIScene_OnKickOff);
    hook_method(kGameImages, "", "SimpleInteractionComponent", "Tick", 1, (void *)hook_Tick, (void **)&orig_Tick);
    hook_method(kGameImages, "GameEngine", "PlayerController", "CheckCrouch", 0, (void *)hook_CheckCrouch, (void **)&orig_CheckCrouch);
    hook_method(kUnityImages, "UnityEngine", "Time", "get_deltaTime", 0, (void *)hooked_get_deltaTime, (void **)&orig_get_deltaTime);
    hook_method(kUnityImages, "UnityEngine", "Time", "get_fixedDeltaTime", 0, (void *)hooked_get_fixedDeltaTime, (void **)&orig_get_fixedDeltaTime);
    hook_method(kGameImages, "GameBase", "Pawn", "OnReSpawnPlayer", 1, (void *)&h_OnReSpawnPlayer, (void **)&o_OnReSpawnPlayer);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "GetFrameRateValue", 2, (void *)hook_GetFrameRateValue, (void **)&orig_GetFrameRateValue);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "SetUltraFrameRateDeviceInfo", 5, (void *)&hooked_SetUltraFrameRateDeviceInfo, (void **)&orig_SetUltraFrameRateDeviceInfo);
    hook_method(kGameImages, "GameBase", "Pawn", "get_m_PhysSkisMaxSpeed", 0, (void *)hook_get_m_PhysSkisMaxSpeed, (void **)&orig_get_m_PhysSkisMaxSpeed);
    hook_method(kGameImages, "GameEngine", "InputSettingConfig", "GetMaxFov_3p", 0, (void *)hook_GetMaxFov_3p, (void **)&orig_GetMaxFov_3p);
    hook_method(kGameImages, "GameEngine", "InputSettingConfig", "GetMaxFov_1p", 0, (void *)hook_GetMaxFov_1p, (void **)&orig_GetMaxFov_1p);
    hook_method(kGameImages, "GameBase", "PlayerPawn", "CalcSwimmingSpeedScale", 0, (void *)hook_CalcSwimmingSpeedScale, (void **)&orig_CalcSwimmingSpeedScale);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "SetEnableVSync", 0, (void *)hook_SetEnableVSync, (void **)&orig_SetEnableVSync);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "ShouldCurDeviceEnableVK", 0, (void *)hook_ShouldCurDeviceEnableVK, (void **)&orig_ShouldCurDeviceEnableVK);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "set_EnableVulkan", 1, (void *)hook_set_EnableVulkan, (void **)&orig_set_EnableVulkan);
    hook_method(kGameImages, "GameEngine", "SystemSetting", "get_EnableVulkan", 0, (void *)hook_GetEnableVulkan, (void **)&orig_GetEnableVulkan);
    hook_method(kGameImages, "", "CODPostEffectChain", "get_UseVulkanVRS", 0, (void *)hook_GetUseVulkanVRS, (void **)&orig_GetUseVulkanVRS);
    hook_method(kGameImages, "OpenWorld", "SkydivingPhysics", "OpenParachute", 1, (void *)hook_OpenParachute, (void **)&orig_OpenParachute);
    hook_method(kGameImages, "GameBase", "Weapon", "GetAutoAssistAimRate", 5, (void *)&newaim1, (void **)&new_aim1);
    hook_method(kGameImages, "GameEngine", "PlayerController", "GetRotateSpeed", 5, (void *)&newaim2, (void **)&new_aim2);
    hook_method(kGameImages, "Tutorial", "TutorialManager", "IsTutorialEnabled", 0, (void *)&hook_IsTutorialEnabled, (void **)&orig_IsTutorialEnabled);
    hook_method(kGameImages, "GameEngine", "GameInfo", "get_AdvanceUAVEnabled", 0, (void *)&hook_get_AdvanceUAVEnabled, (void **)&orig_get_AdvanceUAVEnabled);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent_Instant", "GetScaleRecoil", 0, (void *)hook_GetScaleRecoil, (void **)&orig_GetScaleRecoil);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent_Instant", "GetPunchAngle", 0, (void *)hook_GetPunchAngle, (void **)&orig_GetPunchAngle);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent", "get_ChangeClipTime", 0, (void *)hook_get_ChangeClipTime, (void **)&orig_get_ChangeClipTime);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent", "get_ChangeClipStartTime", 0, (void *)hook_get_ChangeClipStartTime, (void **)&orig_get_ChangeClipStartTime);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent", "get_ChangeClipEndTime", 0, (void *)hook_get_ChangeClipEndTime, (void **)&orig_get_ChangeClipEndTime);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent_Instant", "GetRealSpreadModifier", 0, (void *)hook_GetRealSpreadModifier, (void **)&orig_GetRealSpreadModifier);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent", "GetShotSpread", 0, (void *)hook_GetShotSpread, (void **)&orig_GetShotSpread);
    hook_method(kGameImages, "GameEngine", "PlayerCamera", "UpdateCameraShake", 0, (void *)&hook_UpdateCameraShake, (void **)&orig_UpdateCameraShake);
    hook_method(kGameImages, "GameEngine", "WeaponFireComponent", "SingleLineCheckPhysics", 6, (void *)&hook_SingleLineCheckPhysics, (void **)&orig_SingleLineCheckPhysics);
}