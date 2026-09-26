#include "client/RoomJoiningScreen.hpp"

#include "client/GameClient.hpp"
#include "client/ScreenManager.hpp"
#include "common/Constants.hpp"

#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <string>

namespace {
constexpr int HeaderSize = 34;
constexpr int TextSize = 20;
constexpr int RoomCodeSize = 36;
constexpr Color TextColor{238, 240, 244, 255};
constexpr Color MutedColor{158, 168, 177, 255};
constexpr Color ButtonColor{54, 66, 82, 255};
constexpr Color ActiveColor{48, 118, 96, 255};
constexpr Color FieldColor{35, 42, 52, 255};
constexpr float X = 58.0F;
constexpr float SmallWidth = 190.0F;
constexpr float ServerFieldWidth = 480.0F;
constexpr float FieldHeight = 42.0F;
constexpr float NameY = 114.0F;
constexpr float ServerY = 192.0F;
constexpr float RoomCodeY = 270.0F;
constexpr float ClassY = 360.0F;
constexpr float JoinBtnY = 432.0F;
constexpr float CodeCellWidth = 58.0F;
constexpr float CodeCellHeight = 50.0F;
constexpr float CodeCellGap = 10.0F;
constexpr const char* DefaultServerHost = "127.0.0.1";
constexpr const char* ProductionServerHost = "https://tiny-troopers.mzecheru.com";
constexpr const char* SavedServersFile = "saved_servers.txt";
constexpr float DropdownBtnWidth = 34.0F;
constexpr float DropdownBtnHeight = 34.0F;
constexpr float DropdownItemHeight = 38.0F;
constexpr float KeyRepeatDelay = 0.35F;
constexpr float KeyRepeatRate = 0.04F;

float UiScale() { return static_cast<float>(GetScreenWidth()) / 1280.0f; }
float Sf(float x) { return x * UiScale(); }
int Si(float x) { return static_cast<int>(x * UiScale()); }

void DrawCodeGlyph(char value, Rectangle cell) {
    const int thickness = std::max(1, Si(5.0F));
    const int left = static_cast<int>(cell.x + Sf(12.0F));
    const int top = static_cast<int>(cell.y + Sf(7.0F));
    const int width = static_cast<int>(cell.width - Sf(24.0F));
    const int height = static_cast<int>(cell.height - Sf(14.0F));
    const int midY = top + height / 2;
    const int right = left + width;
    const int bottom = top + height;

    auto h = [&](int y) { DrawLineEx(Vector2{static_cast<float>(left), static_cast<float>(y)}, Vector2{static_cast<float>(right), static_cast<float>(y)}, static_cast<float>(thickness), TextColor); };
    auto v = [&](int x, int y1, int y2) { DrawLineEx(Vector2{static_cast<float>(x), static_cast<float>(y1)}, Vector2{static_cast<float>(x), static_cast<float>(y2)}, static_cast<float>(thickness), TextColor); };

    const char segments = value;
    switch (segments) {
    case '0': h(top); h(bottom); v(left, top, bottom); v(right, top, bottom); break;
    case '1': v(right, top, bottom); break;
    case '2': h(top); h(midY); h(bottom); v(right, top, midY); v(left, midY, bottom); break;
    case '3': h(top); h(midY); h(bottom); v(right, top, bottom); break;
    case '4': h(midY); v(left, top, midY); v(right, top, bottom); break;
    case '5': h(top); h(midY); h(bottom); v(left, top, midY); v(right, midY, bottom); break;
    case '6': h(top); h(midY); h(bottom); v(left, top, bottom); v(right, midY, bottom); break;
    case '7': h(top); v(right, top, bottom); break;
    case '8': h(top); h(midY); h(bottom); v(left, top, bottom); v(right, top, bottom); break;
    case '9': h(top); h(midY); h(bottom); v(left, top, midY); v(right, top, bottom); break;
    default:
        {
            const char text[2] = {value, '\0'};
            const int scaledCodeSize = Si(RoomCodeSize);
            const int textWidth = MeasureText(text, scaledCodeSize);
            DrawText(text,
                static_cast<int>(cell.x + (cell.width - static_cast<float>(textWidth)) * 0.5F),
                static_cast<int>(cell.y + (cell.height - static_cast<float>(scaledCodeSize)) * 0.5F - Sf(1.0F)),
                scaledCodeSize,
                TextColor);
        }
        break;
    }
}

bool Hit(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawButton(Rectangle bounds, const char* text) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRounded(bounds, 0.06F, 8, hover ? Color{64, 142, 116, 255} : ActiveColor);
    DrawText(text, static_cast<int>(bounds.x + Sf(16.0F)), static_cast<int>(bounds.y + Sf(13.0F)), Si(TextSize), TextColor);
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
    const int scaledSize = (std::strlen(text) > 22) ? Si(16) : Si(TextSize);
    const float offsetY = (std::strlen(text) > 22) ? 13.0F : 11.0F;
    DrawText(text, static_cast<int>(bounds.x + Sf(14.0F)), static_cast<int>(bounds.y + Sf(offsetY)), scaledSize, TextColor);
}

void DrawDropdownGlyph(Rectangle bounds, bool open) {
    const float midX = bounds.x + bounds.width * 0.5F;
    const float midY = bounds.y + bounds.height * 0.5F;
    const float triSize = Sf(6.0F);
    const float triOffset = Sf(3.0F);
    if (open) {
        DrawTriangle(
            Vector2{midX - triSize, midY + triOffset},
            Vector2{midX + triSize, midY + triOffset},
            Vector2{midX, midY - triOffset},
            TextColor);
    } else {
        DrawTriangle(
            Vector2{midX - triSize, midY - triOffset},
            Vector2{midX, midY + triOffset},
            Vector2{midX + triSize, midY - triOffset},
            TextColor);
    }
}

bool IsServerHostChar(char value) {
    return std::isalnum(static_cast<unsigned char>(value)) || value == '.' || value == '-' || value == ':' || value == '/';
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

void RoomJoiningScreen::LoadServerHistory() {
    savedServers.clear();
    std::ifstream file(SavedServersFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            std::size_t start = 0;
            while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                ++start;
            }
            if (start > 0) {
                line = line.substr(start);
            }
            if (!line.empty() && line.size() < sizeof(serverHostBuffer)) {
                if (std::find(savedServers.begin(), savedServers.end(), line) == savedServers.end()) {
                    savedServers.push_back(line);
                    if (savedServers.size() >= 10) {
                        break;
                    }
                }
            }
        }
    }
    if (std::find(savedServers.begin(), savedServers.end(), DefaultServerHost) == savedServers.end()) {
        savedServers.insert(savedServers.begin(), DefaultServerHost);
    }
    if (std::find(savedServers.begin(), savedServers.end(), ProductionServerHost) == savedServers.end()) {
        savedServers.push_back(ProductionServerHost);
    }
}

void RoomJoiningScreen::SaveServerHistory() const {
    std::ofstream file(SavedServersFile);
    if (file.is_open()) {
        for (const auto& server : savedServers) {
            file << server << "\n";
        }
    }
}

void RoomJoiningScreen::AddSavedServer(const std::string& host) {
    if (host.empty()) {
        return;
    }
    auto it = std::find(savedServers.begin(), savedServers.end(), host);
    if (it != savedServers.end()) {
        savedServers.erase(it);
    }
    savedServers.insert(savedServers.begin(), host);
    if (savedServers.size() > 10) {
        savedServers.resize(10);
    }
    SaveServerHistory();
}

RoomJoiningScreen::RoomJoiningScreen()
    : nameBuffer{},
      roomCodeBuffer{},
      serverHostBuffer{},
      focus(FieldFocus::Name),
      deleteHoldTimer(0.0F),
      deleteRepeatTimer(0.0F),
      dropdownOpen(false),
      initialized(false),
      savedServers{} {
    LoadServerHistory();
    std::strncpy(serverHostBuffer, DefaultServerHost, sizeof(serverHostBuffer) - 1);
    serverHostBuffer[sizeof(serverHostBuffer) - 1] = '\0';
}

void RoomJoiningScreen::Update(GameClient& client) {
    if (!initialized) {
        initialized = true;
        const std::string& currentName = client.PlayerName();
        if (!currentName.empty() && currentName != "Player 1") {
            std::strncpy(nameBuffer, currentName.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        } else if (currentName == "Player 1") {
            std::strncpy(nameBuffer, "Player 2", sizeof(nameBuffer) - 1);
        } else {
            std::strncpy(nameBuffer, "Player 2", sizeof(nameBuffer) - 1);
        }
    }

    if (dropdownOpen && IsKeyPressed(KEY_ESCAPE)) {
        dropdownOpen = false;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        ScreenManager::Set(ScreenId::MainMenu);
        return;
    }
    if (client.ConsumeJoinedRoomConfirmation()) {
        ScreenManager::Set(ScreenId::Gameplay);
        return;
    }

    const Vector2 mouse = GetMousePosition();
    const Rectangle nameField{Sf(X), Sf(NameY), Sf(ServerFieldWidth), Sf(FieldHeight)};
    const Rectangle serverField{Sf(X), Sf(ServerY), Sf(ServerFieldWidth), Sf(FieldHeight)};
    const Rectangle dropdownBtn{Sf(X + ServerFieldWidth - DropdownBtnWidth - 6.0F), Sf(ServerY) + (Sf(FieldHeight) - Sf(DropdownBtnHeight)) * 0.5F, Sf(DropdownBtnWidth), Sf(DropdownBtnHeight)};
    const float menuHeight = static_cast<float>(savedServers.size()) * Sf(DropdownItemHeight);
    const Rectangle menuRect{Sf(X), Sf(ServerY + FieldHeight + 4.0F), Sf(ServerFieldWidth), menuHeight};
    const Rectangle roomField{Sf(X), Sf(RoomCodeY), Sf(ServerFieldWidth), Sf(54.0F)};

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (dropdownOpen) {
            if (CheckCollisionPointRec(mouse, menuRect)) {
                const int clickedIndex = static_cast<int>((mouse.y - menuRect.y) / Sf(DropdownItemHeight));
                if (clickedIndex >= 0 && clickedIndex < static_cast<int>(savedServers.size())) {
                    std::strncpy(serverHostBuffer, savedServers[static_cast<std::size_t>(clickedIndex)].c_str(), sizeof(serverHostBuffer) - 1);
                    serverHostBuffer[sizeof(serverHostBuffer) - 1] = '\0';
                    focus = FieldFocus::RoomCode;
                }
                dropdownOpen = false;
                return;
            }
            if (CheckCollisionPointRec(mouse, dropdownBtn)) {
                dropdownOpen = false;
                return;
            }
            dropdownOpen = false;
        } else {
            if (CheckCollisionPointRec(mouse, dropdownBtn)) {
                dropdownOpen = true;
                return;
            }
        }
    }

    if (Hit(nameField)) {
        focus = FieldFocus::Name;
    }
    if (Hit(serverField) && !CheckCollisionPointRec(mouse, dropdownBtn)) {
        focus = FieldFocus::ServerHost;
    }
    if (Hit(roomField)) {
        focus = FieldFocus::RoomCode;
    }
    if (IsKeyPressed(KEY_TAB)) {
        if (focus == FieldFocus::Name) {
            focus = FieldFocus::ServerHost;
        } else if (focus == FieldFocus::ServerHost) {
            focus = FieldFocus::RoomCode;
        } else {
            focus = FieldFocus::Name;
        }
    }

    int key = GetCharPressed();
    while (key > 0) {
        const char typed = static_cast<char>(key);
        if (focus == FieldFocus::Name && IsValidNameChar(typed) && std::strlen(nameBuffer) < 16) {
            PushChar(nameBuffer, sizeof(nameBuffer), typed);
        }
        if (focus == FieldFocus::ServerHost && IsServerHostChar(typed)) {
            PushChar(serverHostBuffer, sizeof(serverHostBuffer), typed);
        }
        if (focus == FieldFocus::RoomCode && typed >= '0' && typed <= '9' && std::strlen(roomCodeBuffer) < cfg::RoomCodeLength) {
            PushChar(roomCodeBuffer, cfg::RoomCodeBytes, typed);
        }
        key = GetCharPressed();
    }

    const bool deleteDown = IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE);
    const bool deletePressed = IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE);

    auto doDelete = [&]() {
        if (focus == FieldFocus::Name) {
            PopChar(nameBuffer);
        } else if (focus == FieldFocus::ServerHost) {
            PopChar(serverHostBuffer);
        } else {
            PopChar(roomCodeBuffer);
        }
    };

    const float dt = GetFrameTime();
    if (deletePressed) {
        doDelete();
        deleteHoldTimer = 0.0F;
        deleteRepeatTimer = 0.0F;
    } else if (deleteDown) {
        deleteHoldTimer += dt;
        if (deleteHoldTimer >= KeyRepeatDelay) {
            deleteRepeatTimer += dt;
            while (deleteRepeatTimer >= KeyRepeatRate) {
                doDelete();
                deleteRepeatTimer -= KeyRepeatRate;
            }
        }
    } else {
        deleteHoldTimer = 0.0F;
        deleteRepeatTimer = 0.0F;
    }

    if (Hit(Rectangle{Sf(X), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Ak47); }
    if (Hit(Rectangle{Sf(X + 210.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Pistol); }
    if (Hit(Rectangle{Sf(X + 420.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Sniper); }
    if (Hit(Rectangle{Sf(X + 630.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)})) { client.SetClass(PlayerClass::Shotgun); }

    const bool joinPressed = Hit(Rectangle{Sf(X), Sf(JoinBtnY), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)}) || IsKeyPressed(KEY_ENTER);
    if (joinPressed) {
        if (std::strlen(serverHostBuffer) == 0) {
            client.ShowMessage("Enter the server IP address.");
        } else if (std::strlen(roomCodeBuffer) == cfg::RoomCodeLength) {
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
                trimmedName = "Player 2";
            }
            client.SetPlayerName(trimmedName);
            AddSavedServer(serverHostBuffer);
            client.SetServerHost(serverHostBuffer);
            client.JoinRoom(roomCodeBuffer);
        } else {
            client.ShowMessage("Enter a 4-digit room code.");
        }
    }
}

void RoomJoiningScreen::Draw(GameClient& client) const {
    const Vector2 mouse = GetMousePosition();
    DrawText("Join Room", static_cast<int>(Sf(X)), static_cast<int>(Sf(48.0F)), Si(HeaderSize), TextColor);

    DrawText("Username", static_cast<int>(Sf(X)), static_cast<int>(Sf(90.0F)), Si(TextSize), MutedColor);
    DrawTextField(Rectangle{Sf(X), Sf(NameY), Sf(ServerFieldWidth), Sf(FieldHeight)}, nameBuffer, focus == FieldFocus::Name);

    DrawText("Server URL / IP", static_cast<int>(Sf(X)), static_cast<int>(Sf(168.0F)), Si(TextSize), MutedColor);
    DrawTextField(Rectangle{Sf(X), Sf(ServerY), Sf(ServerFieldWidth), Sf(FieldHeight)}, serverHostBuffer, focus == FieldFocus::ServerHost);

    const Rectangle dropdownBtn{Sf(X + ServerFieldWidth - DropdownBtnWidth - 6.0F), Sf(ServerY) + (Sf(FieldHeight) - Sf(DropdownBtnHeight)) * 0.5F, Sf(DropdownBtnWidth), Sf(DropdownBtnHeight)};
    const bool btnHover = CheckCollisionPointRec(mouse, dropdownBtn);
    DrawRectangleRounded(dropdownBtn, 0.15F, 6, btnHover ? Color{64, 82, 100, 255} : Color{44, 56, 70, 255});
    DrawDropdownGlyph(dropdownBtn, dropdownOpen);

    DrawText("Room Code", static_cast<int>(Sf(X)), static_cast<int>(Sf(246.0F)), Si(TextSize), MutedColor);
    DrawRectangleRounded(Rectangle{Sf(X), Sf(RoomCodeY), Sf(ServerFieldWidth), Sf(54.0F)}, 0.04F, 8, FieldColor);
    const float totalCellsWidth = static_cast<float>(cfg::RoomCodeLength) * Sf(CodeCellWidth) + static_cast<float>(cfg::RoomCodeLength - 1) * Sf(CodeCellGap);
    const float cellStartX = Sf(X) + (Sf(ServerFieldWidth) - totalCellsWidth) * 0.5F;
    for (std::size_t index = 0; index < cfg::RoomCodeLength; ++index) {
        const Rectangle cell{
            cellStartX + static_cast<float>(index) * (Sf(CodeCellWidth) + Sf(CodeCellGap)),
            Sf(RoomCodeY) + Sf(4.0F),
            Sf(CodeCellWidth),
            Sf(46.0F)
        };
        DrawRectangleLinesEx(cell, focus == FieldFocus::RoomCode ? 2.0F : 1.0F, focus == FieldFocus::RoomCode ? Color{92, 168, 135, 255} : Color{78, 96, 118, 255});
        if (index < std::strlen(roomCodeBuffer)) {
            DrawCodeGlyph(roomCodeBuffer[index], cell);
        }
    }

    const PlayerClass selClass = client.SelectedClass();
    DrawText("Class", static_cast<int>(Sf(X)), static_cast<int>(Sf(336.0F)), Si(TextSize), MutedColor);
    DrawText(TextFormat("Select %s", ClassName(selClass)),
        static_cast<int>(Sf(X + 80.0F)), static_cast<int>(Sf(338.0F)), Si(16), Color{190, 230, 215, 255});

    DrawClassChoice(Rectangle{Sf(X), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "AK47", PlayerClass::Ak47, selClass == PlayerClass::Ak47);
    DrawClassChoice(Rectangle{Sf(X + 210.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "M1911", PlayerClass::Pistol, selClass == PlayerClass::Pistol);
    DrawClassChoice(Rectangle{Sf(X + 420.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "KAR98k", PlayerClass::Sniper, selClass == PlayerClass::Sniper);
    DrawClassChoice(Rectangle{Sf(X + 630.0F), Sf(ClassY), Sf(SmallWidth), Sf(cfg::ButtonHeight)}, "Mossberg", PlayerClass::Shotgun, selClass == PlayerClass::Shotgun);

    DrawButton(Rectangle{Sf(X), Sf(JoinBtnY), Sf(cfg::ButtonWidth), Sf(cfg::ButtonHeight)}, "Join Room");
    DrawText("Use your friend's LAN IP, then enter the room code.", static_cast<int>(Sf(X)), static_cast<int>(Sf(496.0F)), Si(TextSize), MutedColor);
    DrawText("Esc returns to menu", static_cast<int>(Sf(X)), static_cast<int>(GetScreenHeight() - Sf(48.0F)), Si(TextSize), MutedColor);
    DrawText(client.Status().c_str(), static_cast<int>(Sf(X + 370.0F)), static_cast<int>(Sf(JoinBtnY + 13.0F)), Si(TextSize), MutedColor);

    if (dropdownOpen && !savedServers.empty()) {
        const float menuHeight = static_cast<float>(savedServers.size()) * Sf(DropdownItemHeight);
        const Rectangle menuRect{Sf(X), Sf(ServerY + FieldHeight + 4.0F), Sf(ServerFieldWidth), menuHeight};
        DrawRectangleRounded(menuRect, 0.04F, 8, Color{24, 30, 38, 255});
        DrawRectangleLinesEx(menuRect, 2.0F, Color{78, 96, 118, 255});

        for (std::size_t i = 0; i < savedServers.size(); ++i) {
            const Rectangle itemRect{menuRect.x, menuRect.y + static_cast<float>(i) * Sf(DropdownItemHeight), menuRect.width, Sf(DropdownItemHeight)};
            const bool itemHover = CheckCollisionPointRec(mouse, itemRect);
            if (itemHover) {
                DrawRectangleRec(Rectangle{itemRect.x + 2.0F, itemRect.y + 2.0F, itemRect.width - 4.0F, itemRect.height - 4.0F}, Color{44, 78, 66, 255});
            }
            const int itemTextSize = (savedServers[i].size() > 22) ? Si(16) : Si(TextSize);
            const float itemOffsetY = (savedServers[i].size() > 22) ? 11.0F : 10.0F;
            DrawText(savedServers[i].c_str(), static_cast<int>(itemRect.x + Sf(14.0F)), static_cast<int>(itemRect.y + Sf(itemOffsetY)), itemTextSize, TextColor);
            if (i > 0) {
                DrawLineEx(Vector2{itemRect.x + Sf(8.0F), itemRect.y}, Vector2{itemRect.x + itemRect.width - Sf(8.0F), itemRect.y}, 1.0F, Color{50, 60, 74, 255});
            }
        }
    }
}
