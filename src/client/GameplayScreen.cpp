#include "client/GameplayScreen.hpp"

#include "client/GameClient.hpp"
#include "client/ScreenManager.hpp"
#include "common/Constants.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace {
const char* WeaponName(KillFeedWeapon weapon) {
    switch (weapon) {
    case KillFeedWeapon::Ak47: return "AK47";
    case KillFeedWeapon::Pistol: return "M1911";
    case KillFeedWeapon::Sniper: return "KAR98k";
    case KillFeedWeapon::Shotgun: return "Mossberg";
    case KillFeedWeapon::Grenade: return "Grenade";
    case KillFeedWeapon::Disconnected: return "Disconnected";
    case KillFeedWeapon::Reconnected: return "Reconnected";
    case KillFeedWeapon::Kicked: return "Kicked";
    case KillFeedWeapon::LeftRoom: return "Left Room";
    }
    return "Weapon";
}

constexpr int HudSize = 20;
constexpr int SmallSize = 16;
constexpr float Pi = 3.14159265F;
constexpr Color FloorColor{57, 93, 66, 255};
constexpr Color LineColor{80, 121, 88, 255};
constexpr Color ObstacleColor{88, 72, 58, 255};
constexpr Color BarrierColor{90, 145, 180, 255};
constexpr Color GrenadeColor{46, 210, 118, 255};
constexpr Color GrenadeFuseColor{255, 220, 80, 255};
constexpr Color BulletColor{255, 226, 110, 255};
constexpr Color RedColor{222, 76, 68, 255};
constexpr Color BlueColor{72, 145, 231, 255};
constexpr Color WhiteColor{235, 238, 242, 255};
constexpr Color MutedColor{165, 174, 182, 255};
constexpr float GridSize = 80.0F;
constexpr float StartButtonWidth = 170.0F;
constexpr float StartButtonHeight = 42.0F;
constexpr float DefaultCameraZoom = 0.50F;
constexpr float SniperMapZoom = 0.45F;
constexpr float RespawnModalWidth = 640.0F;
constexpr float RespawnModalHeight = 360.0F;
constexpr float RespawnClassWidth = 136.0F;
constexpr float RespawnClassHeight = 52.0F;
constexpr float EndModalWidth = 720.0F;
constexpr float EndModalHeight = 500.0F;
constexpr float EndButtonWidth = 160.0F;
constexpr float EndButtonHeight = 44.0F;
constexpr int CountdownSize = 132;
constexpr int CountdownLabelSize = 30;
constexpr int EndTitleSize = 34;
constexpr int EndTextSize = 20;
constexpr int EndSmallSize = 18;

struct ObstacleView {
    Rectangle rect;
};

std::array<ObstacleView, 8> Obstacles(std::uint8_t mapIndex, int& count) {
    if (mapIndex == 0) {
        count = 7;
        return {ObstacleView{Rectangle{1035.0F, 485.0F, 130.0F, 430.0F}}, ObstacleView{Rectangle{380.0F, 292.0F, 360.0F, 76.0F}},
            ObstacleView{Rectangle{1460.0F, 1032.0F, 360.0F, 76.0F}}, ObstacleView{Rectangle{400.0F, 1032.0F, 320.0F, 76.0F}},
            ObstacleView{Rectangle{1480.0F, 292.0F, 320.0F, 76.0F}}, ObstacleView{Rectangle{805.0F, 580.0F, 90.0F, 240.0F}},
            ObstacleView{Rectangle{1305.0F, 580.0F, 90.0F, 240.0F}}, ObstacleView{}};
    }
    if (mapIndex == 1) {
        count = 7;
        return {ObstacleView{Rectangle{820.0F, 655.0F, 560.0F, 90.0F}}, ObstacleView{Rectangle{525.0F, 430.0F, 90.0F, 540.0F}},
            ObstacleView{Rectangle{1585.0F, 430.0F, 90.0F, 540.0F}}, ObstacleView{Rectangle{785.0F, 270.0F, 90.0F, 260.0F}},
            ObstacleView{Rectangle{1325.0F, 870.0F, 90.0F, 260.0F}}, ObstacleView{Rectangle{235.0F, 322.0F, 250.0F, 76.0F}},
            ObstacleView{Rectangle{1715.0F, 1002.0F, 250.0F, 76.0F}}, ObstacleView{}};
    }
    count = 8;
    return {ObstacleView{Rectangle{550.0F, 445.0F, 420.0F, 70.0F}}, ObstacleView{Rectangle{1230.0F, 885.0F, 420.0F, 70.0F}},
        ObstacleView{Rectangle{1040.0F, 640.0F, 120.0F, 120.0F}}, ObstacleView{Rectangle{265.0F, 1005.0F, 290.0F, 70.0F}},
        ObstacleView{Rectangle{1645.0F, 325.0F, 290.0F, 70.0F}}, ObstacleView{Rectangle{725.0F, 770.0F, 70.0F, 300.0F}},
        ObstacleView{Rectangle{1405.0F, 330.0F, 70.0F, 300.0F}}, ObstacleView{Rectangle{920.0F, 1045.0F, 360.0F, 70.0F}}};
}

Color TeamColor(TeamId team) {
    return team == TeamId::Blue ? BlueColor : RedColor;
}

const char* ModeText(GameMode mode) {
    switch (mode) {
    case GameMode::FfaTimed: return "FFA Timed";
    case GameMode::FfaScore: return "FFA Score";
    case GameMode::TdmTimed: return "TDM Timed";
    case GameMode::TdmScore: return "TDM Score";
    }
    return "Mode";
}

const char* MapName(std::uint8_t mapIndex) {
    switch (mapIndex) {
    case 0: return "Map 1";
    case 1: return "Map 2";
    case 2: return "Map 3";
    default: return "Map";
    }
}

bool IsTeamMode(GameMode mode) {
    return mode == GameMode::TdmTimed || mode == GameMode::TdmScore;
}

const char* TeamText(TeamId team) {
    switch (team) {
    case TeamId::Red: return "Red";
    case TeamId::Blue: return "Blue";
    case TeamId::None: return "None";
    }
    return "None";
}

std::string PlayerDisplayName(const PlayerSnapshot& player) {
    if (player.name[0] != '\0') {
        const std::size_t len = std::strlen(player.name.data());
        if (len > 0) {
            return std::string(player.name.data(), std::min(len, static_cast<std::size_t>(cfg::NameBytes - 1)));
        }
    }
    return "Player " + std::to_string(player.id);
}

std::int16_t BestKills(const SnapshotPacket& snapshot) {
    std::int16_t best = 0;
    for (std::size_t index = 0; index < snapshot.playerCount; ++index) {
        best = std::max(best, snapshot.players[index].kills);
    }
    return best;
}

std::string WinnerText(const SnapshotPacket& snapshot) {
    if (snapshot.matchEndedByForfeit) {
        if (IsTeamMode(snapshot.mode)) {
            if (snapshot.winnerTeam == TeamId::Red) { return "Red Team Wins (Forfeit)"; }
            if (snapshot.winnerTeam == TeamId::Blue) { return "Blue Team Wins (Forfeit)"; }
        } else if (snapshot.winnerPlayerId != 0) {
            for (std::size_t i = 0; i < snapshot.playerCount; ++i) {
                if (snapshot.players[i].id == snapshot.winnerPlayerId) {
                    return PlayerDisplayName(snapshot.players[i]) + " Wins (Opponents Left)";
                }
            }
            return "Player " + std::to_string(snapshot.winnerPlayerId) + " Wins (Opponents Left)";
        }
    }
    if (IsTeamMode(snapshot.mode)) {
        if (snapshot.winnerTeam == TeamId::Red || snapshot.redScore > snapshot.blueScore) { return "Red Team Wins"; }
        if (snapshot.winnerTeam == TeamId::Blue || snapshot.blueScore > snapshot.redScore) { return "Blue Team Wins"; }
        return "Draw";
    }
    if (snapshot.winnerPlayerId != 0) {
        for (std::size_t i = 0; i < snapshot.playerCount; ++i) {
            if (snapshot.players[i].id == snapshot.winnerPlayerId) {
                return PlayerDisplayName(snapshot.players[i]) + " Wins";
            }
        }
        return "Player " + std::to_string(snapshot.winnerPlayerId) + " Wins";
    }
    const std::int16_t best = BestKills(snapshot);
    int winners = 0;
    std::string winnerName;
    for (std::size_t index = 0; index < snapshot.playerCount; ++index) {
        if (snapshot.players[index].kills == best) {
            ++winners;
            winnerName = PlayerDisplayName(snapshot.players[index]);
        }
    }
    return winners == 1 ? (winnerName + " Wins") : "Draw";
}

float UiScale() { return static_cast<float>(GetScreenWidth()) / 1280.0f; }
float Sf(float x) { return x * UiScale(); }
int Si(float x) { return static_cast<int>(x * UiScale()); }
}

GameplayScreen::GameplayScreen()
    : sequence(0),
      sendAccumulator(0.0F),
      previousHealth{},
      previousAmmo{},
      localFireCooldown(0.0F),
      localReloadRemaining(0.0F),
      localPredictedAmmo(0),
      lastServerAmmo(0),
      localPredictedGrenades(0),
      lastServerGrenades(0),
      localPredictedBarriers(0),
      lastServerBarriers(0),
      grenadeQueued(false),
      barrierQueued(false),
      respawnQueued(false),
      dashQueued(false),
      previousMatchRunning(false),
      endModalOpen(false),
      grenadeTarget{},
      endSnapshot{},
      aimProgress(0.0F),
      killfeedCollapsed(false),
      killFeedItems{},
      lastKillEventId(0),
      escapeMenuOpen(false),
      confirmLeaveOpen(false),
      pendingKickPlayerId(0),
      fireBlockTimer(0.5F),
      wasAlive(false),
      localRespawnTimer(0.0F),
      playerInterp{},
      localInterpPos{0.0F, 0.0F},
      mapTexture{},
      cachedMapIndex(-1) {}

GameplayScreen::~GameplayScreen() {
    if (mapTexture.id != 0) {
        UnloadRenderTexture(mapTexture);
        mapTexture = RenderTexture2D{};
    }
}

void GameplayScreen::Reset() {
    sequence = 0;
    sendAccumulator = 0.0F;
    previousHealth.fill(cfg::PlayerMaxHealth);
    previousAmmo.fill(0);
    localFireCooldown = 0.0F;
    localReloadRemaining = 0.0F;
    localPredictedAmmo = 0;
    lastServerAmmo = 0;
    localPredictedGrenades = 0;
    lastServerGrenades = 0;
    localPredictedBarriers = 0;
    lastServerBarriers = 0;
    grenadeQueued = false;
    barrierQueued = false;
    respawnQueued = false;
    dashQueued = false;
    previousMatchRunning = false;
    endModalOpen = false;
    grenadeTarget = Vector2{0.0F, 0.0F};
    endSnapshot = SnapshotPacket{};
    aimProgress = 0.0F;
    escapeMenuOpen = false;
    confirmLeaveOpen = false;
    pendingKickPlayerId = 0;
    fireBlockTimer = 0.5F;
    wasAlive = false;
    localRespawnTimer = 0.0F;
    killFeedItems.clear();
    lastKillEventId = 0;
    playerInterp.fill(PlayerInterp{});
    localInterpPos = Vector2{0.0F, 0.0F};
    if (mapTexture.id != 0) {
        UnloadRenderTexture(mapTexture);
        mapTexture = RenderTexture2D{};
        cachedMapIndex = -1;
    }
}

void GameplayScreen::UpdateInterpolation(const SnapshotPacket& snapshot, float dt, bool newSnapshot) {
    if (newSnapshot) {
        for (std::size_t index = 0; index < snapshot.playerCount && index < cfg::MaxPlayers; ++index) {
            const PlayerSnapshot& player = snapshot.players[index];
            PlayerInterp* interp = nullptr;
            for (auto& entry : playerInterp) {
                if (entry.active && entry.id == player.id) {
                    interp = &entry;
                    break;
                }
            }
            if (interp == nullptr) {
                for (auto& entry : playerInterp) {
                    if (!entry.active) {
                        interp = &entry;
                        break;
                    }
                }
            }
            if (interp == nullptr) {
                interp = &playerInterp[index];
            }

            const float targetRot = std::atan2(player.aimY, player.aimX) * 180.0F / Pi - 90.0F;
            if (!interp->active || interp->id != player.id) {
                interp->id = player.id;
                interp->currentPos = Vector2{player.x, player.y};
                interp->targetPos = Vector2{player.x, player.y};
                interp->currentRotation = targetRot;
                interp->targetRotation = targetRot;
                interp->active = true;
            } else {
                const float dx = player.x - interp->currentPos.x;
                const float dy = player.y - interp->currentPos.y;
                if ((dx * dx + dy * dy) > 160.0F * 160.0F) {
                    interp->currentPos = Vector2{player.x, player.y};
                }
                interp->targetPos = Vector2{player.x, player.y};
                interp->targetRotation = targetRot;
            }
        }

        for (auto& entry : playerInterp) {
            if (!entry.active) {
                continue;
            }
            bool found = false;
            for (std::size_t index = 0; index < snapshot.playerCount && index < cfg::MaxPlayers; ++index) {
                if (snapshot.players[index].id == entry.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                entry.active = false;
                entry.id = 0;
            }
        }
    }

    const float posBlend = 1.0F - std::exp(-26.0F * dt);
    const float rotBlend = 1.0F - std::exp(-28.0F * dt);
    for (auto& entry : playerInterp) {
        if (!entry.active) {
            continue;
        }
        entry.currentPos.x += (entry.targetPos.x - entry.currentPos.x) * posBlend;
        entry.currentPos.y += (entry.targetPos.y - entry.currentPos.y) * posBlend;

        float rotDiff = entry.targetRotation - entry.currentRotation;
        while (rotDiff > 180.0F) { rotDiff -= 360.0F; }
        while (rotDiff < -180.0F) { rotDiff += 360.0F; }
        entry.currentRotation += rotDiff * rotBlend;
    }

    const std::uint32_t localId = snapshot.localPlayerId;
    bool localFound = false;
    for (const auto& entry : playerInterp) {
        if (entry.active && entry.id == localId) {
            localInterpPos = entry.currentPos;
            localFound = true;
            break;
        }
    }
    if (!localFound) {
        for (std::size_t index = 0; index < snapshot.playerCount; ++index) {
            if (snapshot.players[index].id == localId) {
                localInterpPos = Vector2{snapshot.players[index].x, snapshot.players[index].y};
                break;
            }
        }
    }
}

void GameplayScreen::Update(GameClient& client) {
    const float dt = GetFrameTime();
    const SnapshotPacket& snapshot = client.Snapshot();
    const PlayerSnapshot local = client.LocalPlayer();
    UpdateInterpolation(snapshot, dt, client.HasNewSnapshot());
    UpdateKillFeed(snapshot, dt);

    if (fireBlockTimer > 0.0F) {
        fireBlockTimer = std::max(0.0F, fireBlockTimer - dt);
    }
    if (!wasAlive && local.alive && snapshot.matchRunning) {
        fireBlockTimer = 0.5F;
    }
    if (snapshot.matchRunning) {
        if (!local.alive && wasAlive) {
            localRespawnTimer = cfg::RespawnSeconds;
        } else if (!local.alive) {
            localRespawnTimer = std::max(0.0F, localRespawnTimer - dt);
        } else {
            localRespawnTimer = 0.0F;
        }
    } else {
        localRespawnTimer = 0.0F;
    }
    wasAlive = local.alive;

    if (previousMatchRunning && !snapshot.matchRunning && snapshot.playerCount > 0) {
        endSnapshot = snapshot;
        endModalOpen = true;
    }
    if (snapshot.matchRunning) {
        endModalOpen = false;
    }
    const bool justStarted = !previousMatchRunning && snapshot.matchRunning;
    if (justStarted) {
        endModalOpen = false;
        endSnapshot = SnapshotPacket{};
        for (std::size_t index = 0; index < snapshot.playerCount && index < cfg::MaxPlayers; ++index) {
            previousAmmo[index] = snapshot.players[index].ammo;
        }
        localPredictedGrenades = local.grenades;
        lastServerGrenades = local.grenades;
        localPredictedBarriers = local.barriers;
        lastServerBarriers = local.barriers;
    }
    previousMatchRunning = snapshot.matchRunning;
    if (endModalOpen) {
        UpdateEndModal();
        sendAccumulator += dt;
        if (sendAccumulator >= cfg::ClientSendSeconds) {
            InputPacket packet{};
            packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
            packet.sequence = ++sequence;
            client.Network().SendInput(packet);
            sendAccumulator = 0.0F;
        }
        return;
    }

    // Confirmation dialogs have priority
    if (confirmLeaveOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            confirmLeaveOpen = false;
            return;
        }
        const Rectangle modal{
            (static_cast<float>(GetScreenWidth()) - Sf(440.0F)) * 0.5F,
            (static_cast<float>(GetScreenHeight()) - Sf(210.0F)) * 0.5F,
            Sf(440.0F),
            Sf(210.0F)
        };
        const Rectangle leaveBtn{modal.x + Sf(35.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
        const Rectangle cancelBtn{modal.x + Sf(245.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(GetMousePosition(), leaveBtn)) {
                confirmLeaveOpen = false;
                escapeMenuOpen = false;
                client.LeaveRoom();
                return;
            }
            if (CheckCollisionPointRec(GetMousePosition(), cancelBtn)) {
                confirmLeaveOpen = false;
                fireBlockTimer = 0.5F;
                return;
            }
        }
        sendAccumulator += dt;
        if (sendAccumulator >= cfg::ClientSendSeconds) {
            InputPacket packet{};
            packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
            packet.sequence = ++sequence;
            client.Network().SendInput(packet);
            sendAccumulator = 0.0F;
        }
        return;
    }

    if (pendingKickPlayerId != 0) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            pendingKickPlayerId = 0;
            fireBlockTimer = 0.5F;
            return;
        }
        const Rectangle modal{
            (static_cast<float>(GetScreenWidth()) - Sf(440.0F)) * 0.5F,
            (static_cast<float>(GetScreenHeight()) - Sf(210.0F)) * 0.5F,
            Sf(440.0F),
            Sf(210.0F)
        };
        const Rectangle kickBtn{modal.x + Sf(35.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
        const Rectangle cancelBtn{modal.x + Sf(245.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(GetMousePosition(), kickBtn)) {
                client.Network().SendKick(pendingKickPlayerId);
                pendingKickPlayerId = 0;
                fireBlockTimer = 0.5F;
                return;
            }
            if (CheckCollisionPointRec(GetMousePosition(), cancelBtn)) {
                pendingKickPlayerId = 0;
                fireBlockTimer = 0.5F;
                return;
            }
        }
        sendAccumulator += dt;
        if (sendAccumulator >= cfg::ClientSendSeconds) {
            InputPacket packet{};
            packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
            packet.sequence = ++sequence;
            client.Network().SendInput(packet);
            sendAccumulator = 0.0F;
        }
        return;
    }

    // Escape key toggles Escape Menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        escapeMenuOpen = !escapeMenuOpen;
        fireBlockTimer = 0.5F;
        return;
    }

    if (escapeMenuOpen) {
        UpdateEscapeMenu(client);
        sendAccumulator += dt;
        if (sendAccumulator >= cfg::ClientSendSeconds) {
            InputPacket packet{};
            packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
            packet.sequence = ++sequence;
            client.Network().SendInput(packet);
            sendAccumulator = 0.0F;
        }
        return;
    }

    if (localFireCooldown > 0.0F) {
        localFireCooldown = std::max(0.0F, localFireCooldown - dt);
    }
    if (localReloadRemaining > 0.0F) {
        localReloadRemaining = std::max(0.0F, localReloadRemaining - dt);
    }
    if (!local.alive) {
        localFireCooldown = 0.0F;
        localReloadRemaining = 0.0F;
        localPredictedGrenades = 0;
        lastServerGrenades = 0;
        localPredictedBarriers = 0;
        lastServerBarriers = 0;
    }
    if (local.ammo != lastServerAmmo) {
        localPredictedAmmo = local.ammo;
        lastServerAmmo = local.ammo;
        if (local.ammo == ClassMaxAmmo(local.playerClass)) {
            localReloadRemaining = 0.0F;
        } else if (local.ammo == 0 && localReloadRemaining <= 0.0F && local.alive) {
            localReloadRemaining = ClassReloadSeconds(local.playerClass);
            localFireCooldown = ClassReloadSeconds(local.playerClass);
        }
    }
    if (snapshot.matchRunning) {
        if (local.grenades != lastServerGrenades) {
            localPredictedGrenades = local.grenades;
            lastServerGrenades = local.grenades;
        }
        if (local.barriers != lastServerBarriers) {
            localPredictedBarriers = local.barriers;
            lastServerBarriers = local.barriers;
        }
    } else {
        localPredictedGrenades = 0;
        lastServerGrenades = 0;
        localPredictedBarriers = 0;
        lastServerBarriers = 0;
    }

    const Camera2D camera = MakeCamera(local, localInterpPos);
    const Vector2 aimWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    const Vector2 aimDelta{aimWorld.x - localInterpPos.x, aimWorld.y - localInterpPos.y};
    const float aimLength = std::max(1.0F, std::sqrt((aimDelta.x * aimDelta.x) + (aimDelta.y * aimDelta.y)));
    const bool countdownActive = snapshot.matchRunning && snapshot.startCountdown > 0.0F;

    const Rectangle chevronBounds = KillFeedChevronBounds();
    const bool chevronHover = CheckCollisionPointRec(GetMousePosition(), chevronBounds);
    if (chevronHover) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }
    if ((chevronHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_K)) {
        killfeedCollapsed = !killfeedCollapsed;
        fireBlockTimer = 0.5F;
    }

    const Rectangle startButton{static_cast<float>(GetScreenWidth()) - Sf(StartButtonWidth) - Sf(20.0F), Sf(12.0F), Sf(StartButtonWidth), Sf(StartButtonHeight)};
    const bool startHover = !snapshot.matchRunning && CheckCollisionPointRec(GetMousePosition(), startButton);
    const bool startClicked = startHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (!snapshot.matchRunning && (IsKeyPressed(KEY_ENTER) || (client.IsHost() && startClicked))) {
        fireBlockTimer = 0.5F;
        client.Network().SendStart();
    }

    std::uint16_t flags = 0;
    if (countdownActive) {
        grenadeQueued = false;
        barrierQueued = false;
        respawnQueued = false;
        dashQueued = false;
    } else if (snapshot.matchRunning && !local.alive && local.id != 0) {
        UpdateRespawnModal(client, local);
        if (respawnQueued) { flags |= InputFlags::Respawn; }
    } else {
        const bool canShoot = snapshot.matchRunning &&
                              local.alive &&
                              local.id != 0 &&
                              !countdownActive &&
                              !chevronHover &&
                              !startHover &&
                              !escapeMenuOpen &&
                              !confirmLeaveOpen &&
                              pendingKickPlayerId == 0 &&
                              !endModalOpen &&
                              fireBlockTimer <= 0.0F;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { flags |= InputFlags::MoveUp; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { flags |= InputFlags::MoveDown; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) { flags |= InputFlags::MoveLeft; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { flags |= InputFlags::MoveRight; }
        if (canShoot && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { flags |= InputFlags::Fire; }
        if (IsKeyDown(KEY_R)) { flags |= InputFlags::Reload; }
        if (IsKeyPressed(KEY_R)) {
            if (local.ammo < ClassMaxAmmo(local.playerClass) && localReloadRemaining <= 0.0F) {
                localReloadRemaining = ClassReloadSeconds(local.playerClass);
                localFireCooldown = ClassReloadSeconds(local.playerClass);
            }
        }
        if (canShoot && (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_Q))) {
            bool hasActiveGrenade = grenadeQueued;
            for (std::size_t i = 0; i < snapshot.grenadeCount; ++i) {
                if (snapshot.grenades[i].ownerId == local.id) {
                    hasActiveGrenade = true;
                    break;
                }
            }
            if (!hasActiveGrenade && localPredictedGrenades > 0) {
                --localPredictedGrenades;
                grenadeQueued = true;
                grenadeTarget = aimWorld;
                client.Audio().PlayGrenade();
            }
        }
        if (canShoot && (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_E))) {
            if (localPredictedBarriers > 0) {
                const Vector2 aimNorm = Vector2{aimDelta.x / aimLength, aimDelta.y / aimLength};
                const Vector2 targetPos = Vector2{local.x + aimNorm.x * cfg::BarrierPlaceDistance, local.y + aimNorm.y * cfg::BarrierPlaceDistance};
                bool tooClose = false;
                for (std::size_t i = 0; i < snapshot.barrierCount; ++i) {
                    const float dX = targetPos.x - snapshot.barriers[i].x;
                    const float dY = targetPos.y - snapshot.barriers[i].y;
                    if (std::sqrt(dX * dX + dY * dY) < 45.0F) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) {
                    --localPredictedBarriers;
                    barrierQueued = true;
                    client.Audio().PlayBarrier();
                }
            }
        }
        if (canShoot && IsKeyPressed(KEY_SPACE)) {
            dashQueued = true;
        }
        if (grenadeQueued) { flags |= InputFlags::Grenade; }
        if (barrierQueued) { flags |= InputFlags::Barrier; }
        if (dashQueued) { flags |= InputFlags::Dash; }

        const bool isAk = local.playerClass == PlayerClass::Ak47;
        const bool isPistol = local.playerClass == PlayerClass::Pistol;
        const bool wantsToFire = canShoot && (isAk ? IsMouseButtonDown(MOUSE_BUTTON_LEFT)
                                      : (isPistol ? IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                                                  : (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))));

        if (canShoot && (local.ammo == 0 || localPredictedAmmo == 0) && localReloadRemaining <= 0.0F) {
            localReloadRemaining = ClassReloadSeconds(local.playerClass);
            localFireCooldown = ClassReloadSeconds(local.playerClass);
            flags |= InputFlags::Reload;
        }

        if (wantsToFire) {
            if (localReloadRemaining <= 0.0F) {
                if (localPredictedAmmo == 0 || local.ammo == 0) {
                    localReloadRemaining = ClassReloadSeconds(local.playerClass);
                    localFireCooldown = ClassReloadSeconds(local.playerClass);
                    flags |= InputFlags::Reload;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        client.Audio().PlayEmpty();
                    }
                } else if (localFireCooldown <= 0.0F) {
                    client.Audio().PlayShot(local.playerClass);
                    localFireCooldown = ClassCooldown(local.playerClass);
                    if (localPredictedAmmo > 0) {
                        --localPredictedAmmo;
                        if (localPredictedAmmo == 0) {
                            localReloadRemaining = ClassReloadSeconds(local.playerClass);
                            localFireCooldown = ClassReloadSeconds(local.playerClass);
                            flags |= InputFlags::Reload;
                        }
                    }
                }
            }
        }
    }

    sendAccumulator += dt;
    if (sendAccumulator >= cfg::ClientSendSeconds) {
        InputPacket packet{};
        packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
        packet.sequence = ++sequence;
        packet.flags = flags;
        packet.aimX = aimDelta.x / aimLength;
        packet.aimY = aimDelta.y / aimLength;
        packet.grenadeTargetX = grenadeTarget.x;
        packet.grenadeTargetY = grenadeTarget.y;
        client.Network().SendInput(packet);
        sendAccumulator = 0.0F;
        grenadeQueued = false;
        barrierQueued = false;
        respawnQueued = false;
        dashQueued = false;
    }

    for (std::size_t index = 0; index < snapshot.playerCount && index < cfg::MaxPlayers; ++index) {
        const PlayerSnapshot& player = snapshot.players[index];
        if (player.id != local.id && previousHealth[index] > 0.0F && player.health < previousHealth[index]) {
            client.Audio().PlayHitmarker();
        }
        if (player.id != local.id && player.alive && player.connected && snapshot.matchRunning) {
            if (previousAmmo[index] > player.ammo) {
                const std::uint8_t bulletsFired = previousAmmo[index] - player.ammo;
                if (bulletsFired <= 5) {
                    const Vector2 soundPos{player.x, player.y};
                    const Vector2 listenerPos{local.x, local.y};
                    for (std::uint8_t b = 0; b < bulletsFired; ++b) {
                        client.Audio().PlayShotSpatial(player.playerClass, soundPos, listenerPos);
                    }
                }
            }
        }
        previousHealth[index] = player.health;
        previousAmmo[index] = player.ammo;
    }
}

void GameplayScreen::Draw(GameClient& client) const {
    const SnapshotPacket& snapshot = client.Snapshot();
    const PlayerSnapshot local = client.LocalPlayer();
    const Camera2D camera = MakeCamera(local, localInterpPos);

    BeginMode2D(camera);
    DrawMap(snapshot.mapIndex);

    // Camera frustum bounds in world coordinates for culling offscreen elements
    const float halfViewW = (static_cast<float>(GetScreenWidth()) * 0.5F) / camera.zoom;
    const float halfViewH = (static_cast<float>(GetScreenHeight()) * 0.5F) / camera.zoom;
    const Rectangle cullBox{
        camera.target.x - halfViewW - 64.0F,
        camera.target.y - halfViewH - 64.0F,
        (halfViewW * 2.0F) + 128.0F,
        (halfViewH * 2.0F) + 128.0F
    };

    for (std::size_t index = 0; index < snapshot.barrierCount; ++index) {
        const BarrierSnapshot& barrier = snapshot.barriers[index];
        if (barrier.x < cullBox.x || barrier.x > cullBox.x + cullBox.width ||
            barrier.y < cullBox.y || barrier.y > cullBox.y + cullBox.height) {
            continue;
        }
        const Vector2 pos{barrier.x, barrier.y};
        const Rectangle rect{pos.x, pos.y, cfg::BarrierWidth, cfg::BarrierHeight};
        const bool isOwn = barrier.ownerId == local.id;
        const Color col = isOwn ? Color{110, 185, 230, 255} : BarrierColor;
        DrawRectanglePro(rect, Vector2{cfg::BarrierWidth * 0.5F, cfg::BarrierHeight * 0.5F}, barrier.rotation * 180.0F / Pi, col);
    }
    for (std::size_t index = 0; index < snapshot.bulletCount; ++index) {
        const float bx = snapshot.bullets[index].x;
        const float by = snapshot.bullets[index].y;
        if (bx < cullBox.x || bx > cullBox.x + cullBox.width ||
            by < cullBox.y || by > cullBox.y + cullBox.height) {
            continue;
        }
        DrawCircleV(Vector2{bx, by}, cfg::BulletRadius, BulletColor);
    }
    for (std::size_t index = 0; index < snapshot.grenadeCount; ++index) {
        const GrenadeSnapshot& grenade = snapshot.grenades[index];
        if (grenade.x < cullBox.x || grenade.x > cullBox.x + cullBox.width ||
            grenade.y < cullBox.y || grenade.y > cullBox.y + cullBox.height) {
            continue;
        }
        const Vector2 shadow{grenade.x, grenade.y};
        const Vector2 body{grenade.x, grenade.y - grenade.height};
        DrawEllipse(static_cast<int>(shadow.x), static_cast<int>(shadow.y), cfg::GrenadeRadius + 5.0F, cfg::GrenadeRadius * 0.55F, Color{0, 0, 0, 90});
        DrawCircleV(body, cfg::GrenadeRadius + 4.0F, GrenadeColor);
        if (grenade.fuse >= 0.0F) {
            const float fuseRatio = std::clamp(grenade.fuse / cfg::GrenadeFuseSeconds, 0.0F, 1.0F);
            DrawCircleLines(static_cast<int>(grenade.x), static_cast<int>(grenade.y), cfg::GrenadeExplosionRadius * (1.0F - fuseRatio), GrenadeFuseColor);
        }
    }
    for (std::size_t index = 0; index < snapshot.playerCount; ++index) {
        const PlayerSnapshot& player = snapshot.players[index];
        if (snapshot.matchRunning && !player.alive) {
            continue;
        }
        Vector2 pos{player.x, player.y};
        float rotation = std::atan2(player.aimY, player.aimX) * 180.0F / Pi - 90.0F;
        for (const auto& entry : playerInterp) {
            if (entry.active && entry.id == player.id) {
                pos = entry.currentPos;
                rotation = entry.currentRotation;
                break;
            }
        }

        const bool isLocal = (player.id == local.id);
        const std::string displayName = PlayerDisplayName(player);

        // Player Name Tag above character
        const int nameFontSize = 20;
        const int nameW = MeasureText(displayName.c_str(), nameFontSize);
        const float tagPadX = 8.0F;
        const float tagPadY = 3.0F;
        const float tagW = static_cast<float>(nameW) + tagPadX * 2.0F;
        const float tagH = static_cast<float>(nameFontSize) + tagPadY * 2.0F;
        const float tagX = pos.x - tagW * 0.5F;
        const float tagY = pos.y - 74.0F;

        // Background pill
        DrawRectangleRounded(Rectangle{tagX, tagY, tagW, tagH}, 0.35F, 6, Color{16, 20, 26, 205});
        DrawRectangleLinesEx(Rectangle{tagX, tagY, tagW, tagH}, 1.5F, isLocal ? Color{255, 215, 80, 190} : (IsTeamMode(snapshot.mode) ? (player.team == TeamId::Red ? Color{235, 85, 75, 190} : Color{85, 160, 245, 190}) : Color{70, 85, 105, 190}));

        // Name text
        Color nameTextColor = isLocal ? Color{255, 230, 110, 255} : (IsTeamMode(snapshot.mode) ? TeamColor(player.team) : WhiteColor);
        DrawText(displayName.c_str(), static_cast<int>(pos.x - static_cast<float>(nameW) * 0.5F), static_cast<int>(tagY + tagPadY), nameFontSize, nameTextColor);

        if (!player.connected) {
            const int discFontSize = 20;
            const int discW = MeasureText("[Disconnected]", discFontSize);
            DrawText("[Disconnected]", static_cast<int>(pos.x - static_cast<float>(discW) * 0.5F), static_cast<int>(tagY - 24.0F), discFontSize, Color{255, 210, 70, 240});
        }
        if (player.invulnerableRemaining > 0.0F) {
            const float pulse = 0.5F + 0.5F * std::sin(GetTime() * 10.0F);
            const float baseRadius = 26.0F;
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), baseRadius + 4.0F + 3.0F * pulse, Color{100, 220, 255, static_cast<unsigned char>(70 + 60 * pulse)});
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), baseRadius + 2.0F, Color{130, 240, 255, 230});
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), baseRadius, Color{220, 250, 255, 255});
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), baseRadius - 2.0F, Color{130, 240, 255, 200});
        }
        client.Assets().DrawSoldier(isLocal, pos, rotation);
        if (!isLocal) {
            DrawRectangle(static_cast<int>(pos.x - 24.0F), static_cast<int>(pos.y - 38.0F), 48, 5, Color{20, 20, 20, 210});
            DrawRectangle(static_cast<int>(pos.x - 24.0F), static_cast<int>(pos.y - 38.0F), static_cast<int>(48.0F * (player.health / cfg::PlayerMaxHealth)), 5, Color{80, 220, 120, 255});
        }
    }
    EndMode2D();

    const int hudH = static_cast<int>(Sf(64.0F));
    DrawRectangle(0, 0, GetScreenWidth(), hudH, Color{14, 17, 21, 215});
    DrawLine(0, hudH, GetScreenWidth(), hudH, Color{36, 44, 56, 180});

    // 1. Left Section: Gamemode • Map & K/D
    float leftX = Sf(120.0F);
    const char* modeStr = ModeText(snapshot.mode);
    const char* mapStr = MapName(snapshot.mapIndex);
    const int modeW = MeasureText(modeStr, Si(18));
    const int mapW = MeasureText(mapStr, Si(18));

    DrawText(modeStr, static_cast<int>(leftX), static_cast<int>(Sf(22.0F)), Si(18), WhiteColor);
    leftX += static_cast<float>(modeW) + Sf(12.0F);
    DrawCircle(static_cast<int>(leftX + Sf(3.0F)), static_cast<int>(Sf(31.0F)), Sf(2.5F), MutedColor);
    leftX += Sf(18.0F);
    DrawText(mapStr, static_cast<int>(leftX), static_cast<int>(Sf(22.0F)), Si(18), MutedColor);
    leftX += static_cast<float>(mapW) + Sf(32.0F);

    const int kdLabelW = MeasureText("K/D", Si(13));
    DrawText("K/D", static_cast<int>(leftX), static_cast<int>(Sf(26.0F)), Si(13), MutedColor);
    DrawText(TextFormat("%d/%d", local.kills, local.deaths), static_cast<int>(leftX + static_cast<float>(kdLabelW) + Sf(6.0F)), static_cast<int>(Sf(22.0F)), Si(18), WhiteColor);

    // 2. Center Section: Match Status (Team scores for team modes, Countdown clock for timed modes)
    const float screenCenterX = static_cast<float>(GetScreenWidth()) * 0.5F;
    const bool isTimed = snapshot.mode == GameMode::FfaTimed || snapshot.mode == GameMode::TdmTimed;
    const int timeLabelW = MeasureText("TIME", Si(13));

    if (IsTeamMode(snapshot.mode)) {
        const char* redText = TextFormat("RED %d", snapshot.redScore);
        const char* blueText = TextFormat("%d BLUE", snapshot.blueScore);
        const int redW = MeasureText(redText, Si(18));
        const int blueW = MeasureText(blueText, Si(18));

        if (isTimed) {
            const int totalSec = std::max(0, static_cast<int>(snapshot.timeRemaining));
            const char* timeText = TextFormat("%02d:%02d", totalSec / 60, totalSec % 60);
            const int timeW = MeasureText(timeText, Si(22));
            const int timeX = static_cast<int>(screenCenterX - static_cast<float>(timeW) * 0.5F);

            DrawText(timeText, timeX, static_cast<int>(Sf(20.0F)), Si(22), WhiteColor);
            DrawText(redText, timeX - static_cast<int>(Sf(32.0F)) - redW, static_cast<int>(Sf(23.0F)), Si(18), RedColor);
            DrawText(blueText, timeX + timeW + static_cast<int>(Sf(32.0F)), static_cast<int>(Sf(23.0F)), Si(18), BlueColor);
        } else {
            const char* sep = "-";
            const int sepW = MeasureText(sep, Si(18));
            const int totalScoreW = redW + static_cast<int>(Sf(20.0F)) + sepW + static_cast<int>(Sf(20.0F)) + blueW;
            const int startX = static_cast<int>(screenCenterX - static_cast<float>(totalScoreW) * 0.5F);

            DrawText(redText, startX, static_cast<int>(Sf(23.0F)), Si(18), RedColor);
            DrawText(sep, startX + redW + static_cast<int>(Sf(20.0F)), static_cast<int>(Sf(23.0F)), Si(18), MutedColor);
            DrawText(blueText, startX + redW + static_cast<int>(Sf(20.0F)) + sepW + static_cast<int>(Sf(20.0F)), static_cast<int>(Sf(23.0F)), Si(18), BlueColor);
        }
    } else if (isTimed) {
        const int totalSec = std::max(0, static_cast<int>(snapshot.timeRemaining));
        const char* timeText = TextFormat("%02d:%02d", totalSec / 60, totalSec % 60);
        const int valW = MeasureText(timeText, Si(22));
        const int totalW = timeLabelW + static_cast<int>(Sf(8.0F)) + valW;
        const int startTimerX = static_cast<int>(screenCenterX - static_cast<float>(totalW) * 0.5F);

        DrawText("TIME", startTimerX, static_cast<int>(Sf(26.0F)), Si(13), MutedColor);
        DrawText(timeText, startTimerX + timeLabelW + static_cast<int>(Sf(8.0F)), static_cast<int>(Sf(20.0F)), Si(22), WhiteColor);
    }

    // 3. Right Section: Room Code / Status (aligned cleanly away from start button)
    const float statusRight = !snapshot.matchRunning
        ? (static_cast<float>(GetScreenWidth()) - Sf(StartButtonWidth) - Sf(40.0F))
        : (static_cast<float>(GetScreenWidth()) - Sf(28.0F));

    const std::string roomText = !client.RoomCode().empty()
        ? ("Room " + client.RoomCode())
        : client.Status();
    const int HeaderRoomCodeSize = Si(22);
    const int statusW = MeasureText(roomText.c_str(), HeaderRoomCodeSize);
    DrawText(roomText.c_str(), static_cast<int>(statusRight - static_cast<float>(statusW)), static_cast<int>(Sf(21.0F)), HeaderRoomCodeSize, WhiteColor);

    DrawHotbar(local, client);
    DrawKillFeed(local, snapshot);

    if (!snapshot.matchRunning) {
        if (client.IsHost()) {
            const Rectangle start{static_cast<float>(GetScreenWidth()) - Sf(StartButtonWidth) - Sf(20.0F), Sf(12.0F), Sf(StartButtonWidth), Sf(StartButtonHeight)};
            const bool hover = CheckCollisionPointRec(GetMousePosition(), start);
            DrawRectangleRounded(start, 0.08F, 8, hover ? Color{62, 142, 116, 255} : Color{48, 118, 96, 255});
            constexpr const char* StartText = "Enter: Start";
            const int scaledSmall = Si(SmallSize);
            const int textWidth = MeasureText(StartText, scaledSmall);
            DrawText(StartText,
                static_cast<int>(start.x + (start.width - static_cast<float>(textWidth)) * 0.5F),
                static_cast<int>(start.y + (start.height - static_cast<float>(scaledSmall)) * 0.5F),
                scaledSmall,
                WhiteColor);
        } else {
            constexpr const char* WaitText = "Waiting for host...";
            const int scaledSmall = Si(SmallSize);
            const int textWidth = MeasureText(WaitText, scaledSmall);
            DrawText(WaitText, static_cast<int>(static_cast<float>(GetScreenWidth()) - static_cast<float>(textWidth) - Sf(30.0F)), static_cast<int>(Sf(24.0F)), scaledSmall, MutedColor);
        }
    }
    if (snapshot.matchRunning && !local.alive && local.id != 0) {
        DrawRespawnModal(client, local);
    }
    if (snapshot.matchRunning && snapshot.startCountdown > 0.0F) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 95});
        const int count = std::max(1, static_cast<int>(std::ceil(snapshot.startCountdown)));
        const char* countText = TextFormat("%d", count);
        const int scaledCountSize = Si(CountdownSize);
        const int scaledLabelSize = Si(CountdownLabelSize);
        const int countWidth = MeasureText(countText, scaledCountSize);
        DrawText(countText,
            (GetScreenWidth() - countWidth) / 2,
            (GetScreenHeight() - scaledCountSize) / 2 - static_cast<int>(Sf(36.0F)),
            scaledCountSize,
            WhiteColor);
        constexpr const char* label = "MATCH STARTING";
        const int labelWidth = MeasureText(label, scaledLabelSize);
        DrawText(label,
            (GetScreenWidth() - labelWidth) / 2,
            (GetScreenHeight() + scaledCountSize) / 2 - static_cast<int>(Sf(18.0F)),
            scaledLabelSize,
            WhiteColor);
    }
    if (endModalOpen) {
        DrawEndModal();
    }
    if (IsKeyDown(KEY_TAB) && !escapeMenuOpen && !endModalOpen && !confirmLeaveOpen && pendingKickPlayerId == 0) {
        DrawScoreboard(snapshot, local, client);
    }
    if (escapeMenuOpen) {
        DrawEscapeMenu(client);
    }
    if (pendingKickPlayerId != 0) {
        DrawConfirmKickModal();
    }
    if (confirmLeaveOpen) {
        DrawConfirmLeaveModal();
    }
}

Camera2D GameplayScreen::MakeCamera(const PlayerSnapshot& local, Vector2 localPos) const {
    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());
    const Vector2 screenCenter{screenW * 0.5F, screenH * 0.5F};

    Vector2 baseTarget{};
    Vector2 baseOffset{};
    float baseZoom = DefaultCameraZoom;

    if (local.playerClass == PlayerClass::Sniper) {
        baseTarget = Vector2{cfg::WorldWidth * 0.5F, cfg::WorldHeight * 0.5F};
        const float hudH = Sf(64.0F);
        baseOffset = Vector2{screenW * 0.5F, (screenH + hudH) * 0.5F};
        const float fitX = screenW / cfg::WorldWidth;
        const float fitY = (screenH - hudH) / cfg::WorldHeight;
        baseZoom = std::min(fitX, fitY);
    } else {
        baseTarget = localPos;
        baseOffset = screenCenter;
        baseZoom = DefaultCameraZoom * (screenW / 1280.0F);
    }

    Camera2D camera{};
    camera.rotation = 0.0F;
    camera.offset = baseOffset;
    camera.target = baseTarget;
    camera.zoom = baseZoom;
    return camera;
}

void GameplayScreen::EnsureMapTexture(std::uint8_t mapIndex) const {
    if (mapTexture.id != 0 && cachedMapIndex == static_cast<int>(mapIndex)) {
        return;
    }
    if (mapTexture.id != 0) {
        UnloadRenderTexture(mapTexture);
        mapTexture = RenderTexture2D{};
    }
    mapTexture = LoadRenderTexture(static_cast<int>(cfg::WorldWidth), static_cast<int>(cfg::WorldHeight));
    cachedMapIndex = static_cast<int>(mapIndex);

    BeginTextureMode(mapTexture);
    ClearBackground(FloorColor);

    int count = 0;
    const std::array<ObstacleView, 8> obstacles = Obstacles(mapIndex, count);
    for (int index = 0; index < count; ++index) {
        const Rectangle rect = obstacles[static_cast<std::size_t>(index)].rect;
        DrawRectangleRec(Rectangle{rect.x, rect.y, rect.width, rect.height}, ObstacleColor);
    }
    EndTextureMode();
}

void GameplayScreen::DrawMap(std::uint8_t mapIndex) const {
    EnsureMapTexture(mapIndex);
    if (mapTexture.id != 0) {
        const Rectangle src{0.0F, 0.0F, static_cast<float>(mapTexture.texture.width), -static_cast<float>(mapTexture.texture.height)};
        DrawTextureRec(mapTexture.texture, src, Vector2{0.0F, 0.0F}, WHITE);
    } else {
        DrawRectangle(0, 0, static_cast<int>(cfg::WorldWidth), static_cast<int>(cfg::WorldHeight), FloorColor);
        int count = 0;
        const std::array<ObstacleView, 8> obstacles = Obstacles(mapIndex, count);
        for (int index = 0; index < count; ++index) {
            const Rectangle rect = obstacles[static_cast<std::size_t>(index)].rect;
            DrawRectangleRec(rect, ObstacleColor);
        }
    }
}

void GameplayScreen::UpdateEndModal() {
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(EndModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(EndModalHeight)) * 0.5F,
        Sf(EndModalWidth),
        Sf(EndModalHeight)
    };
    const Rectangle closeButton{modal.x + modal.width - Sf(56.0F), modal.y + Sf(20.0F), Sf(36.0F), Sf(36.0F)};
    const Rectangle okButton{modal.x + (modal.width - Sf(EndButtonWidth)) * 0.5F, modal.y + modal.height - Sf(68.0F), Sf(EndButtonWidth), Sf(EndButtonHeight)};
    if (IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_ENTER) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            (CheckCollisionPointRec(GetMousePosition(), closeButton) ||
                CheckCollisionPointRec(GetMousePosition(), okButton) ||
                !CheckCollisionPointRec(GetMousePosition(), modal)))) {
        endModalOpen = false;
        fireBlockTimer = 0.5F;
    }
}

void GameplayScreen::DrawEndModal() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 145});
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(EndModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(EndModalHeight)) * 0.5F,
        Sf(EndModalWidth),
        Sf(EndModalHeight)
    };
    DrawRectangleRounded(modal, 0.04F, 12, Color{28, 34, 43, 248});
    DrawRectangleLinesEx(modal, 2.0F, Color{78, 96, 118, 255});

    const Rectangle closeButton{modal.x + modal.width - Sf(56.0F), modal.y + Sf(20.0F), Sf(36.0F), Sf(36.0F)};
    const bool closeHover = CheckCollisionPointRec(GetMousePosition(), closeButton);
    DrawRectangleRounded(closeButton, 0.18F, 8, closeHover ? Color{128, 58, 58, 255} : Color{70, 78, 90, 255});
    DrawText("X", static_cast<int>(closeButton.x + Sf(11.0F)), static_cast<int>(closeButton.y + Sf(7.0F)), Si(22), WhiteColor);

    float x = modal.x + Sf(34.0F);
    float y = modal.y + Sf(28.0F);
    DrawText("Match Results", static_cast<int>(x), static_cast<int>(y), Si(EndTitleSize), WhiteColor);
    y += Sf(54.0F);
    DrawText(WinnerText(endSnapshot).c_str(), static_cast<int>(x), static_cast<int>(y), Si(30), Color{255, 220, 105, 255});
    y += Sf(46.0F);
    DrawText(ModeText(endSnapshot.mode), static_cast<int>(x), static_cast<int>(y), Si(EndTextSize), MutedColor);
    DrawText(TextFormat("Red %d   Blue %d", endSnapshot.redScore, endSnapshot.blueScore),
        static_cast<int>(x + Sf(220.0F)),
        static_cast<int>(y),
        Si(EndTextSize),
        WhiteColor);
    y += Sf(48.0F);

    DrawText("Player", static_cast<int>(x), static_cast<int>(y), Si(EndSmallSize), MutedColor);
    DrawText("Team", static_cast<int>(x + Sf(150.0F)), static_cast<int>(y), Si(EndSmallSize), MutedColor);
    DrawText("Kills", static_cast<int>(x + Sf(300.0F)), static_cast<int>(y), Si(EndSmallSize), MutedColor);
    DrawText("Deaths", static_cast<int>(x + Sf(410.0F)), static_cast<int>(y), Si(EndSmallSize), MutedColor);
    DrawText("K/D", static_cast<int>(x + Sf(540.0F)), static_cast<int>(y), Si(EndSmallSize), MutedColor);
    y += Sf(30.0F);

    for (std::size_t index = 0; index < endSnapshot.playerCount; ++index) {
        const PlayerSnapshot& player = endSnapshot.players[index];
        const float ratio = player.deaths == 0 ? static_cast<float>(player.kills) : static_cast<float>(player.kills) / static_cast<float>(player.deaths);
        const Color rowColor = index % 2 == 0 ? Color{38, 46, 58, 220} : Color{32, 39, 49, 220};
        DrawRectangleRounded(Rectangle{x - Sf(10.0F), y - Sf(7.0F), Sf(EndModalWidth) - Sf(68.0F), Sf(34.0F)}, 0.03F, 8, rowColor);
        DrawText(PlayerDisplayName(player).c_str(), static_cast<int>(x), static_cast<int>(y), Si(EndSmallSize), WhiteColor);
        DrawText(TeamText(player.team), static_cast<int>(x + Sf(150.0F)), static_cast<int>(y), Si(EndSmallSize), TeamColor(player.team));
        DrawText(TextFormat("%d", player.kills), static_cast<int>(x + Sf(300.0F)), static_cast<int>(y), Si(EndSmallSize), WhiteColor);
        DrawText(TextFormat("%d", player.deaths), static_cast<int>(x + Sf(410.0F)), static_cast<int>(y), Si(EndSmallSize), WhiteColor);
        DrawText(TextFormat("%.2f", ratio), static_cast<int>(x + Sf(540.0F)), static_cast<int>(y), Si(EndSmallSize), WhiteColor);
        y += Sf(40.0F);
    }

    const Rectangle okButton{modal.x + (modal.width - Sf(EndButtonWidth)) * 0.5F, modal.y + modal.height - Sf(68.0F), Sf(EndButtonWidth), Sf(EndButtonHeight)};
    const bool okHover = CheckCollisionPointRec(GetMousePosition(), okButton);
    DrawRectangleRounded(okButton, 0.08F, 8, okHover ? Color{65, 145, 116, 255} : Color{48, 118, 96, 255});
    DrawText("Close", static_cast<int>(okButton.x + Sf(52.0F)), static_cast<int>(okButton.y + Sf(12.0F)), Si(EndSmallSize), WhiteColor);
}

void GameplayScreen::UpdateRespawnModal(GameClient& client, const PlayerSnapshot& local) {
    fireBlockTimer = 0.5F;
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(RespawnModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(RespawnModalHeight)) * 0.5F,
        Sf(RespawnModalWidth),
        Sf(RespawnModalHeight)
    };
    const float startX = modal.x + Sf(24.0F);
    const float y = modal.y + Sf(96.0F);
    const Rectangle ak{startX, y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)};
    const Rectangle pistol{startX + Sf(152.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)};
    const Rectangle sniper{startX + Sf(304.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)};
    const Rectangle shotgun{startX + Sf(456.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)};
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, ak)) { client.SetClass(PlayerClass::Ak47); client.Network().SendConfig(client.MakeConfig()); fireBlockTimer = 0.5F; }
        if (CheckCollisionPointRec(mouse, pistol)) { client.SetClass(PlayerClass::Pistol); client.Network().SendConfig(client.MakeConfig()); fireBlockTimer = 0.5F; }
        if (CheckCollisionPointRec(mouse, sniper)) { client.SetClass(PlayerClass::Sniper); client.Network().SendConfig(client.MakeConfig()); fireBlockTimer = 0.5F; }
        if (CheckCollisionPointRec(mouse, shotgun)) { client.SetClass(PlayerClass::Shotgun); client.Network().SendConfig(client.MakeConfig()); fireBlockTimer = 0.5F; }
    }
    const Rectangle respawn{modal.x + (modal.width - Sf(230.0F)) * 0.5F, modal.y + Sf(238.0F), Sf(230.0F), Sf(50.0F)};
    if (localRespawnTimer <= 0.0F) {
        if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), respawn)) || IsKeyPressed(KEY_ENTER)) {
            respawnQueued = true;
            fireBlockTimer = 0.5F;
        }
    }
}

void GameplayScreen::DrawRespawnModal(GameClient& client, const PlayerSnapshot& local) const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 145});
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(RespawnModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(RespawnModalHeight)) * 0.5F,
        Sf(RespawnModalWidth),
        Sf(RespawnModalHeight)
    };
    DrawRectangleRounded(modal, 0.04F, 12, Color{28, 34, 43, 248});
    DrawRectangleLinesEx(modal, 2.0F, Color{78, 96, 118, 255});
    DrawText("Choose Class", static_cast<int>(modal.x + Sf(24.0F)), static_cast<int>(modal.y + Sf(22.0F)), Si(30), WhiteColor);
    DrawText("You are invulnerable until you respawn.", static_cast<int>(modal.x + Sf(24.0F)), static_cast<int>(modal.y + Sf(60.0F)), Si(17), MutedColor);

    const float startX = modal.x + Sf(24.0F);
    const float y = modal.y + Sf(96.0F);
    const Rectangle buttons[] = {
        Rectangle{startX, y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)},
        Rectangle{startX + Sf(152.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)},
        Rectangle{startX + Sf(304.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)},
        Rectangle{startX + Sf(456.0F), y, Sf(RespawnClassWidth), Sf(RespawnClassHeight)}
    };
    const char* labels[] = {"AK47", "M1911", "KAR98k", "Mossberg"};
    const PlayerClass classes[] = {PlayerClass::Ak47, PlayerClass::Pistol, PlayerClass::Sniper, PlayerClass::Shotgun};
    const PlayerClass selectedClass = client.SelectedClass();

    const int classFontSize = Si(22);
    for (int index = 0; index < 4; ++index) {
        const bool active = (selectedClass == classes[index]);
        const bool hover = CheckCollisionPointRec(GetMousePosition(), buttons[index]);

        DrawRectangleRounded(buttons[index], 0.08F, 8, active ? Color{48, 118, 96, 255} : (hover ? Color{68, 82, 101, 255} : Color{44, 54, 70, 255}));
        if (active) {
            DrawRectangleLinesEx(buttons[index], 2.0F, Color{80, 220, 160, 255});
        }

        const int labelW = MeasureText(labels[index], classFontSize);
        const int labelX = static_cast<int>(buttons[index].x + (buttons[index].width - static_cast<float>(labelW)) * 0.5F);
        const int labelY = static_cast<int>(buttons[index].y + (buttons[index].height - static_cast<float>(classFontSize)) * 0.5F);
        DrawText(labels[index], labelX, labelY, classFontSize, WhiteColor);
    }

    const Rectangle infoBox{modal.x + Sf(24.0F), modal.y + Sf(162.0F), Sf(592.0F), Sf(58.0F)};
    DrawRectangleRounded(infoBox, 0.08F, 6, Color{20, 26, 35, 230});
    DrawRectangleLinesEx(infoBox, 1.0F, Color{54, 68, 86, 200});

    const int gCount = ClassMaxGrenades(selectedClass);
    const int bCount = ClassMaxBarriers(selectedClass);
    const int ammoCount = ClassMaxAmmo(selectedClass);

    DrawText(TextFormat("%s Loadout", ClassName(selectedClass)),
        static_cast<int>(infoBox.x + Sf(16.0F)), static_cast<int>(infoBox.y + Sf(8.0F)), Si(17), Color{255, 220, 110, 255});
    DrawText(TextFormat("Grenades: %d frag%s    |    Barriers: %d deployable%s    |    Magazine: %d rounds",
        gCount, gCount == 1 ? "" : "s",
        bCount, bCount == 1 ? "" : "s",
        ammoCount),
        static_cast<int>(infoBox.x + Sf(16.0F)), static_cast<int>(infoBox.y + Sf(32.0F)), Si(14), Color{215, 225, 238, 255});

    const Rectangle respawn{modal.x + (modal.width - Sf(230.0F)) * 0.5F, modal.y + Sf(238.0F), Sf(230.0F), Sf(50.0F)};
    const bool canRespawn = localRespawnTimer <= 0.0F;
    const bool hover = canRespawn && CheckCollisionPointRec(GetMousePosition(), respawn);
    DrawRectangleRounded(respawn, 0.08F, 8, canRespawn ? (hover ? Color{65, 145, 116, 255} : Color{48, 118, 96, 255}) : Color{50, 58, 70, 255});

    if (canRespawn) {
        const int fontSize = Si(22);
        const int rw = MeasureText("Respawn", fontSize);
        DrawText("Respawn", static_cast<int>(respawn.x + (respawn.width - static_cast<float>(rw)) * 0.5F), static_cast<int>(respawn.y + (respawn.height - static_cast<float>(fontSize)) * 0.5F), fontSize, WhiteColor);
        const int subW = MeasureText("Enter also respawns", Si(15));
        DrawText("Enter also respawns", static_cast<int>(modal.x + (modal.width - static_cast<float>(subW)) * 0.5F), static_cast<int>(respawn.y + respawn.height + Sf(8.0F)), Si(15), MutedColor);
    } else {
        int secondsLeft = static_cast<int>(std::ceil(localRespawnTimer));
        if (secondsLeft < 1) secondsLeft = 1;
        const std::string countdownText = TextFormat("%d", secondsLeft);
        const int fontSize = Si(24);
        const int rw = MeasureText(countdownText.c_str(), fontSize);
        DrawText(countdownText.c_str(), static_cast<int>(respawn.x + (respawn.width - static_cast<float>(rw)) * 0.5F), static_cast<int>(respawn.y + (respawn.height - static_cast<float>(fontSize)) * 0.5F), fontSize, Color{190, 200, 215, 255});
        const int subW = MeasureText("Respawn countdown...", Si(15));
        DrawText("Respawn countdown...", static_cast<int>(modal.x + (modal.width - static_cast<float>(subW)) * 0.5F), static_cast<int>(respawn.y + respawn.height + Sf(8.0F)), Si(15), MutedColor);
    }
}

Rectangle GameplayScreen::KillFeedChevronBounds() const {
    return Rectangle{Sf(16.0F), Sf(72.0F), Sf(110.0F), Sf(24.0F)};
}

void GameplayScreen::UpdateKillFeed(const SnapshotPacket& snapshot, float dt) {
    if (snapshot.matchRunning && snapshot.startCountdown > 0.0F && !previousMatchRunning) {
        killFeedItems.clear();
        lastKillEventId = 0;
    }

    for (auto it = killFeedItems.begin(); it != killFeedItems.end();) {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0F) {
            it = killFeedItems.erase(it);
        } else {
            ++it;
        }
    }

    for (std::size_t i = 0; i < snapshot.killFeedCount; ++i) {
        const KillFeedSnapshot& ev = snapshot.killFeed[i];
        if (ev.eventId > lastKillEventId) {
            KillFeedItem item{};
            item.eventId = ev.eventId;
            item.killerId = ev.killerId;
            item.victimId = ev.victimId;
            item.killerTeam = ev.killerTeam;
            item.victimTeam = ev.victimTeam;
            item.weapon = ev.weapon;
            item.isSuicide = ev.isSuicide;
            item.timeRemaining = cfg::KillFeedDuration;
            killFeedItems.push_back(item);
            lastKillEventId = std::max(lastKillEventId, ev.eventId);
        }
    }

    constexpr std::size_t MaxDisplayItems = 5;
    while (killFeedItems.size() > MaxDisplayItems) {
        killFeedItems.erase(killFeedItems.begin());
    }
}

void GameplayScreen::DrawKillFeed(const PlayerSnapshot& local, const SnapshotPacket& snapshot) const {
    const Rectangle chevronBtn = KillFeedChevronBounds();
    const bool hover = CheckCollisionPointRec(GetMousePosition(), chevronBtn);

    DrawRectangleRounded(chevronBtn, 0.25F, 6, hover ? Color{42, 54, 70, 230} : Color{24, 30, 38, 200});
    DrawRectangleLinesEx(chevronBtn, 1.0F, hover ? Color{80, 105, 135, 240} : Color{50, 62, 78, 200});

    const float cx = chevronBtn.x + Sf(14.0F);
    const float cy = chevronBtn.y + Sf(12.0F);
    const Color iconColor = hover ? Color{255, 255, 255, 255} : Color{200, 210, 225, 230};
    if (killfeedCollapsed) {
        DrawLineEx(Vector2{cx - Sf(4.0F), cy - Sf(2.0F)}, Vector2{cx, cy + Sf(3.0F)}, 2.0F, iconColor);
        DrawLineEx(Vector2{cx, cy + Sf(3.0F)}, Vector2{cx + Sf(4.0F), cy - Sf(2.0F)}, 2.0F, iconColor);
    } else {
        DrawLineEx(Vector2{cx - Sf(4.0F), cy + Sf(2.0F)}, Vector2{cx, cy - Sf(3.0F)}, 2.0F, iconColor);
        DrawLineEx(Vector2{cx, cy - Sf(3.0F)}, Vector2{cx + Sf(4.0F), cy + Sf(2.0F)}, 2.0F, iconColor);
    }

    const char* label = (killfeedCollapsed && !killFeedItems.empty())
        ? TextFormat("FEED (%d)", static_cast<int>(killFeedItems.size()))
        : "KILLFEED";
    DrawText(label, static_cast<int>(chevronBtn.x + Sf(28.0F)), static_cast<int>(chevronBtn.y + Sf(5.0F)), Si(14), iconColor);

    if (killfeedCollapsed) {
        return;
    }

    float startY = chevronBtn.y + chevronBtn.height + Sf(6.0F);
    const int ItemFontSize = Si(14);

    const auto GetNameById = [&](std::uint32_t id) -> std::string {
        if (id == local.id && local.id != 0) {
            return "You";
        }
        for (std::size_t pIdx = 0; pIdx < snapshot.playerCount; ++pIdx) {
            if (snapshot.players[pIdx].id == id) {
                return PlayerDisplayName(snapshot.players[pIdx]);
            }
        }
        return "Player " + std::to_string(id);
    };

    for (std::size_t i = 0; i < killFeedItems.size(); ++i) {
        const KillFeedItem& item = killFeedItems[i];
        const float alphaRatio = std::clamp(item.timeRemaining / 0.8F, 0.0F, 1.0F);
        const unsigned char textAlpha = static_cast<unsigned char>(255 * alphaRatio);
        const unsigned char bgAlpha = static_cast<unsigned char>(195 * alphaRatio);
        const unsigned char borderAlpha = static_cast<unsigned char>(130 * alphaRatio);

        const auto GetColor = [&](std::uint32_t id, TeamId team) -> Color {
            if (id == local.id && local.id != 0) {
                return Color{255, 220, 80, textAlpha};
            }
            if (team == TeamId::Red) {
                return Color{235, 85, 75, textAlpha};
            }
            if (team == TeamId::Blue) {
                return Color{85, 160, 245, textAlpha};
            }
            return Color{230, 235, 242, textAlpha};
        };

        std::string killerText;
        std::string weaponText;
        std::string victimText;
        Color killerColor{230, 235, 242, textAlpha};
        Color victimColor{230, 235, 242, textAlpha};
        const Color weaponColor{180, 195, 210, textAlpha};

        if (item.weapon == KillFeedWeapon::Disconnected) {
            killerText = GetNameById(item.victimId);
            killerColor = GetColor(item.victimId, item.victimTeam);
            weaponText = "[Disconnected (10s)]";
        } else if (item.weapon == KillFeedWeapon::Reconnected) {
            killerText = GetNameById(item.victimId);
            killerColor = GetColor(item.victimId, item.victimTeam);
            weaponText = "[Reconnected]";
        } else if (item.weapon == KillFeedWeapon::Kicked) {
            killerText = GetNameById(item.victimId);
            killerColor = GetColor(item.victimId, item.victimTeam);
            weaponText = "[Kicked (timeout)]";
        } else if (item.weapon == KillFeedWeapon::LeftRoom) {
            killerText = GetNameById(item.victimId);
            killerColor = GetColor(item.victimId, item.victimTeam);
            weaponText = "[Left Room]";
        } else if (item.isSuicide) {
            killerText = GetNameById(item.victimId);
            killerColor = GetColor(item.victimId, item.victimTeam);
            weaponText = std::string("[") + WeaponName(item.weapon) + " Suicide]";
        } else {
            killerText = GetNameById(item.killerId);
            killerColor = GetColor(item.killerId, item.killerTeam);
            weaponText = std::string("[") + WeaponName(item.weapon) + "]";
            victimText = GetNameById(item.victimId);
            victimColor = GetColor(item.victimId, item.victimTeam);
        }

        const int kw = MeasureText(killerText.c_str(), ItemFontSize);
        const int ww = MeasureText(weaponText.c_str(), ItemFontSize);
        const int vw = victimText.empty() ? 0 : MeasureText(victimText.c_str(), ItemFontSize);

        const float padX = Sf(8.0F);
        const float gap = Sf(6.0F);
        const float totalW = padX * 2.0F + static_cast<float>(kw) + gap + static_cast<float>(ww) + (victimText.empty() ? 0.0F : (gap + static_cast<float>(vw)));
        const float rowH = Sf(22.0F);
        const Rectangle rowRect{chevronBtn.x, startY + (static_cast<float>(i) * Sf(26.0F)), totalW, rowH};

        DrawRectangleRounded(rowRect, 0.25F, 4, Color{18, 23, 30, bgAlpha});
        DrawRectangleLinesEx(rowRect, 1.0F, Color{52, 65, 82, borderAlpha});

        float curX = rowRect.x + padX;
        const float textY = rowRect.y + Sf(4.0F);
        DrawText(killerText.c_str(), static_cast<int>(curX), static_cast<int>(textY), ItemFontSize, killerColor);
        curX += static_cast<float>(kw) + gap;
        DrawText(weaponText.c_str(), static_cast<int>(curX), static_cast<int>(textY), ItemFontSize, weaponColor);
        curX += static_cast<float>(ww) + gap;
        if (!victimText.empty()) {
            DrawText(victimText.c_str(), static_cast<int>(curX), static_cast<int>(textY), ItemFontSize, victimColor);
        }
    }
}


void GameplayScreen::UpdateEscapeMenu(GameClient& client) {
    const float menuWidth = Sf(560.0F);
    const float menuHeight = Sf(520.0F);
    const float menuX = (static_cast<float>(GetScreenWidth()) - menuWidth) * 0.5F;
    const float menuY = (static_cast<float>(GetScreenHeight()) - menuHeight) * 0.5F;

    const bool isHost = client.IsHost();
    const Rectangle resumeBtn = isHost ? Rectangle{menuX + Sf(35.0F), menuY + menuHeight - Sf(64.0F), Sf(150.0F), Sf(44.0F)}
                                       : Rectangle{menuX + Sf(35.0F), menuY + menuHeight - Sf(64.0F), Sf(220.0F), Sf(44.0F)};
    const Rectangle restartBtn{menuX + Sf(195.0F), menuY + menuHeight - Sf(64.0F), Sf(170.0F), Sf(44.0F)};
    const Rectangle leaveBtn = isHost ? Rectangle{menuX + Sf(375.0F), menuY + menuHeight - Sf(64.0F), Sf(150.0F), Sf(44.0F)}
                                      : Rectangle{menuX + menuWidth - Sf(255.0F), menuY + menuHeight - Sf(64.0F), Sf(220.0F), Sf(44.0F)};

    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, resumeBtn) || (isHost && CheckCollisionPointRec(mouse, restartBtn)) || CheckCollisionPointRec(mouse, leaveBtn)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, resumeBtn)) {
            escapeMenuOpen = false;
            fireBlockTimer = 0.5F;
            return;
        }
        if (isHost && CheckCollisionPointRec(mouse, restartBtn)) {
            client.Network().SendStart();
            escapeMenuOpen = false;
            fireBlockTimer = 0.5F;
            return;
        }
        if (CheckCollisionPointRec(mouse, leaveBtn)) {
            confirmLeaveOpen = true;
            fireBlockTimer = 0.5F;
            return;
        }
    }

    if (client.IsHost()) {
        const SnapshotPacket& snapshot = client.Snapshot();
        const PlayerSnapshot local = client.LocalPlayer();
        const float listY = menuY + Sf(225.0F);
        for (std::size_t i = 0; i < snapshot.playerCount && i < cfg::MaxPlayers; ++i) {
            const PlayerSnapshot& p = snapshot.players[i];
            if (p.id != local.id) {
                const Rectangle kickBtn{menuX + menuWidth - Sf(35.0F) - Sf(84.0F), listY + static_cast<float>(i) * Sf(44.0F) + Sf(4.0F), Sf(80.0F), Sf(30.0F)};
                if (CheckCollisionPointRec(mouse, kickBtn)) {
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        pendingKickPlayerId = p.id;
                        fireBlockTimer = 0.5F;
                        return;
                    }
                }
            }
        }
    }
}

void GameplayScreen::DrawEscapeMenu(GameClient& client) const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 160});
    const SnapshotPacket& snapshot = client.Snapshot();
    const PlayerSnapshot local = client.LocalPlayer();

    const float menuWidth = Sf(560.0F);
    const float menuHeight = Sf(520.0F);
    const float menuX = (static_cast<float>(GetScreenWidth()) - menuWidth) * 0.5F;
    const float menuY = (static_cast<float>(GetScreenHeight()) - menuHeight) * 0.5F;
    const Rectangle menuRect{menuX, menuY, menuWidth, menuHeight};

    DrawRectangleRounded(menuRect, 0.04F, 12, Color{24, 30, 40, 252});
    DrawRectangleLinesEx(menuRect, 2.0F, Color{55, 70, 90, 255});

    // Title & Room Code
    DrawText("PAUSE MENU", static_cast<int>(menuX + Sf(35.0F)), static_cast<int>(menuY + Sf(24.0F)), Si(26), WhiteColor);
    const std::string roomStr = client.RoomCode().empty() ? client.Status() : "Room " + client.RoomCode();
    const int scaledRoomFontSize = Si(22);
    const int roomW = MeasureText(roomStr.c_str(), scaledRoomFontSize);
    DrawText(roomStr.c_str(), static_cast<int>(menuX + menuWidth - Sf(35.0F) - static_cast<float>(roomW)), static_cast<int>(menuY + Sf(26.0F)), scaledRoomFontSize, Color{100, 200, 255, 255});
    DrawLineEx(Vector2{menuX + Sf(35.0F), menuY + Sf(60.0F)}, Vector2{menuX + menuWidth - Sf(35.0F), menuY + Sf(60.0F)}, 1.5F, Color{45, 56, 72, 255});

    // Match Details Box
    const Rectangle infoRect{menuX + Sf(35.0F), menuY + Sf(72.0F), menuWidth - Sf(70.0F), Sf(105.0F)};
    DrawRectangleRounded(infoRect, 0.06F, 8, Color{18, 22, 30, 255});
    DrawRectangleLinesEx(infoRect, 1.0F, Color{40, 52, 68, 255});

    // Left Column
    DrawText("Mode:", static_cast<int>(infoRect.x + Sf(18.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), MutedColor);
    DrawText(ModeText(snapshot.mode), static_cast<int>(infoRect.x + Sf(75.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), WhiteColor);

    DrawText("Map:", static_cast<int>(infoRect.x + Sf(18.0F)), static_cast<int>(infoRect.y + Sf(44.0F)), Si(15), MutedColor);
    DrawText(MapName(snapshot.mapIndex), static_cast<int>(infoRect.x + Sf(75.0F)), static_cast<int>(infoRect.y + Sf(44.0F)), Si(15), WhiteColor);

    DrawText("Class:", static_cast<int>(infoRect.x + Sf(18.0F)), static_cast<int>(infoRect.y + Sf(72.0F)), Si(15), MutedColor);
    DrawText(ClassName(local.playerClass), static_cast<int>(infoRect.x + Sf(75.0F)), static_cast<int>(infoRect.y + Sf(72.0F)), Si(15), WhiteColor);

    // Right Column
    DrawText("Time:", static_cast<int>(infoRect.x + Sf(250.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), MutedColor);
    const bool isTimed = snapshot.mode == GameMode::FfaTimed || snapshot.mode == GameMode::TdmTimed;
    if (!isTimed) {
        DrawText("Score Limit", static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), WhiteColor);
    } else if (snapshot.matchRunning) {
        DrawText(TextFormat("%02d:%02d remaining", static_cast<int>(snapshot.timeRemaining) / 60, static_cast<int>(snapshot.timeRemaining) % 60),
            static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), WhiteColor);
    } else {
        DrawText("Lobby (Waiting)", static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(16.0F)), Si(15), WhiteColor);
    }

    DrawText("Score:", static_cast<int>(infoRect.x + Sf(250.0F)), static_cast<int>(infoRect.y + Sf(44.0F)), Si(15), MutedColor);
    if (IsTeamMode(snapshot.mode)) {
        DrawText(TextFormat("Red: %d   Blue: %d", snapshot.redScore, snapshot.blueScore),
            static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(44.0F)), Si(15), WhiteColor);
    } else {
        DrawText(TextFormat("Leader Kills: %d", BestKills(snapshot)),
            static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(44.0F)), Si(15), WhiteColor);
    }

    DrawText("Role:", static_cast<int>(infoRect.x + Sf(250.0F)), static_cast<int>(infoRect.y + Sf(72.0F)), Si(15), MutedColor);
    DrawText(client.IsHost() ? "Room Host (Admin)" : "Player", static_cast<int>(infoRect.x + Sf(300.0F)), static_cast<int>(infoRect.y + Sf(72.0F)), Si(15), client.IsHost() ? Color{255, 210, 80, 255} : WhiteColor);

    // Players List
    const float sectionTitleY = menuY + Sf(195.0F);
    if (client.IsHost()) {
        DrawText("Players (Host / Admin Kick Controls)", static_cast<int>(menuX + Sf(35.0F)), static_cast<int>(sectionTitleY), Si(16), Color{255, 210, 80, 255});
    } else {
        DrawText("Connected Players", static_cast<int>(menuX + Sf(35.0F)), static_cast<int>(sectionTitleY), Si(16), MutedColor);
    }

    const float listY = menuY + Sf(225.0F);
    for (std::size_t i = 0; i < snapshot.playerCount && i < cfg::MaxPlayers; ++i) {
        const PlayerSnapshot& p = snapshot.players[i];
        const Rectangle rowRect{menuX + Sf(35.0F), listY + static_cast<float>(i) * Sf(44.0F), menuWidth - Sf(70.0F), Sf(38.0F)};
        DrawRectangleRounded(rowRect, 0.08F, 6, Color{30, 38, 50, 255});

        const std::string nameStr = PlayerDisplayName(p);
        const char* youTag = (p.id == local.id) ? " (You)" : "";
        const char* hostTag = p.host ? " [Host]" : "";
        const char* teamTag = IsTeamMode(snapshot.mode) ? (p.team == TeamId::Red ? " [Red]" : " [Blue]") : "";
        Color nameCol = (p.id == local.id) ? Color{255, 220, 110, 255} : WhiteColor;
        DrawText(TextFormat("%s%s%s%s - %s", nameStr.c_str(), youTag, hostTag, teamTag, ClassName(p.playerClass)),
            static_cast<int>(rowRect.x + Sf(14.0F)), static_cast<int>(rowRect.y + Sf(10.0F)), Si(15), nameCol);

        if (p.connected) {
            DrawText("Online", static_cast<int>(rowRect.x + Sf(300.0F)), static_cast<int>(rowRect.y + Sf(11.0F)), Si(14), Color{80, 220, 120, 255});
        } else {
            DrawText("Offline", static_cast<int>(rowRect.x + Sf(300.0F)), static_cast<int>(rowRect.y + Sf(11.0F)), Si(14), Color{255, 200, 70, 255});
        }

        if (client.IsHost() && p.id != local.id) {
            const Rectangle kickBtn{rowRect.x + rowRect.width - Sf(84.0F), rowRect.y + Sf(4.0F), Sf(80.0F), Sf(30.0F)};
            const bool kickHov = CheckCollisionPointRec(GetMousePosition(), kickBtn);
            DrawRectangleRounded(kickBtn, 0.12F, 6, kickHov ? Color{220, 60, 60, 255} : Color{180, 45, 45, 255});
            const int scaledKickFont = Si(15);
            const int kw = MeasureText("Kick", scaledKickFont);
            DrawText("Kick", static_cast<int>(kickBtn.x + (kickBtn.width - static_cast<float>(kw)) * 0.5F), static_cast<int>(kickBtn.y + Sf(7.0F)), scaledKickFont, WhiteColor);
        }
    }

    // Bottom Action Buttons
    const bool isHost = client.IsHost();
    const Rectangle resumeBtn = isHost ? Rectangle{menuX + Sf(35.0F), menuY + menuHeight - Sf(64.0F), Sf(150.0F), Sf(44.0F)}
                                       : Rectangle{menuX + Sf(35.0F), menuY + menuHeight - Sf(64.0F), Sf(220.0F), Sf(44.0F)};
    const Rectangle restartBtn{menuX + Sf(195.0F), menuY + menuHeight - Sf(64.0F), Sf(170.0F), Sf(44.0F)};
    const Rectangle leaveBtn = isHost ? Rectangle{menuX + Sf(375.0F), menuY + menuHeight - Sf(64.0F), Sf(150.0F), Sf(44.0F)}
                                      : Rectangle{menuX + menuWidth - Sf(255.0F), menuY + menuHeight - Sf(64.0F), Sf(220.0F), Sf(44.0F)};

    const int scaledBtnFont = Si(18);

    const bool resHov = CheckCollisionPointRec(GetMousePosition(), resumeBtn);
    DrawRectangleRounded(resumeBtn, 0.08F, 8, resHov ? Color{65, 145, 120, 255} : Color{50, 120, 100, 255});
    const int rw = MeasureText("Resume (Esc)", scaledBtnFont);
    DrawText("Resume (Esc)", static_cast<int>(resumeBtn.x + (resumeBtn.width - static_cast<float>(rw)) * 0.5F), static_cast<int>(resumeBtn.y + Sf(12.0F)), scaledBtnFont, WhiteColor);

    if (isHost) {
        const bool restHov = CheckCollisionPointRec(GetMousePosition(), restartBtn);
        DrawRectangleRounded(restartBtn, 0.08F, 8, restHov ? Color{220, 150, 40, 255} : Color{175, 115, 30, 255});
        const int restw = MeasureText("Restart Game", scaledBtnFont);
        DrawText("Restart Game", static_cast<int>(restartBtn.x + (restartBtn.width - static_cast<float>(restw)) * 0.5F), static_cast<int>(restartBtn.y + Sf(12.0F)), scaledBtnFont, WhiteColor);
    }

    const bool levHov = CheckCollisionPointRec(GetMousePosition(), leaveBtn);
    DrawRectangleRounded(leaveBtn, 0.08F, 8, levHov ? Color{200, 55, 55, 255} : Color{160, 42, 42, 255});
    const int lw = MeasureText("Leave Room", scaledBtnFont);
    DrawText("Leave Room", static_cast<int>(leaveBtn.x + (leaveBtn.width - static_cast<float>(lw)) * 0.5F), static_cast<int>(leaveBtn.y + Sf(12.0F)), scaledBtnFont, WhiteColor);
}

void GameplayScreen::DrawConfirmKickModal() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 175});
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(440.0F)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(210.0F)) * 0.5F,
        Sf(440.0F),
        Sf(210.0F)
    };
    DrawRectangleRounded(modal, 0.05F, 12, Color{28, 33, 44, 252});
    DrawRectangleLinesEx(modal, 2.0F, RedColor);

    std::string kickTargetName = "Player " + std::to_string(pendingKickPlayerId);
    for (std::size_t i = 0; i < playerInterp.size(); ++i) {
        if (playerInterp[i].id == pendingKickPlayerId) {
            // Check snapshot player name
        }
    }
    DrawText("Kick Player?", static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(24.0F)), Si(22), RedColor);
    DrawText(TextFormat("Are you sure you want to kick Player %u?", pendingKickPlayerId),
        static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(64.0F)), Si(17), WhiteColor);
    DrawText("They will be disconnected from the room.",
        static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(92.0F)), Si(14), MutedColor);

    const Rectangle kickBtn{modal.x + Sf(35.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
    const Rectangle cancelBtn{modal.x + Sf(245.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
    const bool kickHov = CheckCollisionPointRec(GetMousePosition(), kickBtn);
    const bool cancelHov = CheckCollisionPointRec(GetMousePosition(), cancelBtn);
    if (kickHov || cancelHov) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    DrawRectangleRounded(kickBtn, 0.08F, 8, kickHov ? Color{220, 60, 60, 255} : Color{180, 45, 45, 255});
    const int scaledModalBtnFont = Si(16);
    const int kw = MeasureText("Confirm Kick", scaledModalBtnFont);
    DrawText("Confirm Kick", static_cast<int>(kickBtn.x + (kickBtn.width - static_cast<float>(kw)) * 0.5F), static_cast<int>(kickBtn.y + Sf(12.0F)), scaledModalBtnFont, WhiteColor);

    DrawRectangleRounded(cancelBtn, 0.08F, 8, cancelHov ? Color{75, 88, 105, 255} : Color{55, 66, 80, 255});
    const int cw = MeasureText("Cancel (Esc)", scaledModalBtnFont);
    DrawText("Cancel (Esc)", static_cast<int>(cancelBtn.x + (cancelBtn.width - static_cast<float>(cw)) * 0.5F), static_cast<int>(cancelBtn.y + Sf(12.0F)), scaledModalBtnFont, WhiteColor);
}

void GameplayScreen::DrawConfirmLeaveModal() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 175});
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(440.0F)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(210.0F)) * 0.5F,
        Sf(440.0F),
        Sf(210.0F)
    };
    DrawRectangleRounded(modal, 0.05F, 12, Color{28, 33, 44, 252});
    DrawRectangleLinesEx(modal, 2.0F, Color{220, 80, 80, 255});

    DrawText("Leave Room?", static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(24.0F)), Si(22), Color{220, 80, 80, 255});
    DrawText("Are you sure you want to leave the room?",
        static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(64.0F)), Si(17), WhiteColor);
    DrawText("You will disconnect and return to the main menu.",
        static_cast<int>(modal.x + Sf(35.0F)), static_cast<int>(modal.y + Sf(92.0F)), Si(14), MutedColor);

    const Rectangle leaveBtn{modal.x + Sf(35.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
    const Rectangle cancelBtn{modal.x + Sf(245.0F), modal.y + Sf(140.0F), Sf(160.0F), Sf(42.0F)};
    const bool leaveHov = CheckCollisionPointRec(GetMousePosition(), leaveBtn);
    const bool cancelHov = CheckCollisionPointRec(GetMousePosition(), cancelBtn);
    if (leaveHov || cancelHov) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    DrawRectangleRounded(leaveBtn, 0.08F, 8, leaveHov ? Color{220, 60, 60, 255} : Color{180, 45, 45, 255});
    const int scaledLeaveBtnFont = Si(16);
    const int lw = MeasureText("Leave Room", scaledLeaveBtnFont);
    DrawText("Leave Room", static_cast<int>(leaveBtn.x + (leaveBtn.width - static_cast<float>(lw)) * 0.5F), static_cast<int>(leaveBtn.y + Sf(12.0F)), scaledLeaveBtnFont, WhiteColor);

    DrawRectangleRounded(cancelBtn, 0.08F, 8, cancelHov ? Color{75, 88, 105, 255} : Color{55, 66, 80, 255});
    const int cw = MeasureText("Cancel (Esc)", scaledLeaveBtnFont);
    DrawText("Cancel (Esc)", static_cast<int>(cancelBtn.x + (cancelBtn.width - static_cast<float>(cw)) * 0.5F), static_cast<int>(cancelBtn.y + Sf(12.0F)), scaledLeaveBtnFont, WhiteColor);
}

void GameplayScreen::DrawScoreboard(const SnapshotPacket& snapshot, const PlayerSnapshot& local, const GameClient& client) const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 135});
    const float sbWidth = Sf(720.0F);
    const float sbHeight = Sf(440.0F);
    const float sbX = (static_cast<float>(GetScreenWidth()) - sbWidth) * 0.5F;
    const float sbY = (static_cast<float>(GetScreenHeight()) - sbHeight) * 0.5F;
    const Rectangle sbRect{sbX, sbY, sbWidth, sbHeight};

    DrawRectangleRounded(sbRect, 0.04F, 12, Color{20, 26, 36, 248});
    DrawRectangleLinesEx(sbRect, 2.0F, Color{54, 68, 86, 255});

    // Header
    DrawText("SCOREBOARD", static_cast<int>(sbX + Sf(35.0F)), static_cast<int>(sbY + Sf(22.0F)), Si(24), WhiteColor);
    const std::string roomCode = client.RoomCode().empty() ? "" : "Room: " + client.RoomCode() + "  |  ";
    const std::string subtitle = roomCode + ModeText(snapshot.mode) + "  |  " + MapName(snapshot.mapIndex);
    DrawText(subtitle.c_str(), static_cast<int>(sbX + Sf(35.0F)), static_cast<int>(sbY + Sf(54.0F)), Si(15), MutedColor);

    const bool isTimed = snapshot.mode == GameMode::FfaTimed || snapshot.mode == GameMode::TdmTimed;
    if (snapshot.matchRunning && isTimed) {
        const char* timeText = TextFormat("Time: %02d:%02d", static_cast<int>(snapshot.timeRemaining) / 60, static_cast<int>(snapshot.timeRemaining) % 60);
        const int scaledTimeFont = Si(16);
        const int tw = MeasureText(timeText, scaledTimeFont);
        DrawText(timeText, static_cast<int>(sbX + sbWidth - Sf(35.0F) - static_cast<float>(tw)), static_cast<int>(sbY + Sf(26.0F)), scaledTimeFont, Color{100, 200, 255, 255});
    }

    DrawLineEx(Vector2{sbX + Sf(35.0F), sbY + Sf(78.0F)}, Vector2{sbX + sbWidth - Sf(35.0F), sbY + Sf(78.0F)}, 1.5F, Color{45, 56, 72, 255});

    const float tableX = sbX + Sf(35.0F);
    float currentY = sbY + Sf(90.0F);

    if (IsTeamMode(snapshot.mode)) {
        // TDM Table: Red Team section, then Blue Team section
        const auto drawTeamSection = [&](TeamId team, const char* teamTitle, Color teamHeaderColor, int score) {
            DrawText(TextFormat("%s  -  Score: %d", teamTitle, score), static_cast<int>(tableX), static_cast<int>(currentY), Si(16), teamHeaderColor);
            currentY += Sf(24.0F);

            DrawRectangle(static_cast<int>(tableX), static_cast<int>(currentY), static_cast<int>(sbWidth - Sf(70.0F)), static_cast<int>(Sf(24.0F)), Color{28, 35, 48, 255});
            DrawText("PLAYER", static_cast<int>(tableX + Sf(10.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            DrawText("CLASS", static_cast<int>(tableX + Sf(180.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            DrawText("KILLS", static_cast<int>(tableX + Sf(300.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            DrawText("DEATHS", static_cast<int>(tableX + Sf(380.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            DrawText("K/D", static_cast<int>(tableX + Sf(470.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            DrawText("STATUS", static_cast<int>(tableX + Sf(560.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
            currentY += Sf(26.0F);

            int count = 0;
            for (std::size_t i = 0; i < snapshot.playerCount && i < cfg::MaxPlayers; ++i) {
                const PlayerSnapshot& p = snapshot.players[i];
                if (p.team != team) {
                    continue;
                }
                ++count;
                const bool isLocal = (p.id == local.id);
                const Rectangle rowRect{tableX, currentY, sbWidth - Sf(70.0F), Sf(28.0F)};
                if (isLocal) {
                    DrawRectangleRounded(rowRect, 0.1F, 4, Color{60, 55, 30, 200});
                    DrawRectangleLinesEx(rowRect, 1.0F, Color{215, 180, 50, 200});
                } else {
                    DrawRectangleRounded(rowRect, 0.1F, 4, Color{26, 32, 44, 180});
                }

                const float kd = p.deaths == 0 ? static_cast<float>(p.kills) : static_cast<float>(p.kills) / static_cast<float>(p.deaths);
                const char* youTag = isLocal ? " (You)" : "";
                const std::string nameStr = PlayerDisplayName(p);
                DrawText(TextFormat("%s%s", nameStr.c_str(), youTag), static_cast<int>(tableX + Sf(10.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), isLocal ? Color{255, 225, 100, 255} : WhiteColor);
                DrawText(ClassName(p.playerClass), static_cast<int>(tableX + Sf(180.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), WhiteColor);
                DrawText(TextFormat("%d", p.kills), static_cast<int>(tableX + Sf(300.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), WhiteColor);
                DrawText(TextFormat("%d", p.deaths), static_cast<int>(tableX + Sf(380.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), WhiteColor);
                DrawText(TextFormat("%.2f", kd), static_cast<int>(tableX + Sf(470.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), WhiteColor);
                DrawText(p.connected ? "Online" : "Offline", static_cast<int>(tableX + Sf(560.0F)), static_cast<int>(currentY + Sf(6.0F)), Si(14), p.connected ? Color{80, 220, 120, 255} : Color{255, 200, 70, 255});

                currentY += Sf(32.0F);
            }
            if (count == 0) {
                DrawText("(No players)", static_cast<int>(tableX + Sf(10.0F)), static_cast<int>(currentY + Sf(4.0F)), Si(13), MutedColor);
                currentY += Sf(24.0F);
            }
            currentY += Sf(8.0F);
        };

        drawTeamSection(TeamId::Red, "RED TEAM", RedColor, snapshot.redScore);
        drawTeamSection(TeamId::Blue, "BLUE TEAM", BlueColor, snapshot.blueScore);
    } else {
        // FFA Table
        DrawRectangle(static_cast<int>(tableX), static_cast<int>(currentY), static_cast<int>(sbWidth - Sf(70.0F)), static_cast<int>(Sf(24.0F)), Color{28, 35, 48, 255});
        DrawText("RANK", static_cast<int>(tableX + Sf(10.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("PLAYER", static_cast<int>(tableX + Sf(70.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("CLASS", static_cast<int>(tableX + Sf(220.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("KILLS", static_cast<int>(tableX + Sf(340.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("DEATHS", static_cast<int>(tableX + Sf(420.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("K/D", static_cast<int>(tableX + Sf(500.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        DrawText("STATUS", static_cast<int>(tableX + Sf(580.0F)), static_cast<int>(currentY + Sf(5.0F)), Si(13), MutedColor);
        currentY += Sf(30.0F);

        std::vector<PlayerSnapshot> sortedPlayers;
        for (std::size_t i = 0; i < snapshot.playerCount && i < cfg::MaxPlayers; ++i) {
            sortedPlayers.push_back(snapshot.players[i]);
        }
        std::sort(sortedPlayers.begin(), sortedPlayers.end(), [](const PlayerSnapshot& a, const PlayerSnapshot& b) {
            if (a.kills != b.kills) { return a.kills > b.kills; }
            return a.deaths < b.deaths;
        });

        for (std::size_t i = 0; i < sortedPlayers.size(); ++i) {
            const PlayerSnapshot& p = sortedPlayers[i];
            const bool isLocal = (p.id == local.id);
            const Rectangle rowRect{tableX, currentY, sbWidth - Sf(70.0F), Sf(32.0F)};
            if (isLocal) {
                DrawRectangleRounded(rowRect, 0.1F, 4, Color{60, 55, 30, 200});
                DrawRectangleLinesEx(rowRect, 1.0F, Color{215, 180, 50, 200});
            } else {
                DrawRectangleRounded(rowRect, 0.1F, 4, Color{26, 32, 44, 180});
            }

            const float kd = p.deaths == 0 ? static_cast<float>(p.kills) : static_cast<float>(p.kills) / static_cast<float>(p.deaths);
            const char* youTag = isLocal ? " (You)" : "";
            const std::string nameStr = PlayerDisplayName(p);
            DrawText(TextFormat("#%d", static_cast<int>(i + 1)), static_cast<int>(tableX + Sf(10.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), WhiteColor);
            DrawText(TextFormat("%s%s", nameStr.c_str(), youTag), static_cast<int>(tableX + Sf(70.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), isLocal ? Color{255, 225, 100, 255} : WhiteColor);
            DrawText(ClassName(p.playerClass), static_cast<int>(tableX + Sf(220.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), WhiteColor);
            DrawText(TextFormat("%d", p.kills), static_cast<int>(tableX + Sf(340.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), WhiteColor);
            DrawText(TextFormat("%d", p.deaths), static_cast<int>(tableX + Sf(420.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), WhiteColor);
            DrawText(TextFormat("%.2f", kd), static_cast<int>(tableX + Sf(500.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), WhiteColor);
            DrawText(p.connected ? "Online" : "Offline", static_cast<int>(tableX + Sf(580.0F)), static_cast<int>(currentY + Sf(7.0F)), Si(14), p.connected ? Color{80, 220, 120, 255} : Color{255, 200, 70, 255});

            currentY += Sf(38.0F);
        }
    }

    constexpr const char* hint = "Hold TAB to view scoreboard";
    const int scaledHintFont = Si(13);
    const int hw = MeasureText(hint, scaledHintFont);
    DrawText(hint, static_cast<int>(sbX + (sbWidth - static_cast<float>(hw)) * 0.5F), static_cast<int>(sbY + sbHeight - Sf(24.0F)), scaledHintFont, MutedColor);
}

void GameplayScreen::DrawHotbar(const PlayerSnapshot& local, const GameClient& client) const {
    const std::string barrierCount = std::to_string(localPredictedBarriers);
    const Color barrierColor = (localPredictedBarriers == 0) ? Color{220, 80, 80, 255} : WhiteColor;

    const std::string grenadeCount = std::to_string(localPredictedGrenades);
    const Color grenadeColor = (localPredictedGrenades == 0) ? Color{220, 80, 80, 255} : WhiteColor;

    const bool isReloading = localReloadRemaining > 0.0F;
    const std::string bulletCount = isReloading ? "0" : std::to_string(local.ammo);
    const Color bulletColor = (local.ammo == 0 || isReloading) ? Color{220, 80, 80, 255} : WhiteColor;

    struct HotbarItem {
        const Texture2D* texture;
        std::string countStr;
        Color countColor;
        bool isZero;
        float itemWidth;
    };

    const int fontSize = Si(18);
    const float iconSize = Sf(22.0F);
    const float iconTextGap = Sf(8.0F);
    const float sectionGap = Sf(24.0F);
    const float padX = Sf(18.0F);
    const float totalH = Sf(38.0F);

    std::array<HotbarItem, 3> items = {{
        {&client.Assets().BarrierIcon(), barrierCount, barrierColor, localPredictedBarriers == 0, 0.0F},
        {&client.Assets().GrenadeIcon(), grenadeCount, grenadeColor, localPredictedGrenades == 0, 0.0F},
        {&client.Assets().BulletIcon(), bulletCount, bulletColor, (local.ammo == 0 || isReloading), 0.0F}
    }};

    float itemsWidth = 0.0F;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const float textW = static_cast<float>(MeasureText(items[i].countStr.c_str(), fontSize));
        items[i].itemWidth = iconSize + iconTextGap + textW;
        itemsWidth += items[i].itemWidth;
    }

    const float totalW = itemsWidth + sectionGap * static_cast<float>(items.size() - 1) + padX * 2.0F;
    const float startX = (static_cast<float>(GetScreenWidth()) - totalW) * 0.5F;
    const float startY = static_cast<float>(GetScreenHeight()) - totalH - Sf(16.0F);

    // 1. Health bar just above hotbar for local player
    if (local.alive && local.id != 0) {
        const float hpBarH = Sf(6.0F);
        const float hpBarY = startY - hpBarH - Sf(6.0F);
        const Rectangle hpBg{startX, hpBarY, totalW, hpBarH};
        DrawRectangleRounded(hpBg, 0.4F, 4, Color{18, 22, 28, 220});
        DrawRectangleLinesEx(hpBg, 1.0F, Color{45, 56, 72, 180});

        const float hpRatio = std::clamp(local.health / cfg::PlayerMaxHealth, 0.0F, 1.0F);
        if (hpRatio > 0.0F) {
            const Rectangle hpFill{startX, hpBarY, totalW * hpRatio, hpBarH};
            const Color hpColor = (hpRatio > 0.5F) ? Color{80, 220, 120, 255} : (hpRatio > 0.25F ? Color{240, 200, 60, 255} : Color{230, 70, 70, 255});
            DrawRectangleRounded(hpFill, 0.4F, 4, hpColor);
        }
    }

    // 2. Continuous dark hotbar background (no separators)
    const Rectangle barRect{startX, startY, totalW, totalH};
    DrawRectangleRounded(barRect, 0.28F, 8, Color{16, 20, 26, 225});
    DrawRectangleLinesEx(barRect, 1.5F, Color{45, 56, 72, 200});

    // 3. Draw items horizontally
    float curX = startX + padX;
    const float centerY = startY + totalH * 0.5F;

    for (std::size_t i = 0; i < items.size(); ++i) {
        const HotbarItem& item = items[i];

        // Draw Icon
        if (item.texture != nullptr && item.texture->id != 0 && item.texture->width > 0 && item.texture->height > 0) {
            const float scale = std::min(iconSize / static_cast<float>(item.texture->width), iconSize / static_cast<float>(item.texture->height));
            const float iconW = static_cast<float>(item.texture->width) * scale;
            const float iconH = static_cast<float>(item.texture->height) * scale;
            const Rectangle src{0.0F, 0.0F, static_cast<float>(item.texture->width), static_cast<float>(item.texture->height)};
            const Rectangle dst{
                curX + (iconSize - iconW) * 0.5F,
                centerY - iconH * 0.5F,
                iconW,
                iconH
            };
            const Color iconTint = WHITE;
            DrawTexturePro(*item.texture, src, dst, Vector2{0.0F, 0.0F}, 0.0F, iconTint);
        }

        // Draw Count Number
        const float textX = curX + iconSize + iconTextGap;
        const float textY = centerY - static_cast<float>(fontSize) * 0.5F;
        DrawText(item.countStr.c_str(), static_cast<int>(textX), static_cast<int>(textY), fontSize, item.countColor);

        curX += item.itemWidth + sectionGap;
    }
}


