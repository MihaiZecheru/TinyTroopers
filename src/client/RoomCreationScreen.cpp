#include "client/RoomCreationScreen.hpp"

#include "client/GameClient.hpp"
#include "client/ScreenManager.hpp"
#include "common/Constants.hpp"

#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace {
constexpr int HeaderSize = 34;
constexpr int TextSize = 20;
constexpr Color TextColor{238, 240, 244, 255};
constexpr Color MutedColor{158, 168, 177, 255};
constexpr Color ButtonColor{54, 66, 82, 255};
constexpr Color ActiveColor{48, 118, 96, 255};
constexpr Color FieldColor{35, 42, 52, 255};
constexpr float RowX = 58.0F;
constexpr float SmallWidth = 190.0F;
constexpr float MediumWidth = 250.0F;
constexpr float HeaderY = 76.0F;
constexpr float NameY = 142.0F;
constexpr float ModeY = 224.0F;
constexpr float ClassY = 362.0F;
constexpr float MapY = 458.0F;
constexpr float HostY = 526.0F;
constexpr float KeyRepeatDelay = 0.35F;
constexpr float KeyRepeatRate = 0.04F;
constexpr const char* LanServerHost = "127.0.0.1";
constexpr const char* ProductionServerHost = "tiny-troopers.mzecheru.com";

float UiScale() { return static_cast<float>(GetScreenWidth()) / 1280.0f; }
float Sf(float x) { return x * UiScale(); }
int Si(float x) { return static_cast<int>(x * UiScale()); }

bool Hit(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawChoice(Rectangle bounds, const char* text, bool active, int fontSize = TextSize) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRounded(bounds, 0.06F, 8, active ? ActiveColor : (hover ? Color{68, 82, 101, 255} : ButtonColor));
    const float offsetY = (fontSize < TextSize) ? 12.0F : 13.0F;
    DrawText(text, static_cast<int>(bounds.x + Sf(14.0F)), static_cast<int>(bounds.y + Sf(offsetY)), Si(static_cast<float>(fontSize)), TextColor);
}

void DrawClassChoice(Rectangle bounds, const char* text, PlayerClass playerClass, bool active) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRounded(bounds, 0.06F, 8, active ? ActiveColor : (hover ? Color{68, 82, 101, 255} : ButtonColor));
    DrawText(text, static_cast<int>(bounds.x + Sf(14.0F)), static_cast<int>(bounds.y + Sf(7.0F)), Si(18), TextColor);
    const int frags = ClassMaxGrenades(playerClass);
    const int barriers = ClassMaxBarriers(playerClass);
    DrawText(TextFormat("%d Frag%s  %d Barrier%s", frags, frags == 1 ? "" : "s", barriers, barriers == 1 ? "" : "s"),
        static_cast<int>(bounds.x + Sf(14.0F)), static_cast<int>(bounds.y + Sf(28.0F)), Si(12), active ? Color{200, 245, 220, 255} : MutedColor);
}

void DrawTextField(Rectangle bounds, const char* text, bool active) {
    DrawRectangleRounded(bounds, 0.05F, 8, FieldColor);
    DrawRectangleLinesEx(bounds, active ? 2.0F : 1.0F, active ? Color{92, 168, 135, 255} : Color{78, 96, 118, 255});
    DrawText(text, static_cast<int>(bounds.x + Sf(14.0F)), static_cast<int>(bounds.y + Sf(11.0F)), Si(TextSize), TextColor);
}

bool IsValidNameChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-';
}

void PushChar(char* buffer, std::size_t capacity, char value) {
    const std::size_t length = std::strlen(buffer);
    if (length + 1 < capacity) {
        buffer[length] = value;
        buffer[length + 1] = '\0';
    }
}

void PopChar(char* buffer) {
    const std::size_t length = std::strlen(buffer);
    if (length > 0) {
        buffer[length - 1] = '\0';
    }
}
}

RoomCreationScreen::RoomCreationScreen()
    : nameBuffer{},
      nameFocus(false),
      deleteHoldTimer(0.0F),
      deleteRepeatTimer(0.0F),
      initialized(false) {}

void RoomCreationScreen::Update(GameClient& client) {
    if (!initialized) {
        initialized = true;
        const std::string& currentName = client.PlayerName();
        if (!currentName.empty()) {
            std::strncpy(nameBuffer, currentName.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        } else {
            std::strncpy(nameBuffer, "Player 1", sizeof(nameBuffer) - 1);
        }
        if (client.ServerHost().empty() || (client.ServerHost() != ProductionServerHost && client.ServerHost() != LanServerHost)) {
            client.SetServerHost(LanServerHost);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        ScreenManager::Set(ScreenId::MainMenu);
    }
    if (client.ConsumeCreatedRoomConfirmation()) {
        ScreenManager::Set(ScreenId::Gameplay);
        return;
    }

    const Rectangle nameFieldRect{Sf(RowX), Sf(NameY), Sf(cfg::ButtonWidth), Sf(42.0F)};
    if (Hit(nameFieldRect)) {
        nameFocus = true;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        nameFocus = false;
    }

    const Rectangle lanBtnRect{Sf(RowX + 360.0F), Sf(NameY), Sf(150.0F), Sf(42.0F)};
    const Rectangle prodBtnRect{Sf(RowX + 525.0F), Sf(NameY), Sf(295.0F), Sf(42.0F)};
    if (Hit(lanBtnRect)) { client.SetServerHost(LanServerHost); }
    if (Hit(prodBtnRect)) { client.SetServerHost(ProductionServerHost); }

    if (nameFocus) {
        int key = GetCharPressed();
        while (key > 0) {
            const char typed = static_cast<char>(key);
            if (IsValidNameChar(typed) && std::strlen(nameBuffer) < 16) {
                PushChar(nameBuffer, sizeof(nameBuffer), typed);
            }
            key = GetCharPressed();
        }

        const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                              IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
        const bool deleteDown = IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE);
        const bool deletePressed = IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE);
        const float dt = GetFrameTime();

        if (deletePressed) {
            if (ctrlDown) {
                nameBuffer[0] = '\0';
            } else {
                PopChar(nameBuffer);
            }
            deleteHoldTimer = 0.0F;
            deleteRepeatTimer = 0.0F;
        } else if (deleteDown) {
            deleteHoldTimer += dt;
            if (deleteHoldTimer >= KeyRepeatDelay) {
                deleteRepeatTimer += dt;
                while (deleteRepeatTimer >= KeyRepeatRate) {
                    if (ctrlDown) {
                        nameBuffer[0] = '\0';
                    } else {
                        PopChar(nameBuffer);
                    }
                    deleteRepeatTimer -= KeyRepeatRate;
                }
            }
        } else {
            deleteHoldTimer = 0.0F;
            deleteRepeatTimer = 0.0F;
        }
    }

    if (Hit(Rectangle{Sf(RowX), Sf(ModeY), Sf(MediumWidth), Sf(cfg::ButtonHeight)})) { client.SetMode(GameMode::FfaTimed); }
    if (Hit(Rectangle{Sf(RowX + 270.0F), Sf(ModeY), Sf(MediumWidth), Sf(cfg::ButtonHeight)})) { client.SetMode(GameMode::FfaScore); }
    if (Hit(Rectangle{Sf(RowX), Sf(ModeY + 56.0F), Sf(MediumWidth), Sf(cfg::ButtonHeight)})) { client.SetMode(GameMode::TdmTimed); }
    if (Hit(Rectangle{Sf(RowX + 270.0F), Sf(ModeY + 56.0F), Sf(MediumWidth), Sf(cfg::ButtonHeight)})) { client.SetMode(GameMode::TdmScore); }

    if (Hit(Rectangle{Sf(RowX), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Ak47); }
    if (Hit(Rectangle{Sf(RowX + 210.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Pistol); }
    if (Hit(Rectangle{Sf(RowX + 420.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Sniper); }
    if (Hit(Rectangle{Sf(RowX + 630.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Shotgun); }

    if (Hit(Rectangle{Sf(RowX), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetMap(0); }
    if (Hit(Rectangle{Sf(RowX + 210.0F), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetMap(1); }
    if (Hit(Rectangle{Sf(RowX + 420.0F), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetMap(2); }

    const bool hostPressed = Hit(Rectangle{Sf(RowX), Sf(HostY), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)}) || (!nameFocus && IsKeyPressed(KEY_ENTER));
    if (hostPressed) {
        std::string trimmedName = nameBuffer;
        while (!trimmedName.empty() && (trimmedName.back() == ' ' || trimmedName.back() == '\t')) {
            trimmedName.pop_back();
        }
        std::size_t start = 0;
        while (start < trimmedName.size() && (trimmedName[start] == ' ' || trimmedName[start] == '\t')) {
            ++start;
        }
        if (start > 0) {
            trimmedName = trimmedName.substr(start);
        }
        if (trimmedName.empty()) {
            trimmedName = "Player 1";
        }
        client.SetPlayerName(trimmedName);
        client.CreateRoom();
    }
}

void RoomCreationScreen::Draw(GameClient& client) const {
    DrawText("Create Room", static_cast<int>(Sf(RowX)), static_cast<int>(Sf(HeaderY)), Si(HeaderSize), TextColor);

    DrawText("Username", static_cast<int>(Sf(RowX)), static_cast<int>(Sf(118.0F)), Si(TextSize), MutedColor);
    DrawTextField(Rectangle{Sf(RowX), Sf(NameY), Sf(cfg::ButtonWidth), Sf(42.0F)}, nameBuffer, nameFocus);

    const bool isLan = (client.ServerHost() != ProductionServerHost);
    DrawText("Host Location", static_cast<int>(Sf(RowX + 360.0F)), static_cast<int>(Sf(118.0F)), Si(TextSize), MutedColor);
    DrawChoice(Rectangle{Sf(RowX + 360.0F), Sf(NameY), Sf(150.0F), Sf(42.0F)}, "LAN", isLan);
    DrawChoice(Rectangle{Sf(RowX + 525.0F), Sf(NameY), Sf(295.0F), Sf(42.0F)}, "tiny-troopers.mzecheru.com", !isLan, 17);

    DrawText("Mode", static_cast<int>(Sf(RowX)), static_cast<int>(Sf(198.0F)), Si(TextSize), MutedColor);
    DrawChoice(Rectangle{Sf(RowX), Sf(ModeY), Sf(MediumWidth), Sf(cfg::ButtonHeight)}, "FFA Timed", client.SelectedMode() == GameMode::FfaTimed);
    DrawChoice(Rectangle{Sf(RowX + 270.0F), Sf(ModeY), Sf(MediumWidth), Sf(cfg::ButtonHeight)}, "FFA Score", client.SelectedMode() == GameMode::FfaScore);
    DrawChoice(Rectangle{Sf(RowX), Sf(ModeY + 56.0F), Sf(MediumWidth), Sf(cfg::ButtonHeight)}, "TDM Timed", client.SelectedMode() == GameMode::TdmTimed);
    DrawChoice(Rectangle{Sf(RowX + 270.0F), Sf(ModeY + 56.0F), Sf(MediumWidth), Sf(cfg::ButtonHeight)}, "TDM Score", client.SelectedMode() == GameMode::TdmScore);

    const PlayerClass selClass = client.SelectedClass();
    DrawText("Class", static_cast<int>(Sf(RowX)), static_cast<int>(Sf(336.0F)), Si(TextSize), MutedColor);
    DrawText(TextFormat("Select %s", ClassName(selClass)),
        static_cast<int>(Sf(RowX + 80.0F)), static_cast<int>(Sf(338.0F)), Si(16), Color{190, 230, 215, 255});

    DrawClassChoice(Rectangle{Sf(RowX), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "AK47", PlayerClass::Ak47, client.SelectedClass() == PlayerClass::Ak47);
    DrawClassChoice(Rectangle{Sf(RowX + 210.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "M1911", PlayerClass::Pistol, client.SelectedClass() == PlayerClass::Pistol);
    DrawClassChoice(Rectangle{Sf(RowX + 420.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "KAR98k", PlayerClass::Sniper, client.SelectedClass() == PlayerClass::Sniper);
    DrawClassChoice(Rectangle{Sf(RowX + 630.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "Mossberg", PlayerClass::Shotgun, client.SelectedClass() == PlayerClass::Shotgun);

    DrawText("Map", static_cast<int>(Sf(RowX)), static_cast<int>(Sf(432.0F)), Si(TextSize), MutedColor);
    DrawChoice(Rectangle{Sf(RowX), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "Map 1", client.SelectedMap() == 0);
    DrawChoice(Rectangle{Sf(RowX + 210.0F), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "Map 2", client.SelectedMap() == 1);
    DrawChoice(Rectangle{Sf(RowX + 420.0F), Sf(MapY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "Map 3", client.SelectedMap() == 2);

    DrawChoice(Rectangle{Sf(RowX), Sf(HostY), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)}, "Host Room", false);
    DrawText("Esc returns to menu", static_cast<int>(Sf(RowX)), static_cast<int>(GetScreenHeight() - Sf(48.0F)), Si(TextSize), MutedColor);
    DrawText(client.Status().c_str(), static_cast<int>(Sf(RowX + 370.0F)), static_cast<int>(Sf(HostY + 13.0F)), Si(TextSize), MutedColor);
}
