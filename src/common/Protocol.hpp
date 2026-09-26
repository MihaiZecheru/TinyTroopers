#pragma once

#include "common/Constants.hpp"

#include <array>
#include <cstdint>

enum class PacketType : std::uint8_t {
    Hello = 1,
    LobbyConfig = 2,
    Input = 3,
    StartMatch = 4,
    Snapshot = 5,
    ServerMessage = 6,
    Disconnect = 7,
    KickPlayer = 8,
    CompactSnapshot = 9
};

enum class ScreenId : std::uint8_t {
    MainMenu,
    RoomCreation,
    RoomJoining,
    Gameplay
};

enum class GameMode : std::uint8_t {
    FfaTimed,
    FfaScore,
    TdmTimed,
    TdmScore
};

enum class PlayerClass : std::uint8_t {
    Ak47,
    Pistol,
    Sniper,
    Shotgun
};

inline constexpr float ClassCooldown(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return 0.105F;
    case PlayerClass::Pistol: return 0.0F;
    case PlayerClass::Sniper: return 1.25F;
    case PlayerClass::Shotgun: return 0.62F;
    }
    return 0.105F;
}

inline constexpr float ClassReloadSeconds(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Sniper: return cfg::ReloadSeconds * 3.0F;
    default: return cfg::ReloadSeconds;
    }
}

inline constexpr std::uint8_t ClassMaxAmmo(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return static_cast<std::uint8_t>(cfg::RifleAmmo);
    case PlayerClass::Pistol: return static_cast<std::uint8_t>(cfg::PistolAmmo);
    case PlayerClass::Sniper: return static_cast<std::uint8_t>(cfg::SniperAmmo);
    case PlayerClass::Shotgun: return static_cast<std::uint8_t>(cfg::ShotgunAmmo);
    }
    return static_cast<std::uint8_t>(cfg::RifleAmmo);
}

inline constexpr std::uint8_t ClassMaxGrenades(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return static_cast<std::uint8_t>(cfg::RifleGrenades);
    case PlayerClass::Pistol: return static_cast<std::uint8_t>(cfg::PistolGrenades);
    case PlayerClass::Sniper: return static_cast<std::uint8_t>(cfg::SniperGrenades);
    case PlayerClass::Shotgun: return static_cast<std::uint8_t>(cfg::ShotgunGrenades);
    }
    return static_cast<std::uint8_t>(cfg::RifleGrenades);
}

inline constexpr std::uint8_t ClassMaxBarriers(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return static_cast<std::uint8_t>(cfg::RifleBarriers);
    case PlayerClass::Pistol: return static_cast<std::uint8_t>(cfg::PistolBarriers);
    case PlayerClass::Sniper: return static_cast<std::uint8_t>(cfg::SniperBarriers);
    case PlayerClass::Shotgun: return static_cast<std::uint8_t>(cfg::ShotgunBarriers);
    }
    return static_cast<std::uint8_t>(cfg::RifleBarriers);
}

inline constexpr const char* ClassName(PlayerClass playerClass) {
    switch (playerClass) {
    case PlayerClass::Ak47: return "AK47";
    case PlayerClass::Pistol: return "M1911";
    case PlayerClass::Sniper: return "KAR98k";
    case PlayerClass::Shotgun: return "Mossberg";
    }
    return "Soldier";
}

enum class TeamId : std::uint8_t {
    None,
    Red,
    Blue
};

enum class KillFeedWeapon : std::uint8_t {
    Ak47,
    Pistol,
    Sniper,
    Shotgun,
    Grenade,
    Disconnected,
    Reconnected,
    Kicked,
    LeftRoom
};

struct KillFeedSnapshot {
    std::uint32_t eventId;
    std::uint32_t killerId;
    std::uint32_t victimId;
    TeamId killerTeam;
    TeamId victimTeam;
    KillFeedWeapon weapon;
    bool isSuicide;
};

enum InputFlags : std::uint16_t {
    MoveUp = 1 << 0,
    MoveDown = 1 << 1,
    MoveLeft = 1 << 2,
    MoveRight = 1 << 3,
    Fire = 1 << 4,
    Reload = 1 << 5,
    Grenade = 1 << 6,
    Barrier = 1 << 7,
    Respawn = 1 << 8,
    Dash = 1 << 9
};

struct PacketHeader {
    std::uint32_t magic;
    std::uint16_t version;
    PacketType type;
    std::uint16_t size;
};

struct HelloPacket {
    PacketHeader header;
    std::array<char, cfg::NameBytes> name;
    std::array<char, cfg::RoomCodeBytes> roomCode;
    bool createRoom;
    bool preferCompact;
};

struct LobbyConfigPacket {
    PacketHeader header;
    GameMode mode;
    PlayerClass playerClass;
    TeamId team;
    std::uint8_t mapIndex;
    std::uint16_t scoreLimit;
    float minutes;
};

struct InputPacket {
    PacketHeader header;
    std::uint32_t sequence;
    std::uint16_t flags;
    float aimX;
    float aimY;
    float grenadeTargetX;
    float grenadeTargetY;
};

struct StartMatchPacket {
    PacketHeader header;
};

struct DisconnectPacket {
    PacketHeader header;
    std::uint32_t playerId;
};

struct KickPlayerPacket {
    PacketHeader header;
    std::uint32_t targetPlayerId;
};

struct PlayerSnapshot {
    std::uint32_t id;
    float x;
    float y;
    float aimX;
    float aimY;
    float health;
    std::int16_t kills;
    std::int16_t deaths;
    std::uint8_t ammo;
    PlayerClass playerClass;
    TeamId team;
    bool alive;
    bool host;
    bool connected;
    std::uint8_t grenades;
    std::uint8_t barriers;
    float invulnerableRemaining;
    std::array<char, cfg::NameBytes> name;
};

struct BulletSnapshot {
    float x;
    float y;
};

struct GrenadeSnapshot {
    float x;
    float y;
    float fuse;
    float height;
    std::uint32_t ownerId;
};

struct BarrierSnapshot {
    float x;
    float y;
    float rotation;
    float health;
    std::uint32_t ownerId;
};

struct SnapshotPacket {
    PacketHeader header;
    bool matchRunning;
    GameMode mode;
    std::uint8_t mapIndex;
    std::uint8_t playerCount;
    std::uint8_t bulletCount;
    std::uint8_t grenadeCount;
    std::uint8_t barrierCount;
    std::uint8_t killFeedCount;
    std::uint32_t localPlayerId;
    float timeRemaining;
    float startCountdown;
    std::int16_t redScore;
    std::int16_t blueScore;
    std::uint32_t winnerPlayerId;
    TeamId winnerTeam;
    bool matchEndedByForfeit;
    std::array<PlayerSnapshot, cfg::MaxPlayers> players;
    std::array<BulletSnapshot, cfg::MaxBullets> bullets;
    std::array<GrenadeSnapshot, cfg::MaxGrenades> grenades;
    std::array<BarrierSnapshot, cfg::MaxBarriers> barriers;
    std::array<KillFeedSnapshot, cfg::MaxRecentKills> killFeed;
};

struct ServerMessagePacket {
    PacketHeader header;
    std::array<char, 256> text;
};

PacketHeader MakeHeader(PacketType type, std::uint16_t size);
bool IsValidHeader(const PacketHeader& header, PacketType expectedType);
int SerializeCompactSnapshot(const SnapshotPacket& src, std::uint8_t* dest, int maxDestSize);
bool DeserializeCompactSnapshot(const std::uint8_t* src, int srcSize, SnapshotPacket& dest);
