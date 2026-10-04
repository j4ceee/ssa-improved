#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "game/data/attack_light_profiles.h"

// Dynamic lights for projectile attacks that have none of their own.
// Every frame the world's components are scanned for projectiles fired by the local players. Projectiles matching an entry of
// Data::kAttackLightProfiles get a light created by the game's own light factory, attached to the projectile's owner object.
// Nothing in the game's code is patched, lights are created and removed only.
namespace ssa::Game::AttackLights
{
    // called from hook_Present every frame
    void Update();

    // -- diagnostics ----------------------------------------------------------------------------------------------------

    struct Stats
    {
        size_t matched; // projectiles with a profile this frame
        size_t active; // lights added by the mod
        size_t maxLights;
        uint32_t created; // total since the game started
    };

    // a projectile group seen while recording
    struct Observation
    {
        Data::AttackLightKey key;
        std::string characterName;
        size_t live = 0; // projectiles of this group alive in the last frame
        bool nativeLight = false; // the owner already had a light that is not ours
    };

    struct TestLightSettings
    {
        float intensity = 3.0f;
        float radius = 12.0f;
    };

    [[nodiscard]] Stats GetStats();
    [[nodiscard]] const std::string& GetStatus();
    [[nodiscard]] const std::vector<Observation>& GetObservations();
    [[nodiscard]] TestLightSettings& GetTestLightSettings();

    // record the projectile groups of the local players; call every frame while recording, it stops ~0.5 s after the last call
    void KeepRecording();
    void ClearObservations();

    // puts a light on player 1 for three seconds
    void SpawnTestLight();
} // namespace ssa::Game::AttackLights
