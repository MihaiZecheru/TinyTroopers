#include "client/AudioManager.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr const char* Ak47ShotPath = "assets/shot_ak47.wav";
constexpr const char* PistolShotPath = "assets/shot_m1911.wav";
constexpr const char* SniperShotPath = "assets/shot_kar98k.wav";
constexpr const char* ShotgunShotPath = "assets/shot_mossberg500.wav";
constexpr const char* EmptyClickPath = "assets/empty_click.wav";
constexpr const char* HitmarkerPath = "assets/hitmarker.wav";
constexpr const char* GrenadePath = "assets/grenade.wav";
constexpr const char* BarrierPath = "assets/barrier.wav";
}

AudioManager::AudioManager() : weaponPools{}, empty{}, hitmarker{}, grenade{}, barrier{}, ready(false), rng(std::random_device{}()) {}

AudioManager::~AudioManager() {
    if (!ready) {
        return;
    }
    for (WeaponAudioPool& pool : weaponPools) {
        for (Sound& voice : pool.voices) {
            if (voice.stream.buffer != nullptr) {
                UnloadSoundAlias(voice);
                voice = Sound{};
            }
        }
        UnloadOptionalSound(pool.masterSound);
        pool.masterSound = Sound{};
    }
    UnloadOptionalSound(empty);
    UnloadOptionalSound(hitmarker);
    UnloadOptionalSound(grenade);
    UnloadOptionalSound(barrier);
    CloseAudioDevice();
}

void AudioManager::Load() {
    TraceLog(LOG_INFO, "AUDIO: TinyTroopers audio startup begin");
    InitAudioDevice();
    ready = IsAudioDeviceReady();
    if (!ready) {
        TraceLog(LOG_WARNING, "AUDIO: Audio device is not ready; continuing without sounds");
        return;
    }

    weaponPools[0].masterSound = LoadOptionalSound(Ak47ShotPath);
    weaponPools[1].masterSound = LoadOptionalSound(PistolShotPath);
    weaponPools[2].masterSound = LoadOptionalSound(SniperShotPath);
    weaponPools[3].masterSound = LoadOptionalSound(ShotgunShotPath);

    for (WeaponAudioPool& pool : weaponPools) {
        pool.nextVoice = 0;
        if (pool.masterSound.stream.buffer != nullptr) {
            for (std::size_t v = 0; v < VoiceCountPerWeapon; ++v) {
                pool.voices[v] = LoadSoundAlias(pool.masterSound);
            }
        } else {
            for (std::size_t v = 0; v < VoiceCountPerWeapon; ++v) {
                pool.voices[v] = Sound{};
            }
        }
    }

    empty = LoadOptionalSound(EmptyClickPath);
    hitmarker = LoadOptionalSound(HitmarkerPath);
    grenade = LoadOptionalSound(GrenadePath);
    barrier = LoadOptionalSound(BarrierPath);
    TraceLog(LOG_INFO, "AUDIO: TinyTroopers audio startup complete");
}

void AudioManager::PlayShot(PlayerClass playerClass) {
    // Local player: full transient punch, centered pan
    PlayShotInternal(playerClass, 0.88F, 1.0F, 0.0F);
}

void AudioManager::PlayShotSpatial(PlayerClass playerClass, Vector2 soundPosition, Vector2 listenerPosition) {
    constexpr float MaxAudibleDistance = 1400.0F;
    const float dx = soundPosition.x - listenerPosition.x;
    const float dy = soundPosition.y - listenerPosition.y;
    const float dist = std::sqrt((dx * dx) + (dy * dy));
    if (dist >= MaxAudibleDistance) {
        return;
    }

    // Distance attenuation with quadratic falloff
    const float norm = dist / MaxAudibleDistance;
    const float falloff = (1.0F - norm) * (1.0F - norm);
    const float volume = 0.75F * falloff;

    // Stereo panning based on horizontal offset relative to listener
    const float pan = std::clamp(dx / 700.0F, -0.80F, 0.80F);

    PlayShotInternal(playerClass, volume, 1.0F, pan);
}

void AudioManager::PlayShotInternal(PlayerClass playerClass, float baseVolume, float basePitch, float pan) {
    if (!ready) {
        return;
    }
    const auto classIdx = static_cast<std::size_t>(playerClass);
    if (classIdx >= weaponPools.size()) {
        return;
    }

    WeaponAudioPool& pool = weaponPools[classIdx];
    if (pool.masterSound.stream.buffer == nullptr) {
        return;
    }

    // Micro-pitch & volume randomization to prevent phase cancellation & comb filtering
    std::uniform_real_distribution<float> pitchDist(-0.035F, 0.035F);
    std::uniform_real_distribution<float> volDist(-0.03F, 0.03F);
    const float finalPitch = std::clamp(basePitch + pitchDist(rng), 0.85F, 1.25F);
    const float finalVolume = std::clamp(baseVolume + volDist(rng), 0.05F, 1.0F);

    // Pick current voice round-robin
    const std::size_t currentIdx = pool.nextVoice;
    pool.nextVoice = (pool.nextVoice + 1) % VoiceCountPerWeapon;
    Sound& voice = pool.voices[currentIdx];
    if (voice.stream.buffer == nullptr) {
        return;
    }

    // Dynamic ducking of older voices to prevent volume stacking and digital distortion:
    const std::size_t prevIdx = (currentIdx + VoiceCountPerWeapon - 1) % VoiceCountPerWeapon;
    Sound& prevVoice = pool.voices[prevIdx];
    if (prevVoice.stream.buffer != nullptr && IsSoundPlaying(prevVoice)) {
        SetSoundVolume(prevVoice, finalVolume * 0.35F);
    }
    const std::size_t prevPrevIdx = (currentIdx + VoiceCountPerWeapon - 2) % VoiceCountPerWeapon;
    Sound& prevPrevVoice = pool.voices[prevPrevIdx];
    if (prevPrevVoice.stream.buffer != nullptr && IsSoundPlaying(prevPrevVoice)) {
        SetSoundVolume(prevPrevVoice, finalVolume * 0.15F);
    }

    SetSoundVolume(voice, finalVolume);
    SetSoundPitch(voice, finalPitch);
    SetSoundPan(voice, pan);
    PlaySound(voice);
}

void AudioManager::PlayEmpty() {
    PlayOptionalSound(empty);
}

void AudioManager::PlayHitmarker() {
    PlayOptionalSound(hitmarker);
}

void AudioManager::PlayGrenade() {
    PlayOptionalSound(grenade);
}

void AudioManager::PlayBarrier() {
    PlayOptionalSound(barrier);
}

Sound AudioManager::LoadOptionalSound(const char* path) {
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "AUDIO: Missing optional sound file: %s", path);
        return Sound{};
    }
    Sound sound = LoadSound(path);
    if (sound.stream.buffer == nullptr) {
        TraceLog(LOG_WARNING, "AUDIO: Failed to load sound file: %s", path);
    }
    return sound;
}

void AudioManager::UnloadOptionalSound(Sound sound) {
    if (sound.stream.buffer != nullptr) {
        UnloadSound(sound);
    }
}

void AudioManager::PlayOptionalSound(Sound sound) {
    if (ready && sound.stream.buffer != nullptr) {
        PlaySound(sound);
    }
}
