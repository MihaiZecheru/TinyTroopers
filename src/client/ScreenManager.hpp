#pragma once

#include "client/GameplayScreen.hpp"
#include "client/MainMenuScreen.hpp"
#include "client/RoomCreationScreen.hpp"
#include "client/RoomJoiningScreen.hpp"
#include "common/Protocol.hpp"

class GameClient;

class ScreenManager {
public:
    ScreenManager();
    ~ScreenManager();
    void Update(GameClient& client);
    void Draw(GameClient& client) const;
    static void Set(ScreenId screen);
    static ScreenId Current();
    static GameplayScreen* Gameplay();

private:
    static ScreenId current;
    static ScreenManager* instance;
    MainMenuScreen mainMenu;
    RoomCreationScreen creation;
    RoomJoiningScreen joining;
    GameplayScreen gameplay;
};
