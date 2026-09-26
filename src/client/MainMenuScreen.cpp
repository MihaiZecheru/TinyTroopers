#include "client/MainMenuScreen.hpp"

#include "client/GameClient.hpp"
#include "client/ScreenManager.hpp"
#include "common/Constants.hpp"

#include "raylib.h"

namespace {
constexpr int TitleSize = 48;
constexpr int TextSize = 20;
constexpr Color TextColor{235, 238, 242, 255};
constexpr Color MutedColor{160, 170, 180, 255};
constexpr Color ButtonColor{52, 93, 74, 255};
constexpr Color ButtonHover{70, 122, 97, 255};

float UiScale() { return static_cast<float>(GetScreenWidth()) / 1280.0f; }
float Sf(float x) { return x * UiScale(); }
int Si(float x) { return static_cast<int>(x * UiScale()); }

bool Hit(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawButton(Rectangle bounds, const char* text) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRec(bounds, hover ? ButtonHover : ButtonColor);
    DrawRectangleLinesEx(bounds, 2.0F, Color{118, 148, 132, 255});
    DrawText(text, static_cast<int>(bounds.x + Sf(18.0F)), static_cast<int>(bounds.y + Sf(14.0F)), Si(TextSize), TextColor);
}
}

void MainMenuScreen::Update(GameClient& client) {
    const Rectangle create{Sf(cfg::MenuColumnX), Sf(cfg::MenuTopY + 120.0F), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)};
    const Rectangle join{Sf(cfg::MenuColumnX), create.y + Sf(cfg::ButtonHeight) + Sf(cfg::ButtonGap), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)};
    if (Hit(create)) {
        ScreenManager::Set(ScreenId::RoomCreation);
    }
    if (Hit(join)) {
        ScreenManager::Set(ScreenId::RoomJoining);
    }
    client.SetStatus(client.Status());
}

void MainMenuScreen::Draw(GameClient& client) const {
    const Rectangle create{Sf(cfg::MenuColumnX), Sf(cfg::MenuTopY + 120.0F), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)};
    const Rectangle join{Sf(cfg::MenuColumnX), create.y + Sf(cfg::ButtonHeight) + Sf(cfg::ButtonGap), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)};
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{22, 25, 29, 255});
    DrawRectangle(0, 0, GetScreenWidth(), static_cast<int>(Sf(8.0f)), Color{255, 204, 64, 255});
    DrawText("TinyTroopers", static_cast<int>(Sf(cfg::MenuColumnX)), static_cast<int>(Sf(cfg::MenuTopY)), Si(TitleSize), TextColor);
    DrawText("Top-down multiplayer shooter", static_cast<int>(Sf(cfg::MenuColumnX)), static_cast<int>(Sf(cfg::MenuTopY + 62.0F)), Si(TextSize), MutedColor);
    DrawButton(create, "Create Room");
    DrawButton(join, "Join Room");
    DrawText(client.Status().c_str(), static_cast<int>(Sf(cfg::MenuColumnX)), static_cast<int>(GetScreenHeight() - Sf(52.0f)), Si(TextSize), MutedColor);
}
