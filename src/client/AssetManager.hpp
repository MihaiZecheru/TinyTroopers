#pragma once

#include "common/Protocol.hpp"

#include "raylib.h"

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    bool Load();
    void DrawSoldier(bool localPlayer, Vector2 position, float rotation) const;
    void DrawDeath(Vector2 position, float rotation, float alpha = 1.0F) const;
    bool IsReady() const;

    const Texture2D& BarrierIcon() const { return barrierIcon; }
    const Texture2D& GrenadeIcon() const { return grenadeIcon; }
    const Texture2D& BulletIcon() const { return bulletIcon; }
    const Texture2D& KillIcon() const { return killIcon; }
    const Texture2D& DeathSprite() const { return deathSprite; }

private:
    Texture2D playerSoldier;
    Texture2D opponentSoldier;
    Texture2D barrierIcon;
    Texture2D grenadeIcon;
    Texture2D bulletIcon;
    Texture2D deathSprite;
    Texture2D killIcon;
    bool ready;
};
