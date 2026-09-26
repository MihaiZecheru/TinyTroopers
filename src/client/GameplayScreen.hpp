#pragma once

#include "common/Protocol.hpp"

#include "raylib.h"

#include <array>
#include <vector>

class GameClient;

class GameplayScreen {
public:
    GameplayScreen();
    ~GameplayScreen();
    void Reset();
    void Update(GameClient& client);
    void Draw(GameClient& client) const;

private:
    struct KillFeedItem {
        std::uint32_t eventId;
        std::uint32_t killerId;
        std::uint32_t victimId;
        TeamId killerTeam;
        TeamId victimTeam;
        KillFeedWeapon weapon;
        bool isSuicide;
        float timeRemaining;
    };

    struct PlayerInterp {
        std::uint32_t id = 0;
        Vector2 currentPos{0.0F, 0.0F};
        Vector2 targetPos{0.0F, 0.0F};
        float currentRotation = 0.0F;
        float targetRotation = 0.0F;
        bool active = false;
    };

    std::uint32_t sequence;
    float sendAccumulator;
    std::array<float, 4> previousHealth;
    std::array<std::uint8_t, cfg::MaxPlayers> previousAmmo;
    float localFireCooldown;
    float localReloadRemaining;
    std::uint8_t localPredictedAmmo;
    std::uint8_t lastServerAmmo;
    std::uint8_t localPredictedGrenades;
    std::uint8_t lastServerGrenades;
    std::uint8_t localPredictedBarriers;
    std::uint8_t lastServerBarriers;
    bool grenadeQueued;
    bool barrierQueued;
    bool respawnQueued;
    bool dashQueued;
    bool previousMatchRunning;
    bool endModalOpen;
    Vector2 grenadeTarget;
    SnapshotPacket endSnapshot;
    float aimProgress;
    bool killfeedCollapsed;
    std::vector<KillFeedItem> killFeedItems;
    std::uint32_t lastKillEventId;
    bool escapeMenuOpen;
    bool confirmLeaveOpen;
    std::uint32_t pendingKickPlayerId;
    float fireBlockTimer;
    bool wasAlive;
    float localRespawnTimer;
    std::array<PlayerInterp, cfg::MaxPlayers> playerInterp;
    Vector2 localInterpPos;

    mutable RenderTexture2D mapTexture;
    mutable int cachedMapIndex;

    Camera2D MakeCamera(const PlayerSnapshot& local, Vector2 localPos) const;
    void EnsureMapTexture(std::uint8_t mapIndex) const;
    void DrawMap(std::uint8_t mapIndex) const;
    void UpdateInterpolation(const SnapshotPacket& snapshot, float dt, bool newSnapshot);
    void UpdateEndModal();
    void DrawEndModal() const;
    void UpdateRespawnModal(GameClient& client, const PlayerSnapshot& local);
    void DrawRespawnModal(GameClient& client, const PlayerSnapshot& local) const;
    Rectangle KillFeedChevronBounds() const;
    void UpdateKillFeed(const SnapshotPacket& snapshot, float dt);
    void DrawKillFeed(const PlayerSnapshot& local, const SnapshotPacket& snapshot) const;
    void UpdateEscapeMenu(GameClient& client);
    void DrawEscapeMenu(GameClient& client) const;
    void DrawScoreboard(const SnapshotPacket& snapshot, const PlayerSnapshot& local, const GameClient& client) const;
    void DrawHotbar(const PlayerSnapshot& local, const GameClient& client) const;
    void DrawConfirmLeaveModal() const;
    void DrawConfirmKickModal() const;
};
