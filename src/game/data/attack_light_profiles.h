#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

namespace ssa::Game::Data
{
    // identifies one group of projectiles of a Skylander
    struct AttackLightKey
    {
        uint32_t character = 0; // Character::nameID
        uint32_t triggerTag = 0; // not an attack ID: several attacks of one Skylander can share it

        constexpr bool operator==(const AttackLightKey& other) const
        {
            return character == other.character && triggerTag == other.triggerTag;
        }
    };

    struct AttackLightProfile
    {
        const char* name;
        AttackLightKey key;
        float colour[3]; // linear HDR RGB
        float intensity; // multiplies colour
        float radius; // game world units
    };

    // projectile groups that get a dynamic light, with tuned values (all tested in game)
    inline constexpr AttackLightProfile kAttackLightProfiles[] =
    {
        {"Zap Basic",               {0xdb226620, 0xe75eeb0d}, {0.35f, 0.8f, 1.0f},  0.5f, 12.0f},
        {"Zap Upgraded",            {0xdb226620, 0xe82e4a04}, {1.0f, 1.0f, 0.35f},  0.5f, 12.0f},
        {"Lightning Rod Basic",     {0x934ec6be, 0x00000000}, {1.0f, 0.8f, 0.0f},   1.0f, 12.0f},
        {"Whirlwind Basic",         {0x0a05f459, 0x00000000}, {0.8f, 0.8f, 1.0f},   0.5f, 6.0f},
        {"Eruptor Basic",           {0x982e41bd, 0x00000000}, {1.0f, 0.5f, 0.0f},   1.0f, 8.0f},
        {"Camo Basic & Vines",      {0xdf0ad641, 0x00000000}, {1.0f, 0.8f, 0.0f},   0.6f, 8.0f},
        {"Zook Weapon",             {0xf9387ab5, 0x03d57943}, {1.0f, 0.54f, 0.0f},  0.3f, 4.0f},
        {"Voodood Weapon",          {0xf3939cbd, 0x00000000}, {0.9f, 0.4f, 1.0f},   0.5f, 4.5f},
        {"Boomer Basic",            {0x72998a95, 0x00000000}, {1.0f, 0.8f, 0.0f},   0.4f, 5.0f},
        {"Drill Seargent Weapon",   {0x77d8c5fa, 0x00000000}, {1.0f, 0.8f, 0.0f},   0.4f, 2.0f},
        {"Trigger Happy Basic",     {0x35cd2899, 0x00000000}, {1.0f, 0.8f, 0.0f},   0.3f, 2.8f},
        {"Slam Bam Ice Attack",     {0xa660f3f1, 0x00000000}, {0.35f, 0.8f, 1.0f},  1.5f, 6.0f},
        {"Wham Shell Shuriken",     {0x36b05ef0, 0x00000000}, {0.0f, 0.7f, 1.0f},   0.5f, 3.4f},
    };

    constexpr bool ValidAttackLightProfiles()
    {
        for (size_t i = 0; i < std::size(kAttackLightProfiles); ++i)
        {
            const auto& p = kAttackLightProfiles[i];
            if (!p.name || !p.name[0] || !p.key.character ||
                !(p.intensity >= 0.1f && p.intensity <= 10) || !(p.radius >= 1 && p.radius <= 24))
                return false;
            for (const float c : p.colour) if (!(c >= 0 && c <= 4)) return false;
            for (size_t j = 0; j < i; ++j) if (p.key == kAttackLightProfiles[j].key) return false;
        }
        return true;
    }
    static_assert(ValidAttackLightProfiles(), "Attack light profiles need valid values and unique character / trigger tag pairs.");

    inline const AttackLightProfile* FindAttackLightProfile(const AttackLightKey& key)
    {
        for (const auto& profile : kAttackLightProfiles)
            if (profile.key == key) return &profile;
        return nullptr;
    }
} // namespace ssa::Game::Data
