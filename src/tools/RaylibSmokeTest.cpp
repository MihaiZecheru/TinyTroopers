#include "common/Constants.hpp"

#include "raylib.h"

#include <cmath>

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(cfg::WindowWidth, cfg::WindowHeight, "Raylib Smoke Test");
    SetTargetFPS(cfg::TargetFps);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{180, 30, 70, 255});
        DrawRectangle(0, 0, GetScreenWidth(), 48, Color{255, 220, 40, 255});
        DrawRectangle(80, 120, 420, 180, Color{30, 90, 220, 255});
        DrawCircle(620 + static_cast<int>(std::sin(GetTime() * 3.0) * 120.0), 240, 64.0F, Color{40, 230, 120, 255});
        DrawText("Raylib SDL presentation test", 48, 80, 36, WHITE);
        DrawText("You should see magenta, yellow, blue, and a moving green circle.", 48, 330, 22, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
