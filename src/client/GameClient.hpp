#pragma once

#include "client/AssetManager.hpp"
#include "client/AudioManager.hpp"
#include "client/NetworkClient.hpp"
#include "common/Protocol.hpp"

#include "raylib.h"

#include <string>

class GameClient {
public:
    GameClient();

    int Run();
    NetworkClient& Network();
    const NetworkClient& Network() const;
    AssetManager& Assets();
    const AssetManager& Assets() const;
    AudioManager& Audio();
    const AudioManager& Audio() const;
    SnapshotPacket& Snapshot();
    const SnapshotPacket& Snapshot() const;
    LobbyConfigPacket MakeConfig() const;
    PlayerSnapshot LocalPlayer() const;
    void SetMode(GameMode value);
    void SetClass(PlayerClass value);
    void SetTeam(TeamId value);
    void SetMap(std::uint8_t value);
    GameMode SelectedMode() const;
    PlayerClass SelectedClass() const;
    std::uint8_t SelectedMap() const;
    void SetServerHost(const std::string& value);
    const std::string& ServerHost() const;
    void SetPlayerName(const std::string& value);
    const std::string& PlayerName() const;
    void LoadPlayerName();
    void SavePlayerName() const;
    const std::string& Status() const;
    void SetStatus(const std::string& value);
    const std::string& RoomCode() const;
    bool IsHost() const;
    void CreateRoom();
    void JoinRoom(const std::string& roomCode);
    void LeaveRoom();
    void Disconnect(const std::string& reason);
    bool ConsumeJoinedRoomConfirmation();
    bool ConsumeCreatedRoomConfirmation();
    void ShowMessage(const std::string& value);
    bool HasNewSnapshot() const;
    void ToggleWindowedFullscreen();
    bool IsWindowedFullscreen() const;

private:
    void UpdateHelpOverlay();
    void DrawHelpOverlay() const;
    void HandleServerMessage(const std::string& message);
    void UpdateMessagePopup();
    void DrawMessagePopup() const;

    NetworkClient network;
    AssetManager assets;
    AudioManager audio;
    SnapshotPacket snapshot;
    GameMode selectedMode;
    PlayerClass selectedClass;
    TeamId selectedTeam;
    std::uint8_t selectedMap;
    std::string serverHost;
    std::string playerName;
    std::string status;
    std::string roomCode;
    std::string popupMessage;
    float secondsSinceServerPacket;
    float connectTimer;
    float heartbeatAccumulator;
    bool connecting;
    bool hostHint;
    bool joinedRoomConfirmed;
    bool createdRoomConfirmed;
    bool helpOpen;
    bool popupOpen;
    bool newSnapshotReceived;
};
