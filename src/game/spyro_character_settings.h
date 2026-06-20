#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct SpyroCharacterSettings
    {
        // indexes into EnemySettings::m_fBaseHP[8] and m_fBaseDamage[8]
        // "Swarmer" / "Spawner" names confirmed from exp field names in the PDB
        enum class EnemyClass : int
        {
            Swarmer = 0,
            Spawner = 1,
            Unknown2 = 2,
            Unknown3 = 3,
            Unknown4 = 4,
            Unknown5 = 5,
            Unknown6 = 6,
            Unknown7 = 7,
        };

        struct ChampionSettings
        {
            float m_fHP[5];                             // +0x00
            float m_fBaseSpeed[5];                      // +0x14
            float m_fBaseCrit[5];                       // +0x28
            float m_fBaseArmor[5];                      // +0x3C
            float m_fAverageHP;                         // +0x50
            float m_fAverageHitsToKillInPvP;            // +0x54
            float m_fAverageRoF;                        // +0x58
            float m_fAverageDamage;                     // +0x5C
            float m_fAverageDPS;                        // +0x60
            float m_fMaxLevelUpHPMultiplier;            // +0x64
            float m_fMaxLevelUpDamageMultiplier;        // +0x68
            float m_fMaxUpgradeDamageMultiplier;        // +0x6C
            float m_fMaxChallengeBuffSpeed;             // +0x70
            float m_fMaxChallengeBuffArmor;             // +0x74
            float m_fMaxChallengeBuffCriticalRate;      // +0x78
            float m_fBaseElementalStrength;             // +0x7C
            float m_fMaxChallengeBuffElementalStrength; // +0x80
            float m_fMaxCollectionElementalStrength;    // +0x84
        }; // size: 0x88

        struct EnemySettings
        {
            float m_fExpSwarmerNormalLevel;                 // +0x00
            float m_fExpSpawnerNormalLevel;                 // +0x04
            float m_fExpSwarmerChallengeLevel;              // +0x08
            float m_fExpSpawnerChallengeLevel;              // +0x0C
            float m_fExpMultiplier[8];                      // +0x10
            float m_fLevelBasedExp[8];                      // +0x30
            float m_fBaseHP[8];                             // +0x50 one entry per EnemyClass
            float m_fBaseDamage[8];                         // +0x70 one entry per EnemyClass
            float m_fAverageHitsToKillThisClass[8];         // +0x90
            float m_fAverageHitsForThisClassToKill[8];      // +0xB0
            float m_fExpMultiplierGlobal;                   // +0xD0
            float m_fExpMultiplierElementalZone;            // +0xD4
            float m_fMaxDamageMultiplier;                   // +0xD8
            float m_fMaxDifficulty;                         // +0xDC
            float m_fMaxChallengeLevelHPMultiplier;         // +0xE0
            float m_fMaxChallengeLevelDamageMultiplier;     // +0xE4
            float m_fCoopEnemyHPMultiplier;                 // +0xE8
            float m_fCoopEnemyDamageMultiplier;             // +0xEC
            float m_fGlobalEnemyHPMultiplier;               // +0xF0 reset every frame by the game, do not write
            float m_fGlobalEnemyDamageMultiplier;           // +0xF4  confirmed unused by EnemyDamage()
            void* m_pLevelAttribute;                        // +0xF8  set by BuildSettings; null between levels
            const ChampionSettings* m_pChampionSettings;    // +0xFC
        }; // size: 0x100

        // ltl2::list<LevelAttribute> header - self-referential pointers when empty
        void* m_listNext; // +0x00
        void* m_listPrev; // +0x04
        uint32_t m_listAllocator; // +0x08

        ChampionSettings m_championSettings; // +0x0C
        EnemySettings m_enemySettings; // +0x94
        void* m_pCurrentLevelAttribute; // +0x194

        static SpyroCharacterSettings* instance()
        {
            return reinterpret_cast<SpyroCharacterSettings*>(GetAddress(SPYRO_CHARACTER_SETTINGS));
        }
    };

    static_assert(sizeof(SpyroCharacterSettings::ChampionSettings) == 0x88);
    static_assert(sizeof(SpyroCharacterSettings::EnemySettings) == 0x100);

    static_assert(offsetof(SpyroCharacterSettings, m_championSettings) == 0x0C);
    static_assert(offsetof(SpyroCharacterSettings, m_enemySettings) == 0x94);
    static_assert(offsetof(SpyroCharacterSettings, m_pCurrentLevelAttribute) == 0x194);

    static_assert(offsetof(SpyroCharacterSettings::EnemySettings, m_fBaseHP) == 0x50);
    static_assert(offsetof(SpyroCharacterSettings::EnemySettings, m_fBaseDamage) == 0x70);
    static_assert(offsetof(SpyroCharacterSettings::EnemySettings, m_fGlobalEnemyHPMultiplier) == 0xF0);
    static_assert(offsetof(SpyroCharacterSettings::EnemySettings, m_fGlobalEnemyDamageMultiplier) == 0xF4);
    static_assert(offsetof(SpyroCharacterSettings::EnemySettings, m_pLevelAttribute) == 0xF8);
} // namespace ssa::Game
#pragma pack(pop)
