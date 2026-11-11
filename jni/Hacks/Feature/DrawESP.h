#include <unordered_map> 
#include <cmath>       
#include "ImGui/skeleton.h"
#include <array>
#pragma once
std::unordered_map<std::string, int> dropdownStates;
std::atomic<bool> MatchGame{false};

void AddTextWithShadow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 textColor, const char* text) {
    ImU32 shadowColor = IM_COL32(0, 0, 0, 220);

    // Thick shadow (8 directions)
    draw->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x - 1, pos.y), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x, pos.y + 1), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x, pos.y - 1), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y + 1), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x - 1, pos.y - 1), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y - 1), shadowColor, text);
    draw->AddText(font, fontSize, ImVec2(pos.x - 1, pos.y + 1), shadowColor, text);

    // Main text
    draw->AddText(font, fontSize, pos, textColor, text);
}


// ---------------- GetClosestTarget ----------------
uintptr_t GetClosestTarget() {
    uintptr_t result = 0;
    float MAX_AIM_DISTANCE = seekbarValues["MAXDIST"];  
    float MinDist = MAX_AIM_DISTANCE;

    auto MatchGame = GamePlay::get_MatchGame();
    if (!MatchGame) return 0;

    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;

    Vector3 MyPos = localPawn->get_LastPawnPos();

    List<Pawn*>* EnemyPawns = MatchGame->EnemyPawns();
    if (!EnemyPawns) return 0;

    Pawn** Items = EnemyPawns->getItems();
    if (!Items) return 0;

    for (int i = 0; i < EnemyPawns->getSize(); i++) {
        Pawn* pPawn = Items[i];
        if (!pPawn) continue;
        if (!pPawn->m_IsAlive()) continue;

        Transform* mesh = pPawn->m_Mesh();
        if (!mesh) continue;

        Vector3 RootPos = mesh->get_position();
        float Distance = Vector3::Distance(MyPos, RootPos);

        if (Distance < MinDist && Distance <= MAX_AIM_DISTANCE) {
            result = (uintptr_t)pPawn;
            MinDist = Distance;
        }
    }

    return result;
}






Vector3 GetTargetWorldPosition(Pawn* targetPawn, int targetMode, const Vector3& camPos = Vector3{0,0,0}, const Vector3& aimDir = Vector3{0,0,1}) {
    if (!targetPawn) return Vector3{0, 0, 0};

    Transform* headBone = targetPawn->m_HeadBone();
    if (!headBone) return Vector3{0, 0, 0};

    Vector3 position = headBone->get_position();
    
    if (targetMode == 0) {
        auto normalizeDir = [&](Vector3 pos) -> Vector3 {
            Vector3 d = pos - camPos;
            float mag = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
            if (mag > 0.0001f) {
                d.x /= mag;
                d.y /= mag;
                d.z /= mag;
            }
            return d;
        };

        Vector3 dirHead = normalizeDir(position);
        float bestDot = dirHead.x * aimDir.x + dirHead.y * aimDir.y + dirHead.z * aimDir.z;
        Vector3 bestPos = position;

        const float offsets[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f};
        for (float off : offsets) {
            Vector3 pos = position;
            pos.y -= off;
            Vector3 dir = normalizeDir(pos);
            float dot = dir.x * aimDir.x + dir.y * aimDir.y + dir.z * aimDir.z;
            if (dot > bestDot) {
                bestDot = dot;
                bestPos = pos;
            }
        }
        return bestPos;
    } else {
        static constexpr std::array<float, 5> HEIGHT_OFFSETS = {0.0f, 0.0f, 0.2f, 0.4f, 0.9f};
        if (targetMode >= 0 && targetMode < static_cast<int>(HEIGHT_OFFSETS.size())) {
            position.y -= HEIGHT_OFFSETS[targetMode];
        }
        return position;
    }
}


uintptr_t GetInsideFOVTarget() {
    uintptr_t result = 0;
    float maxPixelDist = FLT_MAX;
    
    float fovRadius = seekbarValues["FOV500"];
    if (fovRadius <= 0) return 0;
    
    float maxWorldDist = seekbarValues["MAXDIST"];
    if (maxWorldDist <= 0) maxWorldDist = FLT_MAX;

    auto matchGame = MatchGame::get_MatchGame();
    if (!matchGame) return 0;

    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;

    List<Pawn*>* enemyPawns = matchGame->EnemyPawns();
    if (!enemyPawns || enemyPawns->getSize() == 0) return 0;

    Pawn** items = enemyPawns->getItems();
    if (!items) return 0;

    Camera* mainCam = Camera::get_main();
    if (!mainCam) return 0;
    
    Transform* camTransform = mainCam->get_transform();
    if (!camTransform) return 0;
    
    Vector3 camPos = camTransform->get_position();
    Vector3 aimDir = camTransform->get_forward();
    Vector3 myPos = localPawn->get_LastPawnPos();
    
    Vector2 screenCenter((float)get_width() / 2, (float)get_height() / 2);
    int targetMode = dropdownStates["#TARG"];
    
    float maxWorldDistSquared = maxWorldDist * maxWorldDist;
    float fovRadiusSquared = fovRadius * fovRadius;
    bool skipVisibility = checkboxStates["DISABLE::VIS"];

    for (int i = 0; i < enemyPawns->getSize(); i++) {
        Pawn* enemyPawn = items[i];
        if (!enemyPawn) continue;
        if (!enemyPawn->m_IsAlive()) continue;
        
        Vector3 targetWorldPos = GetTargetWorldPosition(enemyPawn, targetMode, camPos, aimDir);
        if (targetWorldPos.x == 0 && targetWorldPos.y == 0 && targetWorldPos.z == 0) continue;
        
        Vector3 diff = targetWorldPos - myPos;
        float distSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSquared > maxWorldDistSquared) continue;
        
        if (!skipVisibility && !enemyPawn->isVisible()) continue;
        
        Vector3 targetScreen = mainCam->WorldToScreenPoint(targetWorldPos);
        if (targetScreen.z < 0.1f) continue;
        
        float dx = targetScreen.x - screenCenter.x;
        float dy = targetScreen.y - screenCenter.y;
        float pixelDistSquared = dx * dx + dy * dy;
        
        if (pixelDistSquared <= fovRadiusSquared && pixelDistSquared < maxPixelDist * maxPixelDist) {
            result = (uintptr_t)enemyPawn;
            maxPixelDist = sqrtf(pixelDistSquared);
        }
    }

    return result;
}

void drawLine(long color, float thickness, Vector3 start, Vector3 end) {
    ImGui::GetBackgroundDrawList()->PathLineTo(ImVec2(start.x, start.y));
    ImGui::GetBackgroundDrawList()->PathLineTo(ImVec2(end.x, end.y));
    ImGui::GetBackgroundDrawList()->PathStroke(color, 0, thickness);
}

struct CircleEffect {
    float timer = 0;
};

std::unordered_map<void*, CircleEffect> circleMap;

void Draw3DCircle(ImDrawList* draw, void* player_ptr, Vector3 pos, int screenHeight, float deltaTime) {
    auto& c = circleMap[player_ptr];
    
    c.timer += deltaTime;
    float t = fmodf(c.timer, 1.2f) / 1.2f;
    
    float alpha = (1.0f - fabsf(2.0f * t - 1.0f)) * 255.0f;
    float radius = 0.3f + t * 0.6f;
    
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;
    
    Vector3 base = pos;
    base.y -= 0.05f;
    
    float step = 2 * IM_PI / 16;
    for (float a = 0; a < 2 * IM_PI; a += step) {
        Vector3 p1 = {
            base.x + cosf(a) * radius,
            base.y,
            base.z + sinf(a) * radius
        };
        Vector3 p2 = {
            base.x + cosf(a + step) * radius,
            base.y,
            base.z + sinf(a + step) * radius
        };
        
        Vector3 sp1 = mainCamera->WorldToScreenPoint(p1);
        Vector3 sp2 = mainCamera->WorldToScreenPoint(p2);
        
        if (sp1.z > 0 && sp2.z > 0) {
            draw->AddLine(
                {sp1.x, (float)screenHeight - sp1.y},
                {sp2.x, (float)screenHeight - sp2.y},
                IM_COL32(255, 255, 255, (int)alpha),
                1.0f
            );
        }
    }
}


void Draw3dBox(ImDrawList* draw, uintptr_t player_ptr, Vector3 rootPos, int glHeight, int glWidth) {
    Pawn* pawn = reinterpret_cast<Pawn*>(player_ptr);
    if (!pawn || !pawn->m_IsAlive()) return;

    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

    float hw = 0.60f * 0.40f;
    float h = 1.65f;
    
    Vector3 localCorners[8] = {
        {-hw, 0, -hw}, {hw, 0, -hw}, {hw, h, -hw}, {-hw, h, -hw},
        {-hw, 0,  hw}, {hw, 0,  hw}, {hw, h,  hw}, {-hw, h,  hw}
    };

    Vector3 sc[8];
    for (int i = 0; i < 8; i++) {
        sc[i] = mainCamera->WorldToScreenPoint(rootPos + localCorners[i]);
        if (sc[i].z <= 0.0f) return;
        sc[i].y = glHeight - sc[i].y;
    }

    ImVec4 col = colorStates["DEFAULT"];
    ImU32 color = IM_COL32(col.x * 255, col.y * 255, col.z * 255, col.w * 255);
    float t = 1.0f;

    draw->AddLine({sc[0].x, sc[0].y}, {sc[1].x, sc[1].y}, color, t);
    draw->AddLine({sc[1].x, sc[1].y}, {sc[2].x, sc[2].y}, color, t);
    draw->AddLine({sc[2].x, sc[2].y}, {sc[3].x, sc[3].y}, color, t);
    draw->AddLine({sc[3].x, sc[3].y}, {sc[0].x, sc[0].y}, color, t);

    draw->AddLine({sc[4].x, sc[4].y}, {sc[5].x, sc[5].y}, color, t);
    draw->AddLine({sc[5].x, sc[5].y}, {sc[6].x, sc[6].y}, color, t);
    draw->AddLine({sc[6].x, sc[6].y}, {sc[7].x, sc[7].y}, color, t);
    draw->AddLine({sc[7].x, sc[7].y}, {sc[4].x, sc[4].y}, color, t);

    draw->AddLine({sc[0].x, sc[0].y}, {sc[4].x, sc[4].y}, color, t);
    draw->AddLine({sc[1].x, sc[1].y}, {sc[5].x, sc[5].y}, color, t);
    draw->AddLine({sc[2].x, sc[2].y}, {sc[6].x, sc[6].y}, color, t);
    draw->AddLine({sc[3].x, sc[3].y}, {sc[7].x, sc[7].y}, color, t);
}

void drawESP__Player(ImDrawList *draw) {
    if (!started) return;
    
    float deltaTime = ImGui::GetIO().DeltaTime;
    Camera* mainCamera = Camera::get_main();
    if (!mainCamera) return;

    if (dropdownStates["#DIST"] == EAim::Crosshair && !checkboxStates["HAND::CAM"]) {
        float fovRadius = seekbarValues["FOV500"] * 1.0f;
        draw->AddCircle(ImVec2(glWidth / 2, glHeight / 2), fovRadius, ImColor(255, 255, 255, 140), 128, 1.5f);
    }

    BaseGame *get_MatchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(get_MatchGame)) return;
    
    MatchGame = true;
    
    Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(get_LocalPawn)) return;
    
    Vector3 pLocalPawn_rootPos = get_LocalPawn->get_LastPawnPos();
           
    List<Pawn *> *EnemyPawns = get_MatchGame->EnemyPawns();
    if (!Tools::IsPtrValid(EnemyPawns) || EnemyPawns->getSize() == 0) return;
    
    Pawn **pawns = (Pawn **)EnemyPawns->getItems();
    if (!pawns) return;
    
    int totalEnemies = 0, totalBots = 0;
    
    for (int i = 0; i < EnemyPawns->getSize(); i++) {
        Pawn *pawn = pawns[i];
        if (!Tools::IsPtrValid(pawn)) continue;
        if (!pawn->m_IsAlive()) continue;

        Transform* headBone = pawn->m_HeadBone();
        Transform* mesh = pawn->m_Mesh();
        if (!headBone || !mesh) continue;

        Vector3 pEnemyPawn_headPos = pawn->get_HeadPosition();
        Vector3 pEnemyPawn_rootPos = pawn->get_LastPawnPos();
        
        Vector3 headPos_WS = mainCamera->WorldToScreenPoint(pEnemyPawn_headPos);
        Vector3 rootPos_WS = mainCamera->WorldToScreenPoint(pEnemyPawn_rootPos);
        
        if (headPos_WS.z <= 0.1f || rootPos_WS.z <= 0.1f) continue;

        float distanceToMe = Vector3::Distance(pLocalPawn_rootPos, pEnemyPawn_rootPos);
        
        if (pawn->m_IsBot()) totalBots++;
        else totalEnemies++;

        if (!checkboxStates["ESP ENABLE"]) continue;

        if (checkboxStates["ESP CIRCLE"]) {
            Draw3DCircle(draw, pawn, pEnemyPawn_rootPos, glHeight, deltaTime);
        }

        if (checkboxStates["3D::BOX"]) {
            Draw3dBox(draw, (uintptr_t)pawn, pEnemyPawn_rootPos, glHeight, glWidth);
        }

        if (checkboxStates["ESP ALERT 360"]) {
            Vector2 screen(glWidth, glHeight);
            Vector2 location(headPos_WS.x, glHeight - headPos_WS.y);
            
            int borders = 0;
            Vector2 topLeft(screen.x * 0.08f, screen.y * 0.08f);
            Vector2 bottomRight(screen.x * 0.92f, screen.y * 0.92f);
            
            if (location.y < topLeft.y) borders |= 1;
            if (location.x > bottomRight.x) borders |= 2;
            if (location.y > bottomRight.y) borders |= 4;
            if (location.x < topLeft.x) borders |= 8;
            
            if (borders) {
                float dotRadius = (glHeight / 1080.0f) * 50.0f;
                float margin = dotRadius + 10.0f;
                float dotX = std::max(margin, std::min(screen.x - margin, location.x));
                float dotY = std::max(margin, std::min(screen.y - margin, location.y));
                draw->AddCircleFilled({dotX, dotY}, dotRadius, IM_COL32(255, 255, 255, 255), 0);
            }
        }

        if (checkboxStates["ESP LINE"] && pawn->m_IsAlive()) {
            bool visible = pawn->isVisible();
            bool bot = pawn->m_IsBot();
            bool knocked = pawn->IsKnocked();
            
            ImVec4 col;
            if (knocked) col = colorStates["KNOCKED"];
            else if (bot) col = visible ? colorStates["BOT"] : colorStates["HIDDEN"];
            else col = visible ? colorStates["ENEMY"] : colorStates["HIDDEN"];
            
            ImU32 lineColor = IM_COL32(col.x * 255, col.y * 255, col.z * 255, col.w * 255);
            
            if (dropdownStates["#LINE_POS"] == 0) {
                Vector3 startPos(glWidth / 2, glHeight - 3);
                Vector3 endPos(rootPos_WS.x, glHeight - (rootPos_WS.y - 5.0f), 0.0f);
                drawLine(lineColor, 1.0f, startPos, endPos);
            } else {
                Vector3 startPos(glWidth / 2, 20);
                Vector3 endPos(headPos_WS.x, glHeight - headPos_WS.y - 20.0f, 0.0f);
                drawLine(lineColor, 1.0f, startPos, endPos);
            }
        }

        if (checkboxStates["SKELETON"]) {
            DrawSkeleton(draw, pawn);
        }

        if (checkboxStates["ESP BOX"]) {
            float boxHeight = abs(headPos_WS.y - rootPos_WS.y) * 1.1f;
            float boxWidth = boxHeight * 0.40f;
            
            if (boxHeight > glHeight * 2.0f || boxWidth > glWidth * 2.0f || boxHeight < 1.0f || boxWidth < 1.0f) continue;
            
            ImVec2 vStart = { headPos_WS.x - (boxWidth / 2), glHeight - headPos_WS.y - (boxHeight * 0.1f) };
            ImVec2 vEnd = { vStart.x + boxWidth, vStart.y + boxHeight };
            
            if (vStart.x > glWidth || vEnd.x < 0 || vStart.y > glHeight || vEnd.y < 0) continue;
            
            ImVec4 col = colorStates["DEFAULT"];
            ImU32 color = IM_COL32(col.x * 255, col.y * 255, col.z * 255, col.w * 255);
            
            if (dropdownStates["#BOX_STYLE"] == 1) {
                draw->AddRect(vStart, vEnd, color, 2.0f, 0, 1.0f);
            } else {
                float cw = boxWidth * 0.25f;
                float ch = boxHeight * 0.25f;
                draw->AddLine({vStart.x, vStart.y}, {vStart.x + cw, vStart.y}, color, 1.0f);
                draw->AddLine({vStart.x, vStart.y}, {vStart.x, vStart.y + ch}, color, 1.0f);
                draw->AddLine({vEnd.x, vStart.y}, {vEnd.x - cw, vStart.y}, color, 1.0f);
                draw->AddLine({vEnd.x, vStart.y}, {vEnd.x, vStart.y + ch}, color, 1.0f);
                draw->AddLine({vStart.x, vEnd.y}, {vStart.x + cw, vEnd.y}, color, 1.0f);
                draw->AddLine({vStart.x, vEnd.y}, {vStart.x, vEnd.y - ch}, color, 1.0f);
                draw->AddLine({vEnd.x, vEnd.y}, {vEnd.x - cw, vEnd.y}, color, 1.0f);
                draw->AddLine({vEnd.x, vEnd.y}, {vEnd.x, vEnd.y - ch}, color, 1.0f);
            }
        }

        if (checkboxStates["ESP NAME"] && pawn->m_IsAlive()) {
            auto playerNameObj = pawn->get_PlayerName();
            const char* playerName = (playerNameObj && playerNameObj->CString()) ? playerNameObj->CString() : "UNKNOWN";
            std::string strName = (pawn->m_IsBot() ? "[BOT] " : "[PLAYER] ") + std::string(playerName);
            
            float boxHeight = abs(headPos_WS.y - rootPos_WS.y) * 1.1f;
            float boxWidth = boxHeight * 0.65f;
            ImVec2 vStart = { headPos_WS.x - (boxWidth / 2), glHeight - headPos_WS.y - (boxHeight * 0.1f) };
            ImVec2 textSize = ImGui::CalcTextSize(strName.c_str());
            ImVec2 textPos(vStart.x + (boxWidth / 2.0f) - (textSize.x / 2.0f), vStart.y - textSize.y - 2.0f);
            AddTextWithShadow(draw, nullptr, 14.0f, textPos, IM_COL32(255, 255, 255, 255), strName.c_str());
        }

        if (checkboxStates["ESP PLAYER ID"] && pawn->m_IsAlive()) {
            std::string strID = "ID: " + std::to_string((int)pawn->get_TeamSeatId());
            ImVec2 textPos(headPos_WS.x - 64.0f, glHeight - headPos_WS.y - 24.0f);
            AddTextWithShadow(draw, nullptr, 15.0f, textPos, IM_COL32(255, 255, 255, 255), strID.c_str());
        }

        if (checkboxStates["ESP HEALTH"] && pawn->m_IsAlive()) {
            int get_Health = 0, max_Health = 0;
            AttackableTargetInfo* m_AttackableInfo = pawn->m_AttackableInfo();
            if (m_AttackableInfo) {
                get_Health = m_AttackableInfo->m_Health();
                max_Health = m_AttackableInfo->m_MaxHealth();
            }

            if (get_Health <= 0 || max_Health <= 0) {
                if (checkboxStates["ESP ARMOR"]) {
                    int currentArmor = pawn->GetArmorValue();
                    int maxArmor = pawn->GetMaxArmor();
                    if (maxArmor > 0) {
                        float percent = std::clamp((float)currentArmor / (float)maxArmor, 0.0f, 1.0f);
                        float boxHeight = abs(headPos_WS.y - rootPos_WS.y) * 1.1f;
                        float boxWidth = boxHeight * 0.65f;
                        if (boxHeight <= glHeight && boxWidth <= glWidth && boxHeight >= 1.0f && boxWidth >= 1.0f) {
                            ImVec2 vStart = { headPos_WS.x - (boxWidth / 2), glHeight - headPos_WS.y - (boxHeight * 0.1f) };
                            ImVec2 vEnd = { vStart.x + boxWidth, vStart.y + boxHeight };
                            if (!(vEnd.x < 0 || vStart.x > glWidth || vEnd.y < 0 || vStart.y > glHeight)) {
                                float barWidth = 4.0f;
                                float barX = vStart.x - (barWidth * 2) - 4.0f;
                                ImVec2 barTop = { barX, vStart.y };
                                ImVec2 barBottom = { barX + barWidth, vStart.y + boxHeight };
                                float fillY = std::clamp(vStart.y + boxHeight * (1.0f - percent), barTop.y, barBottom.y);
                                draw->AddRectFilled(barTop, barBottom, IM_COL32(40, 40, 40, 200));
                                draw->AddRectFilled({ barX, fillY }, barBottom, IM_COL32(0, 191, 255, 255));
                                draw->AddRect(barTop, barBottom, IM_COL32(0, 0, 0, 255));
                            }
                        }
                    }
                }
                continue;
            }

            float percent = std::clamp((float)get_Health / (float)max_Health, 0.0f, 1.0f);
            float boxHeight = abs(headPos_WS.y - rootPos_WS.y) * 1.1f;
            float boxWidth = boxHeight * 0.65f;
            
            if (boxHeight > glHeight || boxWidth > glWidth || boxHeight < 1.0f || boxWidth < 1.0f) continue;
            
            ImVec2 vStart = { headPos_WS.x - (boxWidth / 2), glHeight - headPos_WS.y - (boxHeight * 0.1f) };
            ImVec2 vEnd = { vStart.x + boxWidth, vStart.y + boxHeight };
            
            if (vEnd.x < 0 || vStart.x > glWidth || vEnd.y < 0 || vStart.y > glHeight) continue;
            
            float barWidth = 4.0f;
            float barX = vStart.x - barWidth - 2.0f;
            ImVec2 barTop = { barX, vStart.y };
            ImVec2 barBottom = { barX + barWidth, vStart.y + boxHeight };
            float fillY = std::clamp(vStart.y + boxHeight * (1.0f - percent), barTop.y, barBottom.y);
            
            ImU32 color = percent > 0.5f ? IM_COL32(0, 255, 0, 255) :
                          percent > 0.2f ? IM_COL32(255, 165, 0, 255) :
                          IM_COL32(255, 0, 0, 255);
            
            draw->AddRectFilled(barTop, barBottom, IM_COL32(40, 40, 40, 200));
            draw->AddRectFilled({ barX, fillY }, barBottom, color);
            draw->AddRect(barTop, barBottom, IM_COL32(0, 0, 0, 255));
        }

        if (checkboxStates["ESP DISTANCE"] && pawn->m_IsAlive()) {
            std::string strDistance = std::to_string((int)distanceToMe) + " FT";
            ImVec2 textPos(headPos_WS.x + 50.0f, glHeight - headPos_WS.y - 20.0f);
            AddTextWithShadow(draw, nullptr, 13.0f, textPos, IM_COL32(255, 255, 255, 255), strDistance.c_str());
        }
    }

    if (checkboxStates["ESP::VEHICLE"]) {
        Vector3 myPos = Vector3::zero();
        Pawn *get_LocalPawn = GamePlay::get_LocalPawn();
        if (get_LocalPawn != nullptr) myPos = get_LocalPawn->get_LastPawnPos();
        
        BaseGame *get_MatchGame = GamePlay::get_MatchGame();
        if (get_MatchGame != nullptr) {
            List<PhysicsVehicle *> *m_AllVehicles = get_MatchGame->m_AllVehicles();
            if (m_AllVehicles != nullptr) {
                PhysicsVehicle **physicsVehicles = m_AllVehicles->getItems();
                if (physicsVehicles) {
                    for (int v = 0; v < m_AllVehicles->getSize(); v++) {
                        PhysicsVehicle *physicsVehicle = physicsVehicles[v];
                        if (!physicsVehicle || !physicsVehicle->get_transform()) continue;
                        Vector3 rootPos = physicsVehicle->get_transform()->get_position();
                        Vector3 screenPos = mainCamera->WorldToScreenPoint(rootPos);
                        if (screenPos.z <= 0) continue;
                        float distanceToMe = Vector3::Distance(myPos, rootPos);
                        std::string strVehicle = std::to_string((int)distanceToMe) + "M";
                        ImVec2 textSize = ImGui::CalcTextSize(strVehicle.c_str(), 0, 18.0f);
                        ImVec2 pos(screenPos.x - (textSize.x / 2), glHeight - screenPos.y - textSize.y - 5.0f);
                        draw->AddText(nullptr, 18.0f, pos, ImColor(255, 255, 255, 255), strVehicle.c_str());
                    }
                }
            }
        }
    }

    if (!checkboxStates["HAND::CAM"] && checkboxStates["ENEMY::COUNTER"]) {
        std::string label = "[Player]" + std::to_string(totalEnemies) + "   [AI]" + std::to_string(totalBots);
        float fontSize = ImGui::GetFontSize() + 8.0f;
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        float scale = fontSize / ImGui::GetFontSize();
        textSize.x *= scale;
        textSize.y *= scale;
        float lineStartY = 80.0f;
        float textPosX = (glWidth - textSize.x) / 2.0f;
        float textPosY = lineStartY - textSize.y - 10.0f;
        ImU32 textColor = IM_COL32(255, 255, 255, 255);
        ImDrawList* overlay = ImGui::GetForegroundDrawList();
        AddTextWithShadow(overlay, nullptr, fontSize, ImVec2(textPosX, textPosY), textColor, label.c_str());
    }
}
void* Aimbot_Thread() {
    if (!checkboxStates["AIMBOT 360"]) return nullptr;
    auto matchGame = MatchGame::get_MatchGame();
    if (!matchGame) return nullptr;
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return nullptr;
    Camera* main = Camera::get_main();
    if (!main) return nullptr;
    Transform* camTransform = main->get_transform();
    if (!camTransform) return nullptr;
    Vector3 camPos = camTransform->get_position();
    Vector3 aimDir = camTransform->get_forward();
    int triggerMode = dropdownStates["#TRIG"];
    bool bTriggerReady = (triggerMode == 0) || (triggerMode == 1 && localPawn->get_IsFiring()) || (triggerMode == 2 && localPawn->IsAiming()) || (triggerMode == 3 && (localPawn->get_IsFiring() || localPawn->IsAiming()));
    if (!bTriggerReady) return nullptr;
    uintptr_t Target = (dropdownStates["#DIST"] == 0) ? GetClosestTarget() : GetInsideFOVTarget();
    Pawn* targetPawn = reinterpret_cast<Pawn*>(Target);
    bool isValidTarget = Target && targetPawn && targetPawn->m_IsAlive() && (checkboxStates["DISABLE::VIS"] || targetPawn->isVisible()) && (!checkboxStates["IGNORE KNOCKED"] || !targetPawn->IsKnocked());
    if (!isValidTarget) return nullptr;
    int targetMode = dropdownStates["#TARG"];
    Vector3 targetPos = GetTargetWorldPosition(targetPawn, targetMode, camPos, aimDir);
    if (targetPos.x == 0 && targetPos.y == 0 && targetPos.z == 0) return nullptr;
    Quaternion currentRot = localPawn->get_AimRotation();
    Vector3 lookDir = targetPos - camPos;
    Quaternion targetRot = Quaternion::LookRotation(lookDir, Vector3::Up());
    float smoothness = std::clamp(seekbarValues["AIM SMOOTHNESS"], 1.0f, 100.0f);
    float aimSpeed = std::clamp(seekbarValues["AIM SPEED"], 1.0f, 100.0f);
    float smoothFactor = 1.0f - expf(-((0.25f + (aimSpeed / 100.0f) * 2.5f) * ImGui::GetIO().DeltaTime * 60.0f));
    smoothFactor = smoothFactor / (1.0f + (smoothness / 100.0f) * 3.0f);
    smoothFactor = std::min(smoothFactor, 0.005f + (aimSpeed / 100.0f) * 0.035f);
    localPawn->set_AimRotation(Quaternion::Slerp(currentRot, targetRot, smoothFactor));
    return nullptr;
}
void DrawESP(ImDrawList *draw) {
    drawESP__Player(draw);
    Aimbot_Thread();
}
