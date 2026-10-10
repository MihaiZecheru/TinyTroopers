#include "client/AssetManager.hpp"

#include "common/Constants.hpp"

#include <algorithm>

namespace {
constexpr float ReferenceHeight = 114.0F;
constexpr float SpriteScale = 0.86F;
constexpr float TargetSoldierHeight = ReferenceHeight * SpriteScale;
constexpr float TargetDeathHeight = 64.0F;
}

AssetManager::AssetManager()
    : playerSoldier{}, opponentSoldier{}, barrierIcon{}, grenadeIcon{}, bulletIcon{}, deathSprite{}, killIcon{}, ready(false) {}

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
    if (deathSprite.id != 0) {
        UnloadTexture(deathSprite);
    }
    if (killIcon.id != 0) {
        UnloadTexture(killIcon);
    }
}

bool AssetManager::Load() {
    playerSoldier = LoadTexture("assets/player_soldier.png");
    opponentSoldier = LoadTexture("assets/opponent_soldier.png");
    barrierIcon = LoadTexture("assets/brick-wall.png");
    grenadeIcon = LoadTexture("assets/grenade.png");
    bulletIcon = LoadTexture("assets/bullet.png");
    deathSprite = LoadTexture("assets/death.png");
    killIcon = LoadTexture("assets/kill_icon.png");
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
    if (deathSprite.id != 0) {
        SetTextureFilter(deathSprite, TEXTURE_FILTER_BILINEAR);
    }
    if (killIcon.id != 0) {
        SetTextureFilter(killIcon, TEXTURE_FILTER_BILINEAR);
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

void AssetManager::DrawDeath(Vector2 position, float rotation, float alpha) const {
    const unsigned char a = static_cast<unsigned char>(std::clamp(alpha, 0.0F, 1.0F) * 255.0F);
    const Color tint{255, 255, 255, a};
    if (deathSprite.id != 0 && deathSprite.width > 0 && deathSprite.height > 0) {
        const float scale = (deathSprite.height > 0) ? (TargetDeathHeight / static_cast<float>(deathSprite.height)) : 1.0F;
        const Rectangle source{0.0F, 0.0F, static_cast<float>(deathSprite.width), static_cast<float>(deathSprite.height)};
        const Rectangle target{position.x, position.y, static_cast<float>(deathSprite.width) * scale, static_cast<float>(deathSprite.height) * scale};
        const Vector2 origin{target.width * 0.5F, target.height * 0.5F};
        DrawTexturePro(deathSprite, source, target, origin, rotation, tint);
    } else {
        DrawCircleV(position, cfg::PlayerRadius, Color{180, 40, 40, a});
    }
}

bool AssetManager::IsReady() const {
    return ready;
}
