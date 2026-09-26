#include "client/GameClient.hpp"

#include "raylib.h"

int main() {
    ChangeDirectory(GetApplicationDirectory());
    GameClient client;
    return client.Run();
}

