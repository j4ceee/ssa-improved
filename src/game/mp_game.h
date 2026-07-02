#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct GameData
    {
        uint8_t bHosting;               // +0x00
        char _pad0[3];
        int32_t nType;                  // +0x04
        int32_t nMode;                  // +0x08
        int32_t nMap;                   // +0x0C
        int32_t nRoundTime;             // +0x10
        int32_t nMaxScore;              // +0x14
        int32_t nMaxPlayers;            // +0x18
        int32_t nPrivateSlots;          // +0x1C
        uint8_t bFactionRestriction;    // +0x20
        uint8_t bRestoreHealth;         // +0x21
        uint8_t bPowerups;              // +0x22
        uint8_t bFood;                  // +0x23
        uint8_t bArenaHazards;          // +0x24
        uint8_t bUpgrades;              // +0x25
        char _pad1[2];
    };

    static_assert(sizeof(GameData) == 0x28);

    struct SpawnData
    {
        int32_t nState; // +0x00
        float fDelay;   // +0x04
    };

    static_assert(sizeof(SpawnData) == 0x08);

    struct MPGame
    {
        uint8_t     m_bOnline;                          // +0x000
        uint8_t     m_bReadyToRequestPlayer;            // +0x001
        uint8_t     m_bHandleGameLoadedOncePerMatch;    // +0x002
        char        _pad0[1];
        int32_t     m_nState;                           // +0x004
        int32_t     m_nEventsReceived;                  // +0x008
        GameData    m_gameData;                         // +0x00C
        int32_t     m_crcVersionCheck;                  // +0x034
        char        _pad1[24];
        void*       m_pRules[7];                        // +0x050
        SpawnData   m_spawnData;                        // +0x06C
        char        _pad3[4];
        int32_t     m_nRoundNumber;                     // +0x078
        int32_t     m_nMapTeamScore[3];                 // +0x07C
        int32_t     m_nCurrentKills[3];                 // +0x088
        char        _pad4[20];
        char        m_Users[16];                        // +0x0A8 vector<UserReplica*>
        char        m_NewUserNeedsTeamAndChar[12];      // +0x0B8 list<unsigned short>
        char        m_Avatars[16];                      // +0x0C4 vector<AvatarReplica*>
        char        m_Bots[16];                         // +0x0D4 vector<BotReplica*>
        char        _pad5[4];
        uint8_t     m_bFriendlyFireEnabled;             // +0x0E8
        char        _pad6[3];
        void*       m_pSpawnPoints;                     // +0x0EC
        char        _pad7[1196];
        uint8_t     m_bIsPvPMatch;                      // +0x59C


        static MPGame* instance()
        {
            return reinterpret_cast<MPGame*>(GetAddress(MP_GAME));
        }

        /**
         * DamageEvent branches on this: <2 → CanGetHit(), >=2 → MPGame::CanGetHit()
         * @return <code>true</code> if the game is in a PvP match, <code>false</code> otherwise
         */
        [[nodiscard]] bool isInPvPMatch() const { return m_bIsPvPMatch != 0; }

        void SetInPvPMatch(bool enable)
        {
            if (enable)
                m_bIsPvPMatch = 1;
            else
                m_bIsPvPMatch = 0;
        }
    };

    static_assert(offsetof(MPGame, m_nState) == 0x004);
    static_assert(offsetof(MPGame, m_gameData) == 0x00C);
    static_assert(offsetof(MPGame, m_pRules) == 0x050);
    static_assert(offsetof(MPGame, m_spawnData) == 0x06C);
    static_assert(offsetof(MPGame, m_nRoundNumber) == 0x078);
    static_assert(offsetof(MPGame, m_Users) == 0x0A8);
    static_assert(offsetof(MPGame, m_NewUserNeedsTeamAndChar) == 0x0B8);
    static_assert(offsetof(MPGame, m_Avatars) == 0x0C4);
    static_assert(offsetof(MPGame, m_Bots) == 0x0D4);
    static_assert(offsetof(MPGame, m_bFriendlyFireEnabled) == 0x0E8);
    static_assert(offsetof(MPGame, m_pSpawnPoints) == 0x0EC);
    static_assert(offsetof(MPGame, m_bIsPvPMatch) == 0x59C);
} // namespace ssa::Game
#pragma pack(pop)
