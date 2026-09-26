#include "common/Protocol.hpp"

#include <algorithm>
#include <cstring>

PacketHeader MakeHeader(PacketType type, std::uint16_t size) {
    return PacketHeader{cfg::ProtocolMagic, cfg::ProtocolVersion, type, size};
}

bool IsValidHeader(const PacketHeader& header, PacketType expectedType) {
    return header.magic == cfg::ProtocolMagic &&
        header.version == cfg::ProtocolVersion &&
        header.type == expectedType;
}

#pragma pack(push, 1)
struct CompactSnapshotHeader {
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
};
#pragma pack(pop)

int SerializeCompactSnapshot(const SnapshotPacket& src, std::uint8_t* dest, int maxDestSize) {
    if (dest == nullptr || maxDestSize <= 0) {
        return 0;
    }

    const std::uint8_t validPlayers = static_cast<std::uint8_t>(std::min<std::size_t>(src.playerCount, cfg::MaxPlayers));
    const std::uint8_t validBullets = static_cast<std::uint8_t>(std::min<std::size_t>(src.bulletCount, cfg::MaxBullets));
    const std::uint8_t validGrenades = static_cast<std::uint8_t>(std::min<std::size_t>(src.grenadeCount, cfg::MaxGrenades));
    const std::uint8_t validBarriers = static_cast<std::uint8_t>(std::min<std::size_t>(src.barrierCount, cfg::MaxBarriers));
    const std::uint8_t validKillFeed = static_cast<std::uint8_t>(std::min<std::size_t>(src.killFeedCount, cfg::MaxRecentKills));

    const std::size_t totalBytes = sizeof(CompactSnapshotHeader) +
        (validPlayers * sizeof(PlayerSnapshot)) +
        (validBullets * sizeof(BulletSnapshot)) +
        (validGrenades * sizeof(GrenadeSnapshot)) +
        (validBarriers * sizeof(BarrierSnapshot)) +
        (validKillFeed * sizeof(KillFeedSnapshot));

    if (static_cast<int>(totalBytes) > maxDestSize) {
        return 0;
    }

    CompactSnapshotHeader compHdr{};
    compHdr.header = MakeHeader(PacketType::CompactSnapshot, static_cast<std::uint16_t>(totalBytes));
    compHdr.matchRunning = src.matchRunning;
    compHdr.mode = src.mode;
    compHdr.mapIndex = src.mapIndex;
    compHdr.playerCount = validPlayers;
    compHdr.bulletCount = validBullets;
    compHdr.grenadeCount = validGrenades;
    compHdr.barrierCount = validBarriers;
    compHdr.killFeedCount = validKillFeed;
    compHdr.localPlayerId = src.localPlayerId;
    compHdr.timeRemaining = src.timeRemaining;
    compHdr.startCountdown = src.startCountdown;
    compHdr.redScore = src.redScore;
    compHdr.blueScore = src.blueScore;
    compHdr.winnerPlayerId = src.winnerPlayerId;
    compHdr.winnerTeam = src.winnerTeam;
    compHdr.matchEndedByForfeit = src.matchEndedByForfeit;

    std::uint8_t* ptr = dest;
    std::memcpy(ptr, &compHdr, sizeof(compHdr));
    ptr += sizeof(compHdr);

    if (validPlayers > 0) {
        const std::size_t bytes = validPlayers * sizeof(PlayerSnapshot);
        std::memcpy(ptr, src.players.data(), bytes);
        ptr += bytes;
    }
    if (validBullets > 0) {
        const std::size_t bytes = validBullets * sizeof(BulletSnapshot);
        std::memcpy(ptr, src.bullets.data(), bytes);
        ptr += bytes;
    }
    if (validGrenades > 0) {
        const std::size_t bytes = validGrenades * sizeof(GrenadeSnapshot);
        std::memcpy(ptr, src.grenades.data(), bytes);
        ptr += bytes;
    }
    if (validBarriers > 0) {
        const std::size_t bytes = validBarriers * sizeof(BarrierSnapshot);
        std::memcpy(ptr, src.barriers.data(), bytes);
        ptr += bytes;
    }
    if (validKillFeed > 0) {
        const std::size_t bytes = validKillFeed * sizeof(KillFeedSnapshot);
        std::memcpy(ptr, src.killFeed.data(), bytes);
        ptr += bytes;
    }

    return static_cast<int>(totalBytes);
}

bool DeserializeCompactSnapshot(const std::uint8_t* src, int srcSize, SnapshotPacket& dest) {
    if (src == nullptr || srcSize < static_cast<int>(sizeof(CompactSnapshotHeader))) {
        return false;
    }

    const PacketHeader* hdr = reinterpret_cast<const PacketHeader*>(src);
    if (!IsValidHeader(*hdr, PacketType::CompactSnapshot)) {
        return false;
    }

    CompactSnapshotHeader compHdr{};
    std::memcpy(&compHdr, src, sizeof(CompactSnapshotHeader));

    const std::uint8_t validPlayers = static_cast<std::uint8_t>(std::min<std::size_t>(compHdr.playerCount, cfg::MaxPlayers));
    const std::uint8_t validBullets = static_cast<std::uint8_t>(std::min<std::size_t>(compHdr.bulletCount, cfg::MaxBullets));
    const std::uint8_t validGrenades = static_cast<std::uint8_t>(std::min<std::size_t>(compHdr.grenadeCount, cfg::MaxGrenades));
    const std::uint8_t validBarriers = static_cast<std::uint8_t>(std::min<std::size_t>(compHdr.barrierCount, cfg::MaxBarriers));
    const std::uint8_t validKillFeed = static_cast<std::uint8_t>(std::min<std::size_t>(compHdr.killFeedCount, cfg::MaxRecentKills));

    const std::size_t expectedBytes = sizeof(CompactSnapshotHeader) +
        (validPlayers * sizeof(PlayerSnapshot)) +
        (validBullets * sizeof(BulletSnapshot)) +
        (validGrenades * sizeof(GrenadeSnapshot)) +
        (validBarriers * sizeof(BarrierSnapshot)) +
        (validKillFeed * sizeof(KillFeedSnapshot));

    if (srcSize < static_cast<int>(expectedBytes)) {
        return false;
    }

    dest = SnapshotPacket{};
    dest.header = MakeHeader(PacketType::Snapshot, sizeof(SnapshotPacket));
    dest.matchRunning = compHdr.matchRunning;
    dest.mode = compHdr.mode;
    dest.mapIndex = compHdr.mapIndex;
    dest.playerCount = validPlayers;
    dest.bulletCount = validBullets;
    dest.grenadeCount = validGrenades;
    dest.barrierCount = validBarriers;
    dest.killFeedCount = validKillFeed;
    dest.localPlayerId = compHdr.localPlayerId;
    dest.timeRemaining = compHdr.timeRemaining;
    dest.startCountdown = compHdr.startCountdown;
    dest.redScore = compHdr.redScore;
    dest.blueScore = compHdr.blueScore;
    dest.winnerPlayerId = compHdr.winnerPlayerId;
    dest.winnerTeam = compHdr.winnerTeam;
    dest.matchEndedByForfeit = compHdr.matchEndedByForfeit;

    const std::uint8_t* ptr = src + sizeof(CompactSnapshotHeader);
    if (validPlayers > 0) {
        const std::size_t bytes = validPlayers * sizeof(PlayerSnapshot);
        std::memcpy(dest.players.data(), ptr, bytes);
        ptr += bytes;
    }
    if (validBullets > 0) {
        const std::size_t bytes = validBullets * sizeof(BulletSnapshot);
        std::memcpy(dest.bullets.data(), ptr, bytes);
        ptr += bytes;
    }
    if (validGrenades > 0) {
        const std::size_t bytes = validGrenades * sizeof(GrenadeSnapshot);
        std::memcpy(dest.grenades.data(), ptr, bytes);
        ptr += bytes;
    }
    if (validBarriers > 0) {
        const std::size_t bytes = validBarriers * sizeof(BarrierSnapshot);
        std::memcpy(dest.barriers.data(), ptr, bytes);
        ptr += bytes;
    }
    if (validKillFeed > 0) {
        const std::size_t bytes = validKillFeed * sizeof(KillFeedSnapshot);
        std::memcpy(dest.killFeed.data(), ptr, bytes);
        ptr += bytes;
    }

    return true;
}
