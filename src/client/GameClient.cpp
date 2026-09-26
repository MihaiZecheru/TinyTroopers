#include "client/GameClient.hpp"

#include "client/ScreenManager.hpp"
#include "common/Constants.hpp"

#include "raylib.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace {
constexpr float HelpButtonSize = 42.0F;
constexpr float HelpButtonMargin = 14.0F;
constexpr float HeaderBarHeight = 64.0F;
constexpr float HelpModalWidth = 540.0F;
constexpr float HelpModalHeight = 500.0F;
constexpr float HelpModalPadding = 28.0F;
constexpr float PopupWidth = 520.0F;
constexpr float PopupHeight = 160.0F;
constexpr float PopupButtonWidth = 120.0F;
constexpr float PopupButtonHeight = 38.0F;
constexpr int HelpTitleSize = 30;
constexpr int HelpTextSize = 20;
constexpr int PopupTitleSize = 24;
constexpr int PopupTextSize = 18;
constexpr Color HelpButtonColor{42, 52, 66, 235};
constexpr Color HelpButtonHoverColor{64, 82, 105, 245};
constexpr Color HelpPanelColor{28, 34, 43, 250};
constexpr Color HelpOverlayColor{0, 0, 0, 150};
constexpr Color HelpTextColor{238, 240, 244, 255};
constexpr Color HelpMutedColor{170, 180, 190, 255};

float UiScale() { return static_cast<float>(GetScreenWidth()) / 1280.0f; }
float Sf(float x) { return x * UiScale(); }
int Si(float x) { return static_cast<int>(x * UiScale()); }

std::vector<std::string> FormatMessageLines(const std::string& text, int maxLineWidth, int fontSize) {
    std::vector<std::string> result;
    std::istringstream stream(text);
    std::string rawLine;
    while (std::getline(stream, rawLine)) {
        if (rawLine.empty()) {
            result.push_back("");
            continue;
        }
        std::istringstream words(rawLine);
        std::string word;
        std::string currentLine;
        while (words >> word) {
            std::string testLine = currentLine.empty() ? word : (currentLine + " " + word);
            if (MeasureText(testLine.c_str(), fontSize) > maxLineWidth && !currentLine.empty()) {
                result.push_back(currentLine);
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
        if (!currentLine.empty()) {
            result.push_back(currentLine);
        }
    }
    if (result.empty()) {
        result.push_back("");
    }
    return result;
}

Rectangle MessageModalBounds(const std::string& message) {
    const int font = Si(PopupTextSize);
    const int maxInnerWidth = Si(560.0F);
    const auto lines = FormatMessageLines(message, maxInnerWidth, font);

    int maxMeasuredW = 0;
    for (const auto& line : lines) {
        maxMeasuredW = std::max(maxMeasuredW, MeasureText(line.c_str(), font));
    }

    const float modalWidth = std::max(Sf(PopupWidth), static_cast<float>(maxMeasuredW) + Sf(64.0F));
    const float contentHeight = static_cast<float>(lines.size()) * Sf(26.0F);
    const float modalHeight = std::max(Sf(PopupHeight), Sf(64.0F) + contentHeight + Sf(64.0F));
    return Rectangle{
        (static_cast<float>(GetScreenWidth()) - modalWidth) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - modalHeight) * 0.5F,
        modalWidth,
        modalHeight
    };
}

Rectangle HelpButtonBounds() {
    return Rectangle{
        Sf(HelpButtonMargin),
        Sf((HeaderBarHeight - HelpButtonSize) * 0.5F),
        Sf(HelpButtonSize),
        Sf(HelpButtonSize)
    };
}

Rectangle FullscreenButtonBounds() {
    return Rectangle{
        Sf(HelpButtonMargin + HelpButtonSize + 8.0F),
        Sf((HeaderBarHeight - HelpButtonSize) * 0.5F),
        Sf(HelpButtonSize),
        Sf(HelpButtonSize)
    };
}

void DrawFullscreenIcon(Rectangle bounds, bool isFullscreen, Color color) {
    const float pad = bounds.width * 0.28F;
    const float len = bounds.width * 0.18F;
    const float thick = std::max(1.5F, 2.0F * UiScale());
    const float x1 = bounds.x + pad;
    const float y1 = bounds.y + pad;
    const float x2 = bounds.x + bounds.width - pad;
    const float y2 = bounds.y + bounds.height - pad;

    if (!isFullscreen) {
        // Expand 4 corners outward
        DrawLineEx(Vector2{x1, y1}, Vector2{x1 + len, y1}, thick, color);
        DrawLineEx(Vector2{x1, y1}, Vector2{x1, y1 + len}, thick, color);
        DrawLineEx(Vector2{x2, y1}, Vector2{x2 - len, y1}, thick, color);
        DrawLineEx(Vector2{x2, y1}, Vector2{x2, y1 + len}, thick, color);
        DrawLineEx(Vector2{x1, y2}, Vector2{x1 + len, y2}, thick, color);
        DrawLineEx(Vector2{x1, y2}, Vector2{x1, y2 - len}, thick, color);
        DrawLineEx(Vector2{x2, y2}, Vector2{x2 - len, y2}, thick, color);
        DrawLineEx(Vector2{x2, y2}, Vector2{x2, y2 - len}, thick, color);
    } else {
        // Collapse 4 corners inward
        const float gap = bounds.width * 0.10F;
        DrawLineEx(Vector2{x1 + gap, y1 + gap}, Vector2{x1 + gap + len, y1 + gap}, thick, color);
        DrawLineEx(Vector2{x1 + gap, y1 + gap}, Vector2{x1 + gap, y1 + gap + len}, thick, color);
        DrawLineEx(Vector2{x2 - gap, y1 + gap}, Vector2{x2 - gap - len, y1 + gap}, thick, color);
        DrawLineEx(Vector2{x2 - gap, y1 + gap}, Vector2{x2 - gap, y1 + gap + len}, thick, color);
        DrawLineEx(Vector2{x1 + gap, y2 - gap}, Vector2{x1 + gap + len, y2 - gap}, thick, color);
        DrawLineEx(Vector2{x1 + gap, y2 - gap}, Vector2{x1 + gap, y2 - gap - len}, thick, color);
        DrawLineEx(Vector2{x2 - gap, y2 - gap}, Vector2{x2 - gap - len, y2 - gap}, thick, color);
        DrawLineEx(Vector2{x2 - gap, y2 - gap}, Vector2{x2 - gap, y2 - gap - len}, thick, color);
    }
}

bool IsFullscreenTogglePressed() {
    // F11 (Standard PC)
    if (IsKeyPressed(KEY_F11)) {
        return true;
    }
    // Alt + Enter / Option + Enter (Windows / macOS / Linux)
    if ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER)) {
        return true;
    }
    // Command + Enter (macOS)
    if ((IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) && IsKeyPressed(KEY_ENTER)) {
        return true;
    }
    // Command + F (macOS standard fullscreen shortcut)
    if ((IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) && IsKeyPressed(KEY_F)) {
        return true;
    }
    // Control + Command + F (macOS system fullscreen shortcut)
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) &&
        IsKeyPressed(KEY_F)) {
        return true;
    }
    return false;
}
}

GameClient::GameClient()
    : snapshot{},
      selectedMode(GameMode::FfaTimed),
      selectedClass(PlayerClass::Ak47),
      selectedTeam(TeamId::Red),
      selectedMap(0),
      serverHost("127.0.0.1"),
      playerName("Player 1"),
      status("Not connected."),
      popupMessage{},
      secondsSinceServerPacket(0.0F),
      connectTimer(0.0F),
      heartbeatAccumulator(0.0F),
      connecting(false),
      hostHint(false),
      joinedRoomConfirmed(false),
      createdRoomConfirmed(false),
      helpOpen(false),
      popupOpen(false),
      newSnapshotReceived(false) {
    LoadPlayerName();
}

int GameClient::Run() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_INFO);
    InitWindow(cfg::WindowWidth, cfg::WindowHeight, "TinyTroopers");
    SetExitKey(KEY_NULL);
    SetTargetFPS(cfg::TargetFps);
    assets.Load();
    audio.Load();
    ScreenManager screens;
    while (!WindowShouldClose()) {
        if (IsFullscreenTogglePressed()) {
            ToggleWindowedFullscreen();
        }
        const float dt = GetFrameTime();
        newSnapshotReceived = false;
        SnapshotPacket incoming{};
        if (network.PollSnapshot(incoming)) {
            snapshot = incoming;
            newSnapshotReceived = true;
            secondsSinceServerPacket = 0.0F;
            if (connecting) {
                connecting = false;
                connectTimer = 0.0F;
            }
        }
        std::string message;
        if (network.PollMessage(message)) {
            HandleServerMessage(message);
            secondsSinceServerPacket = 0.0F;
        }

        // Server timeout detection ("server going dark")
        if (network.IsConnected() && ScreenManager::Current() == ScreenId::Gameplay) {
            secondsSinceServerPacket += dt;
            if (secondsSinceServerPacket >= cfg::ServerTimeoutSeconds) {
                Disconnect("Connection to server lost.");
            }
        }

        // Join / Create room connection timeout
        if (connecting) {
            connectTimer += dt;
            if (connectTimer >= cfg::ConnectTimeoutSeconds) {
                connecting = false;
                connectTimer = 0.0F;
                network.Disconnect();
                status = "Failed to connect to server.";
                ShowMessage("Failed to reach server at " + serverHost);
            }
        }

        UpdateMessagePopup();
        UpdateHelpOverlay();
        if (!helpOpen && !popupOpen) {
            screens.Update(*this);
        } else if (network.IsConnected() && ScreenManager::Current() == ScreenId::Gameplay) {
            // Keep sending heartbeat input packets so server doesn't mark client as silent while reading help or popups
            heartbeatAccumulator += dt;
            if (heartbeatAccumulator >= cfg::ClientSendSeconds) {
                InputPacket packet{};
                packet.header = MakeHeader(PacketType::Input, sizeof(InputPacket));
                network.SendInput(packet);
                heartbeatAccumulator = 0.0F;
            }
        }
        BeginDrawing();
        ClearBackground(Color{22, 25, 29, 255});
        screens.Draw(*this);
        DrawHelpOverlay();
        DrawMessagePopup();
        EndDrawing();
    }
    LeaveRoom();
    CloseWindow();
    return 0;
}

NetworkClient& GameClient::Network() { return network; }
const NetworkClient& GameClient::Network() const { return network; }
AssetManager& GameClient::Assets() { return assets; }
const AssetManager& GameClient::Assets() const { return assets; }
AudioManager& GameClient::Audio() { return audio; }
const AudioManager& GameClient::Audio() const { return audio; }
SnapshotPacket& GameClient::Snapshot() { return snapshot; }
const SnapshotPacket& GameClient::Snapshot() const { return snapshot; }

LobbyConfigPacket GameClient::MakeConfig() const {
    LobbyConfigPacket packet{};
    packet.header = MakeHeader(PacketType::LobbyConfig, sizeof(LobbyConfigPacket));
    packet.mode = selectedMode;
    packet.playerClass = selectedClass;
    packet.team = selectedTeam;
    packet.mapIndex = selectedMap;
    packet.scoreLimit = cfg::DefaultScoreLimit;
    packet.minutes = cfg::DefaultMatchMinutes;
    return packet;
}

PlayerSnapshot GameClient::LocalPlayer() const {
    for (std::size_t index = 0; index < snapshot.playerCount; ++index) {
        if (snapshot.players[index].id == snapshot.localPlayerId) {
            return snapshot.players[index];
        }
    }
    return PlayerSnapshot{};
}

void GameClient::SetMode(GameMode value) { selectedMode = value; }
void GameClient::SetClass(PlayerClass value) { selectedClass = value; }
void GameClient::SetTeam(TeamId value) { selectedTeam = value; }
void GameClient::SetMap(std::uint8_t value) { selectedMap = value; }
GameMode GameClient::SelectedMode() const { return selectedMode; }
PlayerClass GameClient::SelectedClass() const { return selectedClass; }
std::uint8_t GameClient::SelectedMap() const { return selectedMap; }
void GameClient::SetServerHost(const std::string& value) { serverHost = value; }
const std::string& GameClient::ServerHost() const { return serverHost; }
void GameClient::SetPlayerName(const std::string& value) { playerName = value; }
const std::string& GameClient::PlayerName() const { return playerName; }

void GameClient::LoadPlayerName() {
    std::ifstream file("saved_name.txt");
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t' || line.back() == '\n')) {
                line.pop_back();
            }
            std::size_t start = 0;
            while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                ++start;
            }
            if (start > 0) {
                line = line.substr(start);
            }
            if (!line.empty() && line.size() < static_cast<std::size_t>(cfg::NameBytes)) {
                playerName = line;
                return;
            }
        }
    }
    playerName = "Player 1";
}

void GameClient::SavePlayerName() const {
    if (playerName.empty()) {
        return;
    }
    std::ofstream file("saved_name.txt");
    if (file.is_open()) {
        file << playerName << "\n";
    }
}

const std::string& GameClient::Status() const { return status; }
void GameClient::SetStatus(const std::string& value) { status = value; }
const std::string& GameClient::RoomCode() const { return roomCode; }
bool GameClient::IsHost() const { return hostHint || LocalPlayer().host; }
void GameClient::ShowMessage(const std::string& value) {
    popupMessage = value;
    popupOpen = true;
}
bool GameClient::HasNewSnapshot() const { return newSnapshotReceived; }

void GameClient::CreateRoom() {
    SavePlayerName();
    roomCode.clear();
    createdRoomConfirmed = false;
    hostHint = false;
    connecting = true;
    connectTimer = 0.0F;
    secondsSinceServerPacket = 0.0F;
    const std::string nameToSend = playerName.empty() ? "Player 1" : playerName;
    status = network.Connect(serverHost, cfg::ServerPort, nameToSend, "", true, true) ? "Creating room..." : "Failed to open UDP socket.";
    network.SendConfig(MakeConfig());
}

void GameClient::JoinRoom(const std::string& roomCodeInput) {
    SavePlayerName();
    roomCode = roomCodeInput;
    joinedRoomConfirmed = false;
    hostHint = false;
    connecting = true;
    connectTimer = 0.0F;
    secondsSinceServerPacket = 0.0F;
    const std::string nameToSend = playerName.empty() ? "Player 2" : playerName;
    status = network.Connect(serverHost, cfg::ServerPort, nameToSend, roomCodeInput, false, true) ? "Joining room..." : "Failed to open UDP socket.";
    network.SendConfig(MakeConfig());
}

void GameClient::LeaveRoom() {
    if (network.IsConnected()) {
        network.Disconnect(LocalPlayer().id);
    }
    snapshot = SnapshotPacket{};
    roomCode.clear();
    hostHint = false;
    joinedRoomConfirmed = false;
    createdRoomConfirmed = false;
    connecting = false;
    connectTimer = 0.0F;
    secondsSinceServerPacket = 0.0F;
    status = "Disconnected.";
    ScreenManager::Set(ScreenId::MainMenu);
}

void GameClient::Disconnect(const std::string& reason) {
    LeaveRoom();
    status = reason;
    ShowMessage(reason);
}

bool GameClient::ConsumeJoinedRoomConfirmation() {
    const bool result = joinedRoomConfirmed;
    if (result) {
        connecting = false;
        connectTimer = 0.0F;
    }
    joinedRoomConfirmed = false;
    return result;
}

bool GameClient::ConsumeCreatedRoomConfirmation() {
    const bool result = createdRoomConfirmed;
    if (result) {
        connecting = false;
        connectTimer = 0.0F;
    }
    createdRoomConfirmed = false;
    return result;
}

void GameClient::HandleServerMessage(const std::string& message) {
    constexpr const char* CreatedSuffix = " created.";
    constexpr const char* JoinedPrefix = "Joined room ";
    constexpr const char* JoinedSuffix = ".";
    const std::string createdSuffix = CreatedSuffix;
    const std::string joinedSuffix = JoinedSuffix;

    if (message == "You were kicked by the host.") {
        Disconnect("You were kicked by the host.");
        return;
    }

    if (message.rfind("Room ", 0) == 0 && message.size() > createdSuffix.size() &&
        message.compare(message.size() - createdSuffix.size(), createdSuffix.size(), createdSuffix) == 0) {
        status = message.substr(0, message.size() - createdSuffix.size());
        if (message.size() > 5 + createdSuffix.size()) {
            roomCode = message.substr(5, message.size() - createdSuffix.size() - 5);
        }
        hostHint = true;
        createdRoomConfirmed = true;
        connecting = false;
        connectTimer = 0.0F;
        network.SendConfig(MakeConfig());
        return;
    }
    if (message.rfind(JoinedPrefix, 0) == 0 && message.size() > joinedSuffix.size() &&
        message.compare(message.size() - joinedSuffix.size(), joinedSuffix.size(), joinedSuffix) == 0) {
        roomCode = message.substr(std::strlen(JoinedPrefix), message.size() - std::strlen(JoinedPrefix) - joinedSuffix.size());
        status = "Room " + roomCode;
        joinedRoomConfirmed = true;
        connecting = false;
        connectTimer = 0.0F;
        network.SendConfig(MakeConfig());
        return;
    }
    if (message == "Match started.") {
        return;
    }
    // During gameplay, do not open a modal popup that halts controls; update status instead
    if (ScreenManager::Current() == ScreenId::Gameplay) {
        status = message;
        return;
    }
    ShowMessage(message);
}

void GameClient::ToggleWindowedFullscreen() {
    ToggleBorderlessWindowed();
}

bool GameClient::IsWindowedFullscreen() const {
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) || IsWindowState(FLAG_FULLSCREEN_MODE);
}

void GameClient::UpdateHelpOverlay() {
    const Rectangle helpButton = HelpButtonBounds();
    const Rectangle fsButton = FullscreenButtonBounds();
    const Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, helpButton) || CheckCollisionPointRec(mouse, fsButton)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    if (IsKeyPressed(KEY_F1)) {
        helpOpen = !helpOpen;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, helpButton)) {
            helpOpen = !helpOpen;
            return;
        }
        if (CheckCollisionPointRec(mouse, fsButton)) {
            ToggleWindowedFullscreen();
            return;
        }
    }
    if (!helpOpen) {
        return;
    }
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(HelpModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(HelpModalHeight)) * 0.5F,
        Sf(HelpModalWidth),
        Sf(HelpModalHeight)
    };
    const Rectangle closeButton{modal.x + modal.width - Sf(52.0F), modal.y + Sf(18.0F), Sf(34.0F), Sf(34.0F)};
    if (IsKeyPressed(KEY_ESCAPE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), closeButton))) {
        helpOpen = false;
    }
}

void GameClient::UpdateMessagePopup() {
    if (!popupOpen) {
        return;
    }
    const Rectangle modal = MessageModalBounds(popupMessage);
    const Rectangle closeButton{modal.x + modal.width - Sf(44.0F), modal.y + Sf(14.0F), Sf(30.0F), Sf(30.0F)};
    const Rectangle okButton{modal.x + (modal.width - Sf(PopupButtonWidth)) * 0.5F, modal.y + modal.height - Sf(52.0F), Sf(PopupButtonWidth), Sf(PopupButtonHeight)};
    if (IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_ENTER) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            (CheckCollisionPointRec(GetMousePosition(), closeButton) ||
                CheckCollisionPointRec(GetMousePosition(), okButton) ||
                !CheckCollisionPointRec(GetMousePosition(), modal)))) {
        popupOpen = false;
    }
}

void GameClient::DrawHelpOverlay() const {
    const Rectangle helpButton = HelpButtonBounds();
    const Rectangle fsButton = FullscreenButtonBounds();
    const Vector2 mouse = GetMousePosition();
    const bool helpHover = CheckCollisionPointRec(mouse, helpButton);
    const bool fsHover = CheckCollisionPointRec(mouse, fsButton);

    // Draw Help Button
    DrawRectangleRounded(helpButton, 0.5F, 16, helpHover ? HelpButtonHoverColor : HelpButtonColor);
    DrawText("?", static_cast<int>(helpButton.x + Sf(14.0F)), static_cast<int>(helpButton.y + Sf(7.0F)), Si(30), HelpTextColor);

    // Draw Fullscreen Button
    const bool isFullscreen = IsWindowedFullscreen();
    DrawRectangleRounded(fsButton, 0.5F, 16, fsHover ? HelpButtonHoverColor : HelpButtonColor);
    DrawFullscreenIcon(fsButton, isFullscreen, HelpTextColor);

    // Tooltips if hovered and modal is not open
    if (!helpOpen && !popupOpen) {
        if (fsHover) {
            const char* tip = isFullscreen ? "Exit Windowed Fullscreen (F11 / Cmd+F / Alt+Enter)" : "Windowed Fullscreen (F11 / Cmd+F / Alt+Enter)";
            const int tipFont = Si(14);
            const int tipW = MeasureText(tip, tipFont);
            const Rectangle tipRect{fsButton.x, fsButton.y + fsButton.height + Sf(6.0F), static_cast<float>(tipW) + Sf(16.0F), Sf(24.0F)};
            DrawRectangleRounded(tipRect, 0.2F, 4, Color{16, 20, 26, 230});
            DrawRectangleLinesEx(tipRect, 1.0F, Color{60, 75, 95, 255});
            DrawText(tip, static_cast<int>(tipRect.x + Sf(8.0F)), static_cast<int>(tipRect.y + Sf(5.0F)), tipFont, HelpTextColor);
        } else if (helpHover) {
            const char* tip = "Help & Controls (F1)";
            const int tipFont = Si(14);
            const int tipW = MeasureText(tip, tipFont);
            const Rectangle tipRect{helpButton.x, helpButton.y + helpButton.height + Sf(6.0F), static_cast<float>(tipW) + Sf(16.0F), Sf(24.0F)};
            DrawRectangleRounded(tipRect, 0.2F, 4, Color{16, 20, 26, 230});
            DrawRectangleLinesEx(tipRect, 1.0F, Color{60, 75, 95, 255});
            DrawText(tip, static_cast<int>(tipRect.x + Sf(8.0F)), static_cast<int>(tipRect.y + Sf(5.0F)), tipFont, HelpTextColor);
        }
    }

    if (!helpOpen) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), HelpOverlayColor);
    const Rectangle modal{
        (static_cast<float>(GetScreenWidth()) - Sf(HelpModalWidth)) * 0.5F,
        (static_cast<float>(GetScreenHeight()) - Sf(HelpModalHeight)) * 0.5F,
        Sf(HelpModalWidth),
        Sf(HelpModalHeight)
    };
    DrawRectangleRounded(modal, 0.04F, 12, HelpPanelColor);
    DrawRectangleLinesEx(modal, 2.0F, Color{78, 96, 118, 255});

    const Rectangle closeButton{modal.x + modal.width - Sf(52.0F), modal.y + Sf(18.0F), Sf(34.0F), Sf(34.0F)};
    const bool closeHover = CheckCollisionPointRec(GetMousePosition(), closeButton);
    DrawRectangleRounded(closeButton, 0.18F, 8, closeHover ? Color{128, 58, 58, 255} : Color{70, 78, 90, 255});
    DrawText("X", static_cast<int>(closeButton.x + Sf(10.0F)), static_cast<int>(closeButton.y + Sf(6.0F)), Si(22), HelpTextColor);

    const float col2X = Sf(180.0F);
    float x = modal.x + Sf(HelpModalPadding);
    float y = modal.y + Sf(HelpModalPadding);
    DrawText("Help", static_cast<int>(x), static_cast<int>(y), Si(HelpTitleSize), HelpTextColor);
    y += Sf(58.0F);

    DrawText("Movement",        static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("WASD or Arrow Keys", static_cast<int>(x + col2X), static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Aim",             static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("Mouse",           static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Shoot",           static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("Left Click",      static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Reload",          static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("R",               static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Dash",            static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("Space",           static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Grenade",         static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("G or E",          static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Barrier",         static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("B or Q",          static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(38.0F);
    DrawText("Start Match",     static_cast<int>(x),          static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    DrawText("Enter",           static_cast<int>(x + col2X),  static_cast<int>(y), Si(HelpTextSize), HelpTextColor);
    y += Sf(50.0F);
    DrawText("F1 also toggles this help menu.", static_cast<int>(x), static_cast<int>(y), Si(HelpTextSize), HelpMutedColor);
    y += Sf(34.0F);
    DrawText("F11 / Cmd+F / Alt+Enter toggles windowed fullscreen.", static_cast<int>(x), static_cast<int>(y), Si(HelpTextSize - 2), HelpMutedColor);
}

void GameClient::DrawMessagePopup() const {
    if (!popupOpen) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 110});
    const Rectangle modal = MessageModalBounds(popupMessage);
    DrawRectangleRounded(modal, 0.04F, 12, HelpPanelColor);
    DrawRectangleLinesEx(modal, 2.0F, Color{78, 96, 118, 255});

    const Rectangle closeButton{modal.x + modal.width - Sf(44.0F), modal.y + Sf(14.0F), Sf(30.0F), Sf(30.0F)};
    const bool closeHover = CheckCollisionPointRec(GetMousePosition(), closeButton);
    DrawRectangleRounded(closeButton, 0.18F, 8, closeHover ? Color{128, 58, 58, 255} : Color{70, 78, 90, 255});
    DrawText("X", static_cast<int>(closeButton.x + Sf(8.0F)), static_cast<int>(closeButton.y + Sf(4.0F)), Si(22), HelpTextColor);

    DrawText("Message", static_cast<int>(modal.x + Sf(24.0F)), static_cast<int>(modal.y + Sf(20.0F)), Si(PopupTitleSize), HelpTextColor);

    const int font = Si(PopupTextSize);
    const int maxInnerWidth = Si(560.0F);
    const auto lines = FormatMessageLines(popupMessage, maxInnerWidth, font);

    float textY = modal.y + Sf(58.0F);
    for (const auto& line : lines) {
        const int measuredW = MeasureText(line.c_str(), font);
        const int textX = static_cast<int>(modal.x + (modal.width - static_cast<float>(measuredW)) * 0.5F);
        DrawText(line.c_str(), textX, static_cast<int>(textY), font, HelpTextColor);
        textY += Sf(26.0F);
    }

    const Rectangle okButton{modal.x + (modal.width - Sf(PopupButtonWidth)) * 0.5F, modal.y + modal.height - Sf(52.0F), Sf(PopupButtonWidth), Sf(PopupButtonHeight)};
    const bool okHover = CheckCollisionPointRec(GetMousePosition(), okButton);
    DrawRectangleRounded(okButton, 0.08F, 8, okHover ? Color{65, 145, 116, 255} : Color{48, 118, 96, 255});

    const int okWidth = MeasureText("OK", Si(PopupTextSize));
    const int okX = static_cast<int>(okButton.x + (okButton.width - static_cast<float>(okWidth)) * 0.5F);
    const int okY = static_cast<int>(okButton.y + (okButton.height - static_cast<float>(Si(PopupTextSize))) * 0.5F - Sf(1.0F));
    DrawText("OK", okX, okY, Si(PopupTextSize), HelpTextColor);
}
