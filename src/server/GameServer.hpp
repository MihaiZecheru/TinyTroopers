#pragma once

#include "common/UdpSocket.hpp"
#include "server/HttpServer.hpp"
#include "server/Room.hpp"

#include <string>
#include <unordered_map>

class GameServer {
public:
    GameServer();
    bool Run();

private:
    void PumpNetwork();
    void SendSnapshots();
    void SendMessage(const sockaddr_in& address, const std::string& text);
    void RemoveTimedOutPlayers(float dt);
    std::string GenerateRoomCode() const;
    static std::string NormalizeRoomCode(const std::array<char, cfg::RoomCodeBytes>& roomCode);
    std::string HandleHttpRequest(const std::string& path) const;

    struct ClientSession {
        std::string roomCode;
        std::uint32_t playerId;
        sockaddr_in address;
        float secondsSinceHeard;
        bool disconnected;
    };

    UdpSocket socket;
    HttpServer httpServer;
    std::unordered_map<std::string, Room> roomsByCode;
    std::unordered_map<std::string, ClientSession> sessionsByAddress;
    float snapshotAccumulator;
};
