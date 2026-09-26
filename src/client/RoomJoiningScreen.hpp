#pragma once

#include "common/Constants.hpp"

#include <string>
#include <vector>

class GameClient;

class RoomJoiningScreen {
public:
    RoomJoiningScreen();
    void Update(GameClient& client);
    void Draw(GameClient& client) const;

private:
    enum class FieldFocus {
        Name,
        ServerHost,
        RoomCode
    };

    void LoadServerHistory();
    void SaveServerHistory() const;
    void AddSavedServer(const std::string& host);

    char nameBuffer[cfg::NameBytes];
    char roomCodeBuffer[cfg::RoomCodeBytes];
    char serverHostBuffer[128];
    FieldFocus focus;

    float deleteHoldTimer;
    float deleteRepeatTimer;
    bool dropdownOpen;
    bool initialized;
    std::vector<std::string> savedServers;
};
