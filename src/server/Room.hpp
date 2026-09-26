#pragma once

#include "common/Protocol.hpp"
#include "common/Vec2.hpp"

#include <string>
#include <vector>

class Room {
public:
    Room();

    std::uint32_t AddOrFindPlayer(const std::string& addressKey, const std::string& requestedName = "");
    void RemovePlayer(std::uint32_t playerId);
    bool MarkPlayerDisconnected(std::uint32_t playerId);
    bool TryReconnectPlayer(const std::string& addressKey, std::uint32_t playerId = 0);
    void KickPlayer(std::uint32_t playerId);
    bool IsPlayerConnected(std::uint32_t playerId) const;
    bool IsPlayerHost(std::uint32_t playerId) const;
    std::string GetPlayerName(std::uint32_t playerId) const;
    std::size_t PlayerCount() const;
    void Configure(std::uint32_t playerId, const LobbyConfigPacket& packet);
    std::string TryStart(std::uint32_t playerId);
    void ApplyInput(std::uint32_t playerId, const InputPacket& packet);
    void Update(float dt);
    SnapshotPacket MakeSnapshot(std::uint32_t playerId) const;

private:
    struct PlayerState {
        std::uint32_t id;
        std::string name;
        std::string addressKey;
        Vec2 position;
        Vec2 aim;
        float health;
        float cooldown;
        float reloadRemaining;
        float respawnRemaining;
        float dashCooldown;
        float dashRemaining;
        Vec2 dashDirection;
        Vec2 grenadeTarget;
        std::int16_t kills;
        std::int16_t deaths;
        std::uint8_t ammo;
        std::uint16_t inputFlags;
        std::uint16_t previousInputFlags;
        PlayerClass playerClass;
        TeamId team;
        bool alive;
        bool host;
        std::uint8_t grenades;
        std::uint8_t barriers;
        bool connected;
        float reconnectRemaining;
        float invulnerableRemaining;
    };

    struct BulletState {
        std::uint32_t ownerId;
        Vec2 position;
        Vec2 velocity;
        float life;
        float damage;
        KillFeedWeapon weapon;
    };

    struct GrenadeState {
        std::uint32_t ownerId;
        Vec2 position;
        Vec2 target;
        Vec2 velocity;
        float fuse;
        float flightRemaining;
        float flightDuration;
    };

    struct BarrierState {
        Vec2 position;
        float rotation;
        float health;
        std::uint32_t ownerId;
    };

    struct ObstacleState {
        Vec2 position;
        Vec2 size;
    };

    PlayerState* FindPlayer(std::uint32_t playerId);
    const PlayerState* FindPlayer(std::uint32_t playerId) const;
    void ResetMatch();
    void Fire(PlayerState& player);
    void LaunchGrenade(PlayerState& player);
    void PlaceBarrier(PlayerState& player);
    void DamagePlayer(PlayerState& target, PlayerState* attacker, float damage, KillFeedWeapon weapon);
    void Respawn(PlayerState& player);
    bool IsTeamMode() const;
    bool IsBlocked(const Vec2& position) const;
    bool IntersectsObstacle(const Vec2& position, float radius) const;
    void BuildMap();
    Vec2 SpawnPoint(std::size_t index) const;
    Vec2 FurthestCornerSpawn(std::uint32_t playerId) const;
    void CheckWinConditions();
    void PushKillFeed(const KillFeedSnapshot& event);

    std::vector<PlayerState> players;
    std::vector<BulletState> bullets;
    std::vector<GrenadeState> grenades;
    std::vector<BarrierState> barriers;
    std::vector<ObstacleState> obstacles;
    std::vector<KillFeedSnapshot> recentKills;
    std::uint32_t nextPlayerId;
    std::uint32_t nextKillEventId;
    std::uint32_t winnerPlayerId;
    TeamId winnerTeam;
    bool matchEndedByForfeit;
    GameMode mode;
    std::uint8_t mapIndex;
    std::uint16_t scoreLimit;
    float matchSeconds;
    float remainingSeconds;
    float countdownRemaining;
    bool matchRunning;
};
