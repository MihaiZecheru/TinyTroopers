#include "server/GameServer.hpp"

#include "common/Constants.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

GameServer::GameServer() : snapshotAccumulator(0.0F) {}

bool GameServer::Run() {
    if (!socket.Open(cfg::ServerPort)) {
        std::cerr << "Failed to bind UDP server on port " << cfg::ServerPort << std::endl;
        return false;
    }
    std::cout << "TinyTroopers server listening on UDP " << cfg::ServerPort << std::endl;

    if (httpServer.Open(cfg::ServerPort)) {
        std::cout << "TinyTroopers HTTP status server listening on TCP " << cfg::ServerPort << std::endl;
    } else {
        std::cerr << "Warning: Failed to bind HTTP status server on TCP port " << cfg::ServerPort << std::endl;
    }

    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    while (true) {
        const auto now = Clock::now();
        const std::chrono::duration<float> elapsed = now - previous;
        previous = now;
        PumpNetwork();
        for (auto& entry : roomsByCode) {
            entry.second.Update(std::min(elapsed.count(), cfg::FixedDt));
        }
        RemoveTimedOutPlayers(elapsed.count());
        snapshotAccumulator += elapsed.count();
        if (snapshotAccumulator >= cfg::SnapshotSeconds) {
            SendSnapshots();
            snapshotAccumulator = 0.0F;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::PumpNetwork() {
    httpServer.Poll([this](const std::string& path) {
        return HandleHttpRequest(path);
    });

    std::array<std::uint8_t, cfg::PacketBytes> bytes{};
    sockaddr_in address{};
    while (true) {
        const int received = socket.Receive(address, bytes.data(), static_cast<int>(bytes.size()));
        if (received <= 0 || received < static_cast<int>(sizeof(PacketHeader))) {
            break;
        }
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(bytes.data());
        const std::string key = UdpSocket::AddressKey(address);
        auto sessionEntry = sessionsByAddress.find(key);

        if (IsValidHeader(*header, PacketType::Hello) && received >= static_cast<int>(sizeof(HelloPacket))) {
            const HelloPacket* hello = reinterpret_cast<const HelloPacket*>(bytes.data());
            const bool createRoom = hello->createRoom;
            std::string roomCode = createRoom ? GenerateRoomCode() : NormalizeRoomCode(hello->roomCode);
            if (!createRoom && roomsByCode.find(roomCode) == roomsByCode.end()) {
                std::cout << "[ROOM] Join failed: Room " << roomCode << " not found (Client: " << key << ")\n";
                SendMessage(address, "Room " + roomCode + " not found.");
                continue;
            }

            std::string playerName(hello->name.data(), std::min(std::strlen(hello->name.data()), static_cast<std::size_t>(cfg::NameBytes - 1)));
            while (!playerName.empty() && (playerName.back() == ' ' || playerName.back() == '\r' || playerName.back() == '\n' || playerName.back() == '\t')) {
                playerName.pop_back();
            }
            std::size_t pStart = 0;
            while (pStart < playerName.size() && (playerName[pStart] == ' ' || playerName[pStart] == '\t')) {
                ++pStart;
            }
            if (pStart > 0) {
                playerName = playerName.substr(pStart);
            }

            Room& room = roomsByCode[roomCode];
            auto existingSession = sessionsByAddress.find(key);
            if (existingSession != sessionsByAddress.end() && existingSession->second.roomCode == roomCode) {
                const std::string existingName = room.GetPlayerName(existingSession->second.playerId);
                if (!existingName.empty() && existingName != playerName && room.IsPlayerConnected(existingSession->second.playerId)) {
                    std::cout << "[ROOM] Join rejected: Network conflict for '" << playerName << "' (address " << key << " already used by '" << existingName << "' in Room " << roomCode << ")\n";
                    SendMessage(address, "Another player on your network is already in this room.\nIf you are on the same network, please use LAN IP.");
                    continue;
                }
            }

            const std::uint32_t playerId = room.AddOrFindPlayer(key, playerName);
            if (playerId == 0) {
                std::cout << "[ROOM] Join failed: Room " << roomCode << " is full (Client: " << key << ")\n";
                SendMessage(address, "Room is full.");
                continue;
            }
            sessionsByAddress[key] = ClientSession{roomCode, playerId, address, 0.0F, false, hello->preferCompact};
            if (createRoom) {
                std::cout << "[ROOM] Room " << roomCode << " created by host '" << playerName << "' at " << key << " (Player ID: " << playerId << (hello->preferCompact ? ", compact snapshots" : "") << ")\n";
            } else {
                std::cout << "[ROOM] Player " << playerId << " ('" << playerName << "') joined Room " << roomCode << " from " << key << " (Total: " << room.PlayerCount() << (hello->preferCompact ? ", compact snapshots" : "") << ")\n";
            }
            SendMessage(address, createRoom ? "Room " + roomCode + " created." : "Joined room " + roomCode + ".");
        } else if (sessionEntry != sessionsByAddress.end()) {
            ClientSession& session = sessionEntry->second;
            auto roomEntry = roomsByCode.find(session.roomCode);
            if (roomEntry == roomsByCode.end()) {
                sessionsByAddress.erase(sessionEntry);
                continue;
            }
            Room& room = roomEntry->second;
            session.secondsSinceHeard = 0.0F;
            session.address = address;
            if (session.disconnected) {
                session.disconnected = false;
                std::cout << "[ROOM] Player " << session.playerId << " reconnected to Room " << session.roomCode << " from " << key << "\n";
                room.TryReconnectPlayer(key, session.playerId);
            }
            if (IsValidHeader(*header, PacketType::LobbyConfig) && received >= static_cast<int>(sizeof(LobbyConfigPacket))) {
                room.Configure(session.playerId, *reinterpret_cast<const LobbyConfigPacket*>(bytes.data()));
                std::cout << "[LOBBY] Room " << session.roomCode << " config updated by Player " << session.playerId << "\n";
            } else if (IsValidHeader(*header, PacketType::Input) && received >= static_cast<int>(sizeof(InputPacket))) {
                room.ApplyInput(session.playerId, *reinterpret_cast<const InputPacket*>(bytes.data()));
            } else if (IsValidHeader(*header, PacketType::StartMatch)) {
                const std::string result = room.TryStart(session.playerId);
                std::cout << "[MATCH] Room " << session.roomCode << " StartMatch by Player " << session.playerId << ": " << result << "\n";
                SendMessage(address, result);
            } else if (IsValidHeader(*header, PacketType::Disconnect)) {
                std::cout << "[ROOM] Player " << session.playerId << " sent disconnect in Room " << session.roomCode << "\n";
                session.disconnected = true;
                room.MarkPlayerDisconnected(session.playerId);
            } else if (IsValidHeader(*header, PacketType::KickPlayer) && received >= static_cast<int>(sizeof(KickPlayerPacket))) {
                const auto* kick = reinterpret_cast<const KickPlayerPacket*>(bytes.data());
                if (room.IsPlayerHost(session.playerId) && kick->targetPlayerId != session.playerId) {
                    std::cout << "[ROOM] Host " << session.playerId << " kicked Player " << kick->targetPlayerId << " in Room " << session.roomCode << "\n";
                    for (const auto& entry : sessionsByAddress) {
                        if (entry.second.roomCode == session.roomCode && entry.second.playerId == kick->targetPlayerId) {
                            SendMessage(entry.second.address, "You were kicked by the host.");
                            break;
                        }
                    }
                    room.KickPlayer(kick->targetPlayerId);
                }
            }
        }
    }
}

void GameServer::SendSnapshots() {
    for (const auto& entry : sessionsByAddress) {
        const ClientSession& session = entry.second;
        if (session.disconnected) {
            continue;
        }
        auto roomEntry = roomsByCode.find(session.roomCode);
        if (roomEntry == roomsByCode.end()) {
            continue;
        }
        const SnapshotPacket snapshot = roomEntry->second.MakeSnapshot(session.playerId);
        if (session.preferCompact) {
            std::array<std::uint8_t, cfg::PacketBytes> compactBuffer{};
            const int compactSize = SerializeCompactSnapshot(snapshot, compactBuffer.data(), static_cast<int>(compactBuffer.size()));
            if (compactSize > 0) {
                socket.Send(session.address, compactBuffer.data(), compactSize);
                continue;
            }
        }
        socket.Send(session.address, &snapshot, sizeof(snapshot));
    }
}

void GameServer::SendMessage(const sockaddr_in& address, const std::string& text) {
    ServerMessagePacket packet{};
    packet.header = MakeHeader(PacketType::ServerMessage, sizeof(ServerMessagePacket));
    std::strncpy(packet.text.data(), text.c_str(), packet.text.size() - 1);
    socket.Send(address, &packet, sizeof(packet));
}

void GameServer::RemoveTimedOutPlayers(float dt) {
    for (auto& entry : sessionsByAddress) {
        ClientSession& session = entry.second;
        session.secondsSinceHeard += dt;
        auto roomEntry = roomsByCode.find(session.roomCode);
        if (roomEntry == roomsByCode.end()) {
            continue;
        }
        Room& room = roomEntry->second;
        if (!session.disconnected && session.secondsSinceHeard >= cfg::ClientSilenceThreshold) {
            session.disconnected = true;
            std::cout << "[TIMEOUT] Player " << session.playerId << " connection silent in Room " << session.roomCode << "\n";
            room.MarkPlayerDisconnected(session.playerId);
        }
    }

    std::vector<std::string> toEraseSessions;
    for (const auto& entry : sessionsByAddress) {
        const ClientSession& session = entry.second;
        auto roomEntry = roomsByCode.find(session.roomCode);
        if (roomEntry == roomsByCode.end()) {
            toEraseSessions.push_back(entry.first);
        } else {
            const Room& room = roomEntry->second;
            if (!room.IsPlayerConnected(session.playerId) && session.secondsSinceHeard >= (cfg::ClientSilenceThreshold + cfg::ReconnectWindowSeconds)) {
                std::cout << "[TIMEOUT] Player " << session.playerId << " session expired in Room " << session.roomCode << "\n";
                toEraseSessions.push_back(entry.first);
            }
        }
    }
    for (const std::string& key : toEraseSessions) {
        sessionsByAddress.erase(key);
    }

    for (auto it = roomsByCode.begin(); it != roomsByCode.end(); ) {
        if (it->second.PlayerCount() == 0) {
            std::cout << "[ROOM] Room " << it->first << " is now empty and closed.\n";
            it = roomsByCode.erase(it);
        } else {
            ++it;
        }
    }
}

std::string GameServer::GenerateRoomCode() const {
    constexpr char Digits[] = "0123456789";
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 9);
    std::string code;
    do {
        code.clear();
        for (int index = 0; index < cfg::RoomCodeLength; ++index) {
            code.push_back(Digits[distribution(generator)]);
        }
    } while (roomsByCode.find(code) != roomsByCode.end());
    return code;
}

std::string GameServer::NormalizeRoomCode(const std::array<char, cfg::RoomCodeBytes>& roomCode) {
    std::string normalized;
    for (char value : roomCode) {
        if (value == '\0' || normalized.size() >= cfg::RoomCodeLength) {
            break;
        }
        if (value >= '0' && value <= '9') {
            normalized.push_back(value);
        }
    }
    return normalized;
}

std::string GameServer::HandleHttpRequest(const std::string& path) const {
    std::size_t activePlayerCount = 0;
    for (const auto& entry : roomsByCode) {
        activePlayerCount += entry.second.PlayerCount();
    }

    std::ostringstream json;
    json << "{\n"
         << "  \"status\": \"online\",\n"
         << "  \"server\": \"TinyTroopers\",\n"
         << "  \"path\": \"" << path << "\",\n"
         << "  \"udpPort\": " << cfg::ServerPort << ",\n"
         << "  \"tcpPort\": " << cfg::ServerPort << ",\n"
         << "  \"activeRooms\": " << roomsByCode.size() << ",\n"
         << "  \"activePlayers\": " << activePlayerCount << ",\n"
         << "  \"rooms\": [";

    bool first = true;
    for (const auto& entry : roomsByCode) {
        if (!first) {
            json << ", ";
        }
        first = false;
        json << "{\"code\": \"" << entry.first << "\", \"players\": " << entry.second.PlayerCount() << "}";
    }

    json << "]\n}\n";
    return json.str();
}

