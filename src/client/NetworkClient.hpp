#pragma once

#include "common/Protocol.hpp"

#include <memory>
#include <string>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Connect(const std::string& host, std::uint16_t port, const std::string& playerName, const std::string& roomCode, bool createRoom);
    void SendConfig(const LobbyConfigPacket& packet);
    void SendInput(const InputPacket& packet);
    void SendStart();
    void SendKick(std::uint32_t targetPlayerId);
    void Disconnect(std::uint32_t playerId = 0);
    bool IsConnected() const;
    bool PollSnapshot(SnapshotPacket& packet);
    bool PollMessage(std::string& text);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
