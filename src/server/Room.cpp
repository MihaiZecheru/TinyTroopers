#include "server/Room.hpp"

#include "common/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>

namespace {
constexpr float AkFireSeconds = 0.105F;
constexpr float PistolFireSeconds = 0.0F;
constexpr float SniperFireSeconds = 1.25F;
constexpr float ShotgunFireSeconds = 0.62F;
constexpr float AkBulletSpeed = 2100.0F * 1.40F;
constexpr float PistolBulletSpeed = 2100.0F * 1.40F;
constexpr float SniperBulletSpeed = 4300.0F;
constexpr float ShotgunBulletSpeed = 2100.0F;
constexpr float AkDamage = 18.0F;
constexpr float PistolDamage = 14.0F;
constexpr float SniperDamage = cfg::PlayerMaxHealth;
constexpr float ShotgunPelletDamage = 17.0F;
constexpr float AkMoveSpeed = 418.75F;
constexpr float PistolMoveSpeed = 481.25F;
constexpr float SniperMoveSpeed = cfg::PlayerSpeed;
constexpr float ShotgunMoveSpeed = 462.5F;
constexpr float AkRangeSeconds = 1.17F / 1.30F;
constexpr float PistolRangeSeconds = (AkBulletSpeed * AkRangeSeconds) / PistolBulletSpeed;
constexpr float SniperRangeSeconds = 1.725F / 1.30F;
constexpr float ShotgunRangeSeconds = (0.51F / 1.30F) * 0.5F;
constexpr int ShotgunPellets = 6;
constexpr float ShotgunSpread = 0.28F;
constexpr float Pi = 3.14159265F;
constexpr float MapMargin = 90.0F;



float ClassSpeed(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return AkBulletSpeed;
    case PlayerClass::Pistol: return PistolBulletSpeed;
    case PlayerClass::Sniper: return SniperBulletSpeed;
    case PlayerClass::Shotgun: return ShotgunBulletSpeed;
    }
    return AkBulletSpeed;
}

float ClassDamage(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return AkDamage;
    case PlayerClass::Pistol: return PistolDamage;
    case PlayerClass::Sniper: return SniperDamage;
    case PlayerClass::Shotgun: return ShotgunPelletDamage;
    }
    return AkDamage;
}

float ClassMoveSpeed(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return AkMoveSpeed;
    case PlayerClass::Pistol: return PistolMoveSpeed;
    case PlayerClass::Sniper: return SniperMoveSpeed;
    case PlayerClass::Shotgun: return ShotgunMoveSpeed;
    }
    return SniperMoveSpeed;
}

float ClassLife(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return AkRangeSeconds;
    case PlayerClass::Pistol: return PistolRangeSeconds;
    case PlayerClass::Sniper: return SniperRangeSeconds;
    case PlayerClass::Shotgun: return ShotgunRangeSeconds;
    }
    return AkRangeSeconds;
}

std::uint8_t ClassAmmo(PlayerClass playerClass) {
    return ClassMaxAmmo(playerClass);
}

KillFeedWeapon ToKillFeedWeapon(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return KillFeedWeapon::Ak47;
    case PlayerClass::Pistol: return KillFeedWeapon::Pistol;
    case PlayerClass::Sniper: return KillFeedWeapon::Sniper;
    case PlayerClass::Shotgun: return KillFeedWeapon::Shotgun;
    }
    return KillFeedWeapon::Ak47;
}

float Distance(const Vec2& a, const Vec2& b) {
    return (a - b).Length();
}

float DistanceToSegment(const Vec2& point, const Vec2& start, const Vec2& end) {
    const Vec2 segment = end - start;
    const float lengthSquared = (segment.x * segment.x) + (segment.y * segment.y);
    if (lengthSquared <= 0.0F) {
        return Distance(point, start);
    }
    const Vec2 pointOffset = point - start;
    const float t = std::clamp(((pointOffset.x * segment.x) + (pointOffset.y * segment.y)) / lengthSquared, 0.0F, 1.0F);
    return Distance(point, start + (segment * t));
}

bool SegmentIntersectsRectWithT(const Vec2& start, const Vec2& end, float left, float top, float right, float bottom, float& outT) {
    float tMin = 0.0F;
    float tMax = 1.0F;
    const Vec2 delta = end - start;

    auto clip = [&](float p, float q) {
        if (p == 0.0F) {
            return q >= 0.0F;
        }
        const float r = q / p;
        if (p < 0.0F) {
            if (r > tMax) { return false; }
            if (r > tMin) { tMin = r; }
        } else {
            if (r < tMin) { return false; }
            if (r < tMax) { tMax = r; }
        }
        return true;
    };

    if (clip(-delta.x, start.x - left) &&
        clip(delta.x, right - start.x) &&
        clip(-delta.y, start.y - top) &&
        clip(delta.y, bottom - start.y)) {
        outT = tMin;
        return true;
    }
    return false;
}

bool SegmentIntersectsRect(const Vec2& start, const Vec2& end, float left, float top, float right, float bottom) {
    float t = 0.0F;
    return SegmentIntersectsRectWithT(start, end, left, top, right, bottom, t);
}

bool BulletIntersectsBarrier(const Vec2& start, const Vec2& end, const Vec2& barrierPos, float barrierRot, float& outT) {
    const float cosR = std::cos(-barrierRot);
    const float sinR = std::sin(-barrierRot);
    const Vec2 dStart = start - barrierPos;
    const Vec2 dEnd = end - barrierPos;
    const Vec2 locStart(dStart.x * cosR - dStart.y * sinR, dStart.x * sinR + dStart.y * cosR);
    const Vec2 locEnd(dEnd.x * cosR - dEnd.y * sinR, dEnd.x * sinR + dEnd.y * cosR);

    const float halfW = cfg::BarrierWidth * 0.5F + cfg::BulletRadius;
    const float halfH = cfg::BarrierHeight * 0.5F + cfg::BulletRadius;

    return SegmentIntersectsRectWithT(locStart, locEnd, -halfW, -halfH, halfW, halfH, outT);
}
}

Room::Room()
    : nextPlayerId(1),
      nextKillEventId(0),
      winnerPlayerId(0),
      winnerTeam(TeamId::None),
      matchEndedByForfeit(false),
      mode(GameMode::FfaTimed),
      mapIndex(0),
      scoreLimit(cfg::DefaultScoreLimit),
      matchSeconds(cfg::DefaultMatchMinutes * 60.0F),
      remainingSeconds(matchSeconds),
      countdownRemaining(0.0F),
      matchRunning(false) {
    BuildMap();
}

std::uint32_t Room::AddOrFindPlayer(const std::string& addressKey, const std::string& requestedName) {
    for (PlayerState& player : players) {
        if (player.addressKey == addressKey) {
            if (!requestedName.empty()) {
                player.name = requestedName;
            }
            if (!player.connected) {
                player.connected = true;
                player.reconnectRemaining = 0.0F;
                KillFeedSnapshot event{};
                event.eventId = ++nextKillEventId;
                event.victimId = player.id;
                event.victimTeam = player.team;
                event.weapon = KillFeedWeapon::Reconnected;
                PushKillFeed(event);
            }
            return player.id;
        }
    }
    // Check if an existing disconnected player can be reclaimed (e.g. client reconnected on a new port)
    for (PlayerState& player : players) {
        if (!player.connected) {
            player.addressKey = addressKey;
            if (!requestedName.empty()) {
                player.name = requestedName;
            }
            player.connected = true;
            player.reconnectRemaining = 0.0F;
            KillFeedSnapshot event{};
            event.eventId = ++nextKillEventId;
            event.victimId = player.id;
            event.victimTeam = player.team;
            event.weapon = KillFeedWeapon::Reconnected;
            PushKillFeed(event);
            return player.id;
        }
    }
    if (players.size() >= cfg::MaxPlayers) {
        return 0;
    }
    const std::uint32_t id = nextPlayerId++;
    const std::string finalName = requestedName.empty() ? ("Player " + std::to_string(id)) : requestedName;
    const PlayerClass playerClass = PlayerClass::Ak47;
    const bool host = players.empty();
    const Vec2 spawn = FurthestCornerSpawn(id);
    players.push_back(PlayerState{id, finalName, addressKey, spawn, Vec2(1.0F, 0.0F),
        cfg::PlayerMaxHealth, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, Vec2(1.0F, 0.0F), spawn, 0, 0, ClassAmmo(playerClass), 0, 0, playerClass,
        host ? TeamId::Red : TeamId::Blue, true, host, ClassMaxGrenades(playerClass), ClassMaxBarriers(playerClass), true, 0.0F, 0.0F});
    return id;
}

void Room::RemovePlayer(std::uint32_t playerId) {
    const auto existing = std::find_if(players.begin(), players.end(), [playerId](const PlayerState& player) {
        return player.id == playerId;
    });
    const bool removedHost = existing != players.end() && existing->host;
    const auto removed = std::remove_if(players.begin(), players.end(), [playerId](const PlayerState& player) {
        return player.id == playerId;
    });
    players.erase(removed, players.end());

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [playerId](const BulletState& bullet) {
        return bullet.ownerId == playerId;
    }), bullets.end());
    grenades.erase(std::remove_if(grenades.begin(), grenades.end(), [playerId](const GrenadeState& grenade) {
        return grenade.ownerId == playerId;
    }), grenades.end());
    barriers.erase(std::remove_if(barriers.begin(), barriers.end(), [playerId](const BarrierState& barrier) {
        return barrier.ownerId == playerId;
    }), barriers.end());

    if (removedHost && !players.empty()) {
        players.front().host = true;
    }
    CheckWinConditions();
}

bool Room::MarkPlayerDisconnected(std::uint32_t playerId) {
    PlayerState* player = FindPlayer(playerId);
    if (player == nullptr || !player->connected) {
        return false;
    }
    player->connected = false;
    player->reconnectRemaining = cfg::ReconnectWindowSeconds;
    player->inputFlags = 0;
    player->previousInputFlags = 0;

    KillFeedSnapshot event{};
    event.eventId = ++nextKillEventId;
    event.victimId = player->id;
    event.victimTeam = player->team;
    event.weapon = KillFeedWeapon::Disconnected;
    PushKillFeed(event);
    return true;
}

bool Room::TryReconnectPlayer(const std::string& addressKey, std::uint32_t playerId) {
    for (PlayerState& player : players) {
        if (!player.connected && (playerId == 0 || player.id == playerId || player.addressKey == addressKey)) {
            player.connected = true;
            player.addressKey = addressKey;
            player.reconnectRemaining = 0.0F;

            KillFeedSnapshot event{};
            event.eventId = ++nextKillEventId;
            event.victimId = player.id;
            event.victimTeam = player.team;
            event.weapon = KillFeedWeapon::Reconnected;
            PushKillFeed(event);
            return true;
        }
    }
    return false;
}

void Room::KickPlayer(std::uint32_t playerId) {
    PlayerState* player = FindPlayer(playerId);
    if (player != nullptr) {
        KillFeedSnapshot event{};
        event.eventId = ++nextKillEventId;
        event.victimId = player->id;
        event.victimTeam = player->team;
        event.weapon = KillFeedWeapon::Kicked;
        PushKillFeed(event);
    }
    RemovePlayer(playerId);
}

bool Room::IsPlayerConnected(std::uint32_t playerId) const {
    const PlayerState* player = FindPlayer(playerId);
    return player != nullptr && player->connected;
}

bool Room::IsPlayerHost(std::uint32_t playerId) const {
    const PlayerState* player = FindPlayer(playerId);
    return player != nullptr && player->host;
}

void Room::PushKillFeed(const KillFeedSnapshot& event) {
    recentKills.push_back(event);
    if (recentKills.size() > cfg::MaxRecentKills) {
        recentKills.erase(recentKills.begin());
    }
}

std::size_t Room::PlayerCount() const {
    return players.size();
}

void Room::Configure(std::uint32_t playerId, const LobbyConfigPacket& packet) {
    PlayerState* player = FindPlayer(playerId);
    if (player == nullptr) {
        return;
    }
    player->playerClass = packet.playerClass;
    player->team = packet.team;
    player->ammo = ClassAmmo(player->playerClass);
    player->grenades = ClassMaxGrenades(player->playerClass);
    player->barriers = ClassMaxBarriers(player->playerClass);
    if (matchRunning) {
        return;
    }
    if (player->host) {
        mode = packet.mode;
        mapIndex = static_cast<std::uint8_t>(packet.mapIndex % cfg::MapCount);
        scoreLimit = packet.scoreLimit == 0 ? cfg::DefaultScoreLimit : packet.scoreLimit;
        matchSeconds = std::max(30.0F, packet.minutes * 60.0F);
        remainingSeconds = matchSeconds;
        BuildMap();
    }
}

std::string Room::TryStart(std::uint32_t playerId) {
    const PlayerState* player = FindPlayer(playerId);
    if (player == nullptr || !player->host) {
        return "Only the host can start the match.";
    }
    if (players.size() < cfg::MinPlayers) {
        return "Need at least 2 players.";
    }
    if (IsTeamMode() && (players.size() % 2 != 0)) {
        return "TDM requires an even number of players.";
    }
    ResetMatch();
    matchRunning = true;
    countdownRemaining = cfg::MatchCountdownSeconds;
    return "Match started.";
}

void Room::ApplyInput(std::uint32_t playerId, const InputPacket& packet) {
    PlayerState* player = FindPlayer(playerId);
    if (player == nullptr) {
        return;
    }
    player->inputFlags = packet.flags;
    player->aim = Vec2(packet.aimX, packet.aimY).Normalized();
    player->grenadeTarget = Vec2(
        std::clamp(packet.grenadeTargetX, 0.0F, cfg::WorldWidth),
        std::clamp(packet.grenadeTargetY, 0.0F, cfg::WorldHeight));
}

void Room::Update(float dt) {
    std::vector<std::uint32_t> toKick;
    for (PlayerState& player : players) {
        if (!player.connected) {
            player.reconnectRemaining -= dt;
            if (player.reconnectRemaining <= 0.0F) {
                toKick.push_back(player.id);
            }
        }
    }
    for (std::uint32_t id : toKick) {
        KickPlayer(id);
    }

    if (!matchRunning) {
        return;
    }
    if (countdownRemaining > 0.0F) {
        countdownRemaining = std::max(0.0F, countdownRemaining - dt);
        if (countdownRemaining == 0.0F) {
            for (PlayerState& player : players) {
                player.invulnerableRemaining = cfg::SpawnProtectionSeconds;
            }
        }
        for (PlayerState& player : players) {
            player.previousInputFlags = player.inputFlags;
        }
        return;
    }
    remainingSeconds = std::max(0.0F, remainingSeconds - dt);

    for (PlayerState& player : players) {
        if (!player.connected) {
            player.inputFlags = 0;
            player.previousInputFlags = 0;
        }
        const bool wasReloading = player.reloadRemaining > 0.0F;
        player.cooldown = std::max(0.0F, player.cooldown - dt);
        player.reloadRemaining = std::max(0.0F, player.reloadRemaining - dt);
        player.dashCooldown = std::max(0.0F, player.dashCooldown - dt);
        player.dashRemaining = std::max(0.0F, player.dashRemaining - dt);
        player.invulnerableRemaining = std::max(0.0F, player.invulnerableRemaining - dt);
        if (wasReloading && player.reloadRemaining == 0.0F) {
            player.ammo = ClassAmmo(player.playerClass);
        }
        if (!player.alive) {
            player.respawnRemaining -= dt;
            if (player.respawnRemaining <= 0.0F && (player.inputFlags & InputFlags::Respawn) != 0) {
                Respawn(player);
            }
            player.previousInputFlags = player.inputFlags;
            continue;
        }

        Vec2 movement;
        if ((player.inputFlags & InputFlags::MoveUp) != 0) { movement.y -= 1.0F; }
        if ((player.inputFlags & InputFlags::MoveDown) != 0) { movement.y += 1.0F; }
        if ((player.inputFlags & InputFlags::MoveLeft) != 0) { movement.x -= 1.0F; }
        if ((player.inputFlags & InputFlags::MoveRight) != 0) { movement.x += 1.0F; }
        const Vec2 movementDirection = movement.Normalized();
        if ((player.inputFlags & InputFlags::Dash) != 0 && player.dashCooldown == 0.0F) {
            player.dashDirection = movementDirection.Length() > 0.0F ? movementDirection : player.aim;
            player.dashCooldown = cfg::DashCooldownSeconds;
            player.dashRemaining = cfg::DashSeconds;
        }
        Vec2 velocity = movementDirection * (ClassMoveSpeed(player.playerClass) * dt);
        if (player.dashRemaining > 0.0F) {
            velocity += player.dashDirection * (cfg::DashSpeed * dt);
        }
        const Vec2 horizontalNext(std::clamp(player.position.x + velocity.x, MapMargin, cfg::WorldWidth - MapMargin), player.position.y);
        if (!IsBlocked(horizontalNext)) {
            player.position.x = horizontalNext.x;
        }
        const Vec2 verticalNext(player.position.x, std::clamp(player.position.y + velocity.y, MapMargin, cfg::WorldHeight - MapMargin));
        if (!IsBlocked(verticalNext)) {
            player.position.y = verticalNext.y;
        }
        if (player.ammo == 0 && player.reloadRemaining == 0.0F) {
            player.reloadRemaining = ClassReloadSeconds(player.playerClass);
        }
        if ((player.inputFlags & InputFlags::Reload) != 0 && player.reloadRemaining == 0.0F && player.ammo < ClassAmmo(player.playerClass)) {
            player.reloadRemaining = ClassReloadSeconds(player.playerClass);
        }
        if ((player.inputFlags & InputFlags::Fire) != 0) {
            Fire(player);
        }
        if ((player.inputFlags & InputFlags::Grenade) != 0 && (player.previousInputFlags & InputFlags::Grenade) == 0) {
            LaunchGrenade(player);
        }
        if ((player.inputFlags & InputFlags::Barrier) != 0 && (player.previousInputFlags & InputFlags::Barrier) == 0) {
            PlaceBarrier(player);
        }
        player.previousInputFlags = player.inputFlags;
    }

    for (BulletState& bullet : bullets) {
        const Vec2 previousPosition = bullet.position;
        bullet.position += bullet.velocity * dt;
        bullet.life -= dt;

        float earliestHitT = 2.0F;
        enum class HitType { None, Obstacle, Barrier, Player } hitType = HitType::None;
        std::size_t hitBarrierIdx = 0;
        PlayerState* hitTargetPlayer = nullptr;

        for (const ObstacleState& obstacle : obstacles) {
            const float left = obstacle.position.x - obstacle.size.x * 0.5F - cfg::BulletRadius;
            const float right = obstacle.position.x + obstacle.size.x * 0.5F + cfg::BulletRadius;
            const float top = obstacle.position.y - obstacle.size.y * 0.5F - cfg::BulletRadius;
            const float bottom = obstacle.position.y + obstacle.size.y * 0.5F + cfg::BulletRadius;
            float tObs = 0.0F;
            if (SegmentIntersectsRectWithT(previousPosition, bullet.position, left, top, right, bottom, tObs)) {
                if (tObs < earliestHitT) {
                    earliestHitT = tObs;
                    hitType = HitType::Obstacle;
                }
            }
        }

        for (std::size_t bIdx = 0; bIdx < barriers.size(); ++bIdx) {
            BarrierState& barrier = barriers[bIdx];
            if (barrier.ownerId == bullet.ownerId) {
                continue;
            }
            float tBar = 0.0F;
            if (BulletIntersectsBarrier(previousPosition, bullet.position, barrier.position, barrier.rotation, tBar)) {
                if (tBar < earliestHitT) {
                    earliestHitT = tBar;
                    hitType = HitType::Barrier;
                    hitBarrierIdx = bIdx;
                }
            }
        }

        PlayerState* attacker = FindPlayer(bullet.ownerId);
        for (PlayerState& player : players) {
            if (!player.alive || player.id == bullet.ownerId) {
                continue;
            }
            if (attacker != nullptr && IsTeamMode() && attacker->team == player.team) {
                continue;
            }
            const Vec2 seg = bullet.position - previousPosition;
            const float segLenSq = (seg.x * seg.x) + (seg.y * seg.y);
            float tPl = 0.0F;
            if (segLenSq > 0.0F) {
                tPl = std::clamp(((player.position.x - previousPosition.x) * seg.x + (player.position.y - previousPosition.y) * seg.y) / segLenSq, 0.0F, 1.0F);
            }
            const Vec2 closestPt = previousPosition + (seg * tPl);
            if (Distance(player.position, closestPt) <= cfg::PlayerHitRadius) {
                if (tPl < earliestHitT) {
                    earliestHitT = tPl;
                    hitType = HitType::Player;
                    hitTargetPlayer = &player;
                }
            }
        }

        if (hitType == HitType::Obstacle) {
            bullet.life = 0.0F;
        } else if (hitType == HitType::Barrier) {
            barriers[hitBarrierIdx].health -= bullet.damage;
            bullet.life = 0.0F;
        } else if (hitType == HitType::Player && hitTargetPlayer != nullptr) {
            DamagePlayer(*hitTargetPlayer, attacker, bullet.damage, bullet.weapon);
            bullet.life = 0.0F;
        }
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const BulletState& bullet) {
        return bullet.life <= 0.0F;
    }), bullets.end());

    for (GrenadeState& grenade : grenades) {
        if (grenade.flightRemaining > 0.0F) {
            grenade.position += grenade.velocity * dt;
            grenade.flightRemaining = std::max(0.0F, grenade.flightRemaining - dt);
            if (grenade.flightRemaining == 0.0F) {
                grenade.position = grenade.target;
                grenade.velocity = Vec2();
                grenade.fuse = cfg::GrenadeFuseSeconds;
            }
            continue;
        }
        grenade.fuse -= dt;
        if (grenade.fuse > 0.0F) {
            continue;
        }
        PlayerState* attacker = FindPlayer(grenade.ownerId);
        for (PlayerState& player : players) {
            if (player.alive && Distance(player.position, grenade.position) <= cfg::GrenadeExplosionRadius) {
                DamagePlayer(player, attacker, cfg::GrenadeDamage, KillFeedWeapon::Grenade);
            }
        }
    }
    grenades.erase(std::remove_if(grenades.begin(), grenades.end(), [](const GrenadeState& grenade) {
        return grenade.flightRemaining <= 0.0F && grenade.fuse <= 0.0F;
    }), grenades.end());
    barriers.erase(std::remove_if(barriers.begin(), barriers.end(), [](const BarrierState& barrier) {
        return barrier.health <= 0.0F;
    }), barriers.end());

    CheckWinConditions();
}

void Room::CheckWinConditions() {
    if (!matchRunning) {
        return;
    }

    if (mode == GameMode::FfaTimed || mode == GameMode::FfaScore) {
        if (players.size() <= 1) {
            matchRunning = false;
            countdownRemaining = 0.0F;
            if (players.size() == 1) {
                winnerPlayerId = players.front().id;
                winnerTeam = TeamId::None;
                matchEndedByForfeit = true;
            } else {
                winnerPlayerId = 0;
                winnerTeam = TeamId::None;
                matchEndedByForfeit = false;
            }
            return;
        }
    }

    if (mode == GameMode::TdmTimed || mode == GameMode::TdmScore) {
        int redCount = 0;
        int blueCount = 0;
        for (const PlayerState& player : players) {
            if (player.team == TeamId::Red) { ++redCount; }
            else if (player.team == TeamId::Blue) { ++blueCount; }
        }
        if (redCount == 0 || blueCount == 0) {
            matchRunning = false;
            countdownRemaining = 0.0F;
            if (redCount > 0) {
                winnerPlayerId = 0;
                winnerTeam = TeamId::Red;
                matchEndedByForfeit = true;
            } else if (blueCount > 0) {
                winnerPlayerId = 0;
                winnerTeam = TeamId::Blue;
                matchEndedByForfeit = true;
            } else {
                winnerPlayerId = 0;
                winnerTeam = TeamId::None;
                matchEndedByForfeit = false;
            }
            return;
        }
    }

    const bool timedDone = (mode == GameMode::FfaTimed || mode == GameMode::TdmTimed) && remainingSeconds <= 0.0F;
    const bool scoreDone = (mode == GameMode::FfaScore || mode == GameMode::TdmScore) &&
        std::any_of(players.begin(), players.end(), [this](const PlayerState& player) { return player.kills >= scoreLimit; });
    if (timedDone || scoreDone) {
        matchRunning = false;
        countdownRemaining = 0.0F;
        matchEndedByForfeit = false;
        if (IsTeamMode()) {
            std::int16_t redScore = 0;
            std::int16_t blueScore = 0;
            for (const PlayerState& p : players) {
                if (p.team == TeamId::Red) { redScore += p.kills; }
                else if (p.team == TeamId::Blue) { blueScore += p.kills; }
            }
            if (redScore > blueScore) { winnerTeam = TeamId::Red; }
            else if (blueScore > redScore) { winnerTeam = TeamId::Blue; }
            else { winnerTeam = TeamId::None; }
            winnerPlayerId = 0;
        } else {
            std::int16_t bestKills = -1;
            std::uint32_t bestId = 0;
            int tiedCount = 0;
            for (const PlayerState& p : players) {
                if (p.kills > bestKills) {
                    bestKills = p.kills;
                    bestId = p.id;
                    tiedCount = 1;
                } else if (p.kills == bestKills) {
                    ++tiedCount;
                }
            }
            winnerPlayerId = (tiedCount == 1) ? bestId : 0;
            winnerTeam = TeamId::None;
        }
        std::cout << "[MATCH] Match ended (Winner: ";
        if (winnerPlayerId != 0) {
            std::cout << "Player " << winnerPlayerId;
        } else if (winnerTeam == TeamId::Red) {
            std::cout << "Red Team";
        } else if (winnerTeam == TeamId::Blue) {
            std::cout << "Blue Team";
        } else {
            std::cout << "Draw/None";
        }
        if (matchEndedByForfeit) {
            std::cout << " by forfeit";
        }
        std::cout << ")\n";
    }
}

SnapshotPacket Room::MakeSnapshot(std::uint32_t playerId) const {
    SnapshotPacket packet{};
    packet.header = MakeHeader(PacketType::Snapshot, sizeof(SnapshotPacket));
    packet.matchRunning = matchRunning;
    packet.mode = mode;
    packet.mapIndex = mapIndex;
    packet.localPlayerId = playerId;
    packet.timeRemaining = remainingSeconds;
    packet.startCountdown = countdownRemaining;
    packet.playerCount = static_cast<std::uint8_t>(std::min(players.size(), static_cast<std::size_t>(cfg::MaxPlayers)));
    packet.bulletCount = static_cast<std::uint8_t>(std::min(bullets.size(), static_cast<std::size_t>(cfg::MaxBullets)));
    packet.grenadeCount = static_cast<std::uint8_t>(std::min(grenades.size(), static_cast<std::size_t>(cfg::MaxGrenades)));
    packet.barrierCount = static_cast<std::uint8_t>(std::min(barriers.size(), static_cast<std::size_t>(cfg::MaxBarriers)));
    packet.winnerPlayerId = winnerPlayerId;
    packet.winnerTeam = winnerTeam;
    packet.matchEndedByForfeit = matchEndedByForfeit;
    for (std::size_t index = 0; index < packet.playerCount; ++index) {
        const PlayerState& player = players[index];
        std::array<char, cfg::NameBytes> nameArr{};
        std::strncpy(nameArr.data(), player.name.c_str(), cfg::NameBytes - 1);
        nameArr[cfg::NameBytes - 1] = '\0';
        packet.players[index] = PlayerSnapshot{player.id, player.position.x, player.position.y, player.aim.x, player.aim.y,
            player.health, player.kills, player.deaths, player.ammo, player.playerClass, player.team, player.alive, player.host, player.connected,
            player.grenades, player.barriers, player.invulnerableRemaining, nameArr};
        if (player.team == TeamId::Red) { packet.redScore += player.kills; }
        if (player.team == TeamId::Blue) { packet.blueScore += player.kills; }
    }
    for (std::size_t index = 0; index < packet.bulletCount; ++index) {
        packet.bullets[index] = BulletSnapshot{bullets[index].position.x, bullets[index].position.y};
    }
    for (std::size_t index = 0; index < packet.grenadeCount; ++index) {
        const GrenadeState& grenade = grenades[index];
        float height = 0.0F;
        if (grenade.flightRemaining > 0.0F && grenade.flightDuration > 0.0F) {
            const float progress = 1.0F - (grenade.flightRemaining / grenade.flightDuration);
            height = std::sin(progress * Pi) * cfg::GrenadeArcHeight;
        }
        packet.grenades[index] = GrenadeSnapshot{grenade.position.x, grenade.position.y, grenade.fuse, height, grenade.ownerId};
    }
    for (std::size_t index = 0; index < packet.barrierCount; ++index) {
        packet.barriers[index] = BarrierSnapshot{barriers[index].position.x, barriers[index].position.y,
            barriers[index].rotation, barriers[index].health, barriers[index].ownerId};
    }
    packet.killFeedCount = static_cast<std::uint8_t>(std::min(recentKills.size(), static_cast<std::size_t>(cfg::MaxRecentKills)));
    for (std::size_t index = 0; index < packet.killFeedCount; ++index) {
        packet.killFeed[index] = recentKills[index];
    }
    return packet;
}

Room::PlayerState* Room::FindPlayer(std::uint32_t playerId) {
    for (PlayerState& player : players) {
        if (player.id == playerId) {
            return &player;
        }
    }
    return nullptr;
}

const Room::PlayerState* Room::FindPlayer(std::uint32_t playerId) const {
    for (const PlayerState& player : players) {
        if (player.id == playerId) {
            return &player;
        }
    }
    return nullptr;
}

void Room::ResetMatch() {
    bullets.clear();
    grenades.clear();
    barriers.clear();
    recentKills.clear();
    remainingSeconds = matchSeconds;
    winnerPlayerId = 0;
    winnerTeam = TeamId::None;
    matchEndedByForfeit = false;
    for (std::size_t index = 0; index < players.size(); ++index) {
        PlayerState& player = players[index];
        player.position = FurthestCornerSpawn(player.id);
        player.health = cfg::PlayerMaxHealth;
        player.kills = 0;
        player.deaths = 0;
        player.ammo = ClassAmmo(player.playerClass);
        player.inputFlags = 0;
        player.previousInputFlags = 0;
        player.cooldown = 0.5F;
        player.dashCooldown = 0.0F;
        player.dashRemaining = 0.0F;
        player.alive = true;
        player.invulnerableRemaining = cfg::SpawnProtectionSeconds;
        player.grenades = ClassMaxGrenades(player.playerClass);
        player.barriers = ClassMaxBarriers(player.playerClass);
    }
}

void Room::Fire(PlayerState& player) {
    if (player.cooldown > 0.0F || player.reloadRemaining > 0.0F || player.ammo == 0 || bullets.size() >= cfg::MaxBullets) {
        return;
    }
    if (player.playerClass == PlayerClass::Pistol && (player.previousInputFlags & InputFlags::Fire) != 0) {
        return;
    }
    --player.ammo;
    if (player.ammo == 0) {
        player.reloadRemaining = ClassReloadSeconds(player.playerClass);
    }
    player.cooldown = ClassCooldown(player.playerClass);
    const int pellets = player.playerClass == PlayerClass::Shotgun ? ShotgunPellets : 1;
    for (int index = 0; index < pellets && bullets.size() < cfg::MaxBullets; ++index) {
        float angle = std::atan2(player.aim.y, player.aim.x);
        if (pellets > 1) {
            const float center = static_cast<float>(index) - (static_cast<float>(pellets - 1) * 0.5F);
            angle += center * ShotgunSpread / static_cast<float>(pellets);
        }
        Vec2 direction(std::cos(angle), std::sin(angle));
        bullets.push_back(BulletState{player.id, player.position + (direction * cfg::PlayerRadius),
            direction * ClassSpeed(player.playerClass), ClassLife(player.playerClass), ClassDamage(player.playerClass),
            ToKillFeedWeapon(player.playerClass)});
    }
}

void Room::LaunchGrenade(PlayerState& player) {
    if (player.grenades == 0 || grenades.size() >= cfg::MaxGrenades) {
        return;
    }
    for (const GrenadeState& g : grenades) {
        if (g.ownerId == player.id) {
            return;
        }
    }
    --player.grenades;
    const Vec2 target(
        std::clamp(player.grenadeTarget.x, 0.0F, cfg::WorldWidth),
        std::clamp(player.grenadeTarget.y, 0.0F, cfg::WorldHeight));
    const float distance = Distance(player.position, target);
    const float flightSeconds = std::clamp(distance / cfg::GrenadeThrowSpeed, cfg::GrenadeMinFlightSeconds, cfg::GrenadeMaxFlightSeconds);
    const Vec2 velocity = (target - player.position) * (1.0F / flightSeconds);
    grenades.push_back(GrenadeState{player.id, player.position, target, velocity, -1.0F, flightSeconds, flightSeconds});
}

void Room::PlaceBarrier(PlayerState& player) {
    if (player.barriers == 0 || barriers.size() >= cfg::MaxBarriers) {
        return;
    }
    const Vec2 targetPos = player.position + (player.aim * cfg::BarrierPlaceDistance);
    for (const BarrierState& existing : barriers) {
        if (Distance(targetPos, existing.position) < 45.0F) {
            return;
        }
    }
    --player.barriers;
    barriers.push_back(BarrierState{targetPos,
        std::atan2(player.aim.y, player.aim.x) + Pi * 0.5F, cfg::BarrierHealth, player.id});
}

void Room::DamagePlayer(PlayerState& target, PlayerState* attacker, float damage, KillFeedWeapon weapon) {
    if (!target.alive || target.invulnerableRemaining > 0.0F) {
        return;
    }
    target.health -= damage;
    if (target.health > 0.0F) {
        return;
    }
    target.alive = false;
    target.health = 0.0F;
    target.respawnRemaining = cfg::RespawnSeconds;
    barriers.erase(std::remove_if(barriers.begin(), barriers.end(), [targetId = target.id](const BarrierState& barrier) {
        return barrier.ownerId == targetId;
    }), barriers.end());
    ++target.deaths;
    const bool isSuicide = (attacker == nullptr || attacker->id == target.id);
    if (attacker != nullptr && !isSuicide) {
        ++attacker->kills;
        attacker->health = std::min(cfg::PlayerMaxHealth, attacker->health + 0.5F * cfg::PlayerMaxHealth);
        attacker->grenades = ClassMaxGrenades(attacker->playerClass);
        attacker->barriers = ClassMaxBarriers(attacker->playerClass);
    }

    KillFeedSnapshot event{};
    event.eventId = ++nextKillEventId;
    event.killerId = attacker != nullptr ? attacker->id : 0;
    event.victimId = target.id;
    event.killerTeam = attacker != nullptr ? attacker->team : TeamId::None;
    event.victimTeam = target.team;
    event.weapon = weapon;
    event.isSuicide = isSuicide;

    PushKillFeed(event);
}

void Room::Respawn(PlayerState& player) {
    player.alive = true;
    player.health = cfg::PlayerMaxHealth;
    player.ammo = ClassAmmo(player.playerClass);
    player.position = FurthestCornerSpawn(player.id);
    player.dashCooldown = 0.0F;
    player.dashRemaining = 0.0F;
    player.cooldown = 0.5F;
    player.invulnerableRemaining = cfg::SpawnProtectionSeconds;
    player.grenades = ClassMaxGrenades(player.playerClass);
    player.barriers = ClassMaxBarriers(player.playerClass);
}

bool Room::IsTeamMode() const {
    return mode == GameMode::TdmTimed || mode == GameMode::TdmScore;
}

bool Room::IsBlocked(const Vec2& position) const {
    return IntersectsObstacle(position, cfg::PlayerRadius);
}

bool Room::IntersectsObstacle(const Vec2& position, float radius) const {
    for (const ObstacleState& obstacle : obstacles) {
        const float left = obstacle.position.x - obstacle.size.x * 0.5F;
        const float right = obstacle.position.x + obstacle.size.x * 0.5F;
        const float top = obstacle.position.y - obstacle.size.y * 0.5F;
        const float bottom = obstacle.position.y + obstacle.size.y * 0.5F;
        if (position.x + radius > left && position.x - radius < right && position.y + radius > top && position.y - radius < bottom) {
            return true;
        }
    }
    return false;
}

void Room::BuildMap() {
    obstacles.clear();
    if (mapIndex == 0) {
        obstacles = {{Vec2(1100.0F, 700.0F), Vec2(130.0F, 430.0F)}, {Vec2(560.0F, 330.0F), Vec2(360.0F, 76.0F)},
            {Vec2(1640.0F, 1070.0F), Vec2(360.0F, 76.0F)}, {Vec2(560.0F, 1070.0F), Vec2(320.0F, 76.0F)},
            {Vec2(1640.0F, 330.0F), Vec2(320.0F, 76.0F)}, {Vec2(850.0F, 700.0F), Vec2(90.0F, 240.0F)},
            {Vec2(1350.0F, 700.0F), Vec2(90.0F, 240.0F)}};
    } else if (mapIndex == 1) {
        obstacles = {{Vec2(1100.0F, 700.0F), Vec2(560.0F, 90.0F)}, {Vec2(570.0F, 700.0F), Vec2(90.0F, 540.0F)},
            {Vec2(1630.0F, 700.0F), Vec2(90.0F, 540.0F)}, {Vec2(830.0F, 400.0F), Vec2(90.0F, 260.0F)},
            {Vec2(1370.0F, 1000.0F), Vec2(90.0F, 260.0F)}, {Vec2(360.0F, 360.0F), Vec2(250.0F, 76.0F)},
            {Vec2(1840.0F, 1040.0F), Vec2(250.0F, 76.0F)}};
    } else {
        obstacles = {{Vec2(760.0F, 480.0F), Vec2(420.0F, 70.0F)}, {Vec2(1440.0F, 920.0F), Vec2(420.0F, 70.0F)},
            {Vec2(1100.0F, 700.0F), Vec2(120.0F, 120.0F)}, {Vec2(410.0F, 1040.0F), Vec2(290.0F, 70.0F)},
            {Vec2(1790.0F, 360.0F), Vec2(290.0F, 70.0F)}, {Vec2(760.0F, 920.0F), Vec2(70.0F, 300.0F)},
            {Vec2(1440.0F, 480.0F), Vec2(70.0F, 300.0F)}, {Vec2(1100.0F, 1080.0F), Vec2(360.0F, 70.0F)}};
    }
}

Vec2 Room::SpawnPoint(std::size_t index) const {
    const std::array<Vec2, cfg::MaxPlayers> spawns = {
        Vec2(190.0F, 190.0F),
        Vec2(2010.0F, 1210.0F),
        Vec2(190.0F, 1210.0F),
        Vec2(2010.0F, 190.0F)
    };
    return spawns[index % spawns.size()];
}

Vec2 Room::FurthestCornerSpawn(std::uint32_t playerId) const {
    const std::array<Vec2, 4> corners = {
        Vec2(190.0F, 190.0F),
        Vec2(2010.0F, 1210.0F),
        Vec2(190.0F, 1210.0F),
        Vec2(2010.0F, 190.0F)
    };
    const PlayerState* self = FindPlayer(playerId);
    const TeamId myTeam = self != nullptr ? self->team : TeamId::None;

    std::vector<Vec2> opponentPositions;
    for (const PlayerState& p : players) {
        if (p.id == playerId || !p.connected) {
            continue;
        }
        if (IsTeamMode() && p.team == myTeam && myTeam != TeamId::None) {
            continue;
        }
        if (p.alive) {
            opponentPositions.push_back(p.position);
        }
    }
    if (opponentPositions.empty()) {
        for (const PlayerState& p : players) {
            if (p.id == playerId || !p.connected) {
                continue;
            }
            if (IsTeamMode() && p.team == myTeam && myTeam != TeamId::None) {
                continue;
            }
            opponentPositions.push_back(p.position);
        }
    }
    if (opponentPositions.empty()) {
        for (const PlayerState& p : players) {
            if (p.id != playerId) {
                opponentPositions.push_back(p.position);
            }
        }
    }
    if (opponentPositions.empty()) {
        return corners[0];
    }

    float bestScore = -1.0F;
    float bestSumDist = -1.0F;
    Vec2 bestCorner = corners[0];

    for (const Vec2& corner : corners) {
        float minOppDist = 1e9F;
        float sumDist = 0.0F;
        for (const Vec2& oppPos : opponentPositions) {
            const float d = Distance(corner, oppPos);
            minOppDist = std::min(minOppDist, d);
            sumDist += d;
        }
        if (minOppDist > bestScore || (std::abs(minOppDist - bestScore) < 1.0F && sumDist > bestSumDist)) {
            bestScore = minOppDist;
            bestSumDist = sumDist;
            bestCorner = corner;
        }
    }
    return bestCorner;
}
