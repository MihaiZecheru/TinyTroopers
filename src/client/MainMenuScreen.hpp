#pragma once

class GameClient;

class MainMenuScreen {
public:
    void Update(GameClient& client);
    void Draw(GameClient& client) const;
};
