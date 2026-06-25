#pragma once
#include <cstring>

#include "game/character.h"
#include "config.h"
#include "game/spyro_character_settings.h"

namespace ssa::Difficulty
{
    // Snapshot of the values BuildSettings wrote, before we multiplied them
    // Used to re-apply the multiplier from config after a level transition or reload
    inline float s_baseDmgSnapshot[8] = {};
    inline void* s_snapshotAttr = nullptr;

    inline void ApplyDamageMultiplier(Game::SpyroCharacterSettings::EnemySettings& enemy)
    {
        for (int i = 0; i < 8; i++)
            enemy.m_fBaseDamage[i] = s_baseDmgSnapshot[i] * g_config.dmgMult;
    }

    inline void SnapshotAndApplyDamage(Game::SpyroCharacterSettings::EnemySettings& enemy)
    {
        memcpy(s_baseDmgSnapshot, enemy.m_fBaseDamage, sizeof(s_baseDmgSnapshot));
        s_snapshotAttr = enemy.m_pLevelAttribute;
        ApplyDamageMultiplier(enemy);
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

        // EnemySettings damage: handles regular enemies (attackMultiplier is a no-op for them)
        if (enemy.m_pLevelAttribute)
        {
            if (enemy.m_pLevelAttribute != s_snapshotAttr)
                SnapshotAndApplyDamage(enemy); // new level -> re-snapshot then apply
            else
                ApplyDamageMultiplier(enemy); // same level -> keep enforcing (also covers reloads)
        }

        // Per-Character writes: cover both regular enemies and boss Characters (evil Skylanders)
        // HP: hpMultiplier works for all enemies
        // Damage: attackMultiplier is a no-op for regular enemies but works for evil Skylanders
        auto* list = Game::CharacterList::instanceAll();

        for (const auto* node = list->begin(); node != list->end(); node = node->next)
        {
            auto* ch = node->ptr;

            if (!ch || !ch->isEnemy() || !ch->isLocalAI())
                continue;

            if (ch->hpMultiplier != g_config.hpMult)
            {
                ch->hpMultiplier = g_config.hpMult;
                ch->m_fCurrHealth = ch->baseHP * ch->hpMultiplier;
            }

            if (!g_config.enemyHitReaction && !ch->hasIgnoreHitReaction())
            {
                ch->setIgnoreHitReaction(true);
            }
            else if (g_config.enemyHitReaction && ch->hasIgnoreHitReaction())
            {
                ch->setIgnoreHitReaction(false);
            }

            if (ch->attackMultiplier != g_config.dmgMult)
                ch->attackMultiplier = g_config.dmgMult;
        }
    }
}
