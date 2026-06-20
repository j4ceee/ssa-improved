#pragma once
#include <cstring>
#include "config.h"
#include "game/spyro_character_settings.h"

namespace ssa::Difficulty
{
    // Snapshot of the values BuildSettings wrote, before we multiplied them
    // Used to re-apply the multiplier from config after a level transition or reload
    inline float s_baseHpSnapshot[8] = {};
    inline float s_baseDmgSnapshot[8] = {};
    inline void* s_snapshotAttr = nullptr;

    inline void ApplyMultipliers(Game::SpyroCharacterSettings::EnemySettings& enemy)
    {
        for (int i = 0; i < 8; i++)
        {
            enemy.m_fBaseHP[i] = s_baseHpSnapshot[i] * g_config.hpMult;
            enemy.m_fBaseDamage[i] = s_baseDmgSnapshot[i] * g_config.dmgMult;
        }
    }

    inline void SnapshotAndApply(Game::SpyroCharacterSettings::EnemySettings& enemy)
    {
        memcpy(s_baseHpSnapshot, enemy.m_fBaseHP, sizeof(s_baseHpSnapshot));
        memcpy(s_baseDmgSnapshot, enemy.m_fBaseDamage, sizeof(s_baseDmgSnapshot));
        s_snapshotAttr = enemy.m_pLevelAttribute;
        ApplyMultipliers(enemy);
    }

    // called from hook_Present every frame
    inline void Update()
    {
        auto& enemy = Game::SpyroCharacterSettings::instance()->m_enemySettings;

        // Heroic challenge and XP fields seem to be set once by ReadFromLX at startup and never touched again by the game
        // -> writing every frame is safe and keeps config in sync
        enemy.m_fMaxChallengeLevelHPMultiplier = g_config.heroicHpCeiling;
        enemy.m_fMaxChallengeLevelDamageMultiplier = g_config.heroicDmgCeiling;
        enemy.m_fExpMultiplierGlobal = g_config.xpMult;

        if (!enemy.m_pLevelAttribute)
            return;

        if (enemy.m_pLevelAttribute != s_snapshotAttr)
            SnapshotAndApply(enemy); // new level -> re-snapshot then apply
        else
            ApplyMultipliers(enemy); // same level -> just keep enforcing
    }
}
