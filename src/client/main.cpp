#include "client/GameClient.hpp"

#include "raylib.h"

int main() {
    ChangeDirectory(GetApplicationDirectory());
    // Inside a macOS .app bundle, assets live in Contents/Resources (next to Contents/MacOS)
    if (!DirectoryExists("assets") && DirectoryExists("../Resources/assets")) {
        ChangeDirectory("../Resources");
    }
    GameClient client;
    return client.Run();
}

