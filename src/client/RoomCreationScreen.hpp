#pragma once

#include "common/Constants.hpp"

class GameClient;

class RoomCreationScreen {
public:
    RoomCreationScreen();
    void Update(GameClient& client);
    void Draw(GameClient& client) const;

private:
    char nameBuffer[cfg::NameBytes];
    bool nameFocus;
    float deleteHoldTimer;
    float deleteRepeatTimer;
    bool initialized;
};
