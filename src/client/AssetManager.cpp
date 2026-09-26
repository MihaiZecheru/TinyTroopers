#include "client/AssetManager.hpp"

#include "common/Constants.hpp"

namespace {
constexpr float ReferenceHeight = 114.0F;
constexpr float SpriteScale = 0.86F;
constexpr float TargetSoldierHeight = ReferenceHeight * SpriteScale;
}

AssetManager::AssetManager()
    : playerSoldier{}, opponentSoldier{}, barrierIcon{}, grenadeIcon{}, bulletIcon{}, ready(false) {}

AssetManager::~AssetManager() {
    if (playerSoldier.id != 0) {
        UnloadTexture(playerSoldier);
    }
    if (opponentSoldier.id != 0) {
        UnloadTexture(opponentSoldier);
    }
    if (barrierIcon.id != 0) {
        UnloadTexture(barrierIcon);
    }
    if (grenadeIcon.id != 0) {
        UnloadTexture(grenadeIcon);
    }
    if (bulletIcon.id != 0) {
        UnloadTexture(bulletIcon);
    }
}

bool AssetManager::Load() {
    playerSoldier = LoadTexture("assets/player_soldier.png");
    opponentSoldier = LoadTexture("assets/opponent_soldier.png");
    barrierIcon = LoadTexture("assets/brick-wall.png");
    grenadeIcon = LoadTexture("assets/grenade.png");
    bulletIcon = LoadTexture("assets/bullet.png");
    ready = playerSoldier.id != 0 && opponentSoldier.id != 0;
    if (playerSoldier.id != 0) {
        SetTextureFilter(playerSoldier, TEXTURE_FILTER_POINT);
    }
    if (opponentSoldier.id != 0) {
        SetTextureFilter(opponentSoldier, TEXTURE_FILTER_POINT);
    }
    if (barrierIcon.id != 0) {
        SetTextureFilter(barrierIcon, TEXTURE_FILTER_BILINEAR);
    }
    if (grenadeIcon.id != 0) {
        SetTextureFilter(grenadeIcon, TEXTURE_FILTER_BILINEAR);
    }
    if (bulletIcon.id != 0) {
        SetTextureFilter(bulletIcon, TEXTURE_FILTER_BILINEAR);
    }
    return ready;
}

void AssetManager::DrawSoldier(bool localPlayer, Vector2 position, float rotation) const {
    if (!ready) {
        DrawCircleV(position, cfg::PlayerRadius, localPlayer ? GREEN : YELLOW);
        return;
    }
    const Texture2D& texture = localPlayer ? playerSoldier : opponentSoldier;
    const float scale = (texture.height > 0) ? (TargetSoldierHeight / static_cast<float>(texture.height)) : SpriteScale;
    const Rectangle source{0.0F, 0.0F, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    const Rectangle target{position.x, position.y, static_cast<float>(texture.width) * scale, static_cast<float>(texture.height) * scale};
    const Vector2 origin{target.width * 0.5F, target.height * 0.5F};
    DrawTexturePro(texture, source, target, origin, rotation, WHITE);
}

bool AssetManager::IsReady() const {
    return ready;
}
