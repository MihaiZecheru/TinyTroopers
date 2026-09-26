#include "client/ScreenManager.hpp"

ScreenId ScreenManager::current = ScreenId::MainMenu;
ScreenManager* ScreenManager::instance = nullptr;

ScreenManager::ScreenManager() {
    instance = this;
}

ScreenManager::~ScreenManager() {
    if (instance == this) {
        instance = nullptr;
    }
}

void ScreenManager::Update(GameClient& client) {
    switch (current) {
    case ScreenId::MainMenu: mainMenu.Update(client); break;
    case ScreenId::RoomCreation: creation.Update(client); break;
    case ScreenId::RoomJoining: joining.Update(client); break;
    case ScreenId::Gameplay: gameplay.Update(client); break;
    }
}

void ScreenManager::Draw(GameClient& client) const {
    switch (current) {
    case ScreenId::MainMenu: mainMenu.Draw(client); break;
    case ScreenId::RoomCreation: creation.Draw(client); break;
    case ScreenId::RoomJoining: joining.Draw(client); break;
    case ScreenId::Gameplay: gameplay.Draw(client); break;
    }
}

void ScreenManager::Set(ScreenId screen) {
    if (instance != nullptr && (screen == ScreenId::MainMenu || (screen == ScreenId::Gameplay && current != ScreenId::Gameplay))) {
        instance->gameplay.Reset();
    }
    current = screen;
}

ScreenId ScreenManager::Current() {
    return current;
}

GameplayScreen* ScreenManager::Gameplay() {
    return instance != nullptr ? &instance->gameplay : nullptr;
}
