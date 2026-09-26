#pragma once

#include "common/Protocol.hpp"

#include "raylib.h"

#include <array>
#include <random>

class AudioManager {
public:
    static constexpr std::size_t VoiceCountPerWeapon = 8;

    AudioManager();
    ~AudioManager();

    void Load();
    void PlayShot(PlayerClass playerClass);
    void PlayShotSpatial(PlayerClass playerClass, Vector2 soundPosition, Vector2 listenerPosition);
    void PlayEmpty();
    void PlayHitmarker();
    void PlayGrenade();
    void PlayBarrier();

private:
    struct WeaponAudioPool {
        Sound masterSound{};
        std::array<Sound, VoiceCountPerWeapon> voices{};
        std::size_t nextVoice{0};
    };

    Sound LoadOptionalSound(const char* path);
    void UnloadOptionalSound(Sound sound);
    void PlayOptionalSound(Sound sound);
    void PlayShotInternal(PlayerClass playerClass, float volume, float pitch, float pan);

    std::array<WeaponAudioPool, 4> weaponPools;
    Sound empty;
    Sound hitmarker;
    Sound grenade;
    Sound barrier;
    bool ready;
    std::mt19937 rng;
};
