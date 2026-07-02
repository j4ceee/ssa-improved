#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct LevelDesc
    {
        uint32_t    m_Id;           // +0x00 CRC32("Level_NNN"), matches Game::m_CurrLevel
        uint32_t    m_NextLevel;    // +0x04 CRC32 of the level that follows
        Ltl2String  m_LevelFile;    // +0x08 e.g. "FrontEnd", "Level_Hub", "Level_001"
        Ltl2String  m_Category;     // +0x14 e.g. "SpyroLevels", "PvP_Level"
        float       m_fDeathHeight; // +0x20 Y below which characters respawn
        Vec3        m_WorldMin;     // +0x24
        Vec3        m_WorldMax;     // +0x30
        char        _pad0[0x18];
    };
    static_assert(sizeof(LevelDesc) == 0x54);
    static_assert(offsetof(LevelDesc, m_Id) == 0x00);
    static_assert(offsetof(LevelDesc, m_NextLevel) == 0x04);
    static_assert(offsetof(LevelDesc, m_LevelFile) == 0x08);
    static_assert(offsetof(LevelDesc, m_Category) == 0x14);
    static_assert(offsetof(LevelDesc, m_WorldMin) == 0x24);
    static_assert(offsetof(LevelDesc, m_WorldMax) == 0x30);


    struct CharacterDesc
    {
        Ltl2String  m_Name;                 // +0x00 display name (matches portal figure name)
        uint32_t    m_Id;                   // +0x0C CRC32 character identifier
        uint32_t    m_NameStrId;            // +0x10
        Ltl2String  m_FilePath;             // +0x14 model/data bundle path
        uint32_t    m_characterAttributes;  // +0x20 CRC32 attribute tag
        float       m_fScale;               // +0x24
    };
    static_assert(sizeof(CharacterDesc) == 0x28);
    static_assert(offsetof(CharacterDesc, m_Name) == 0x00);
    static_assert(offsetof(CharacterDesc, m_Id) == 0x0C);
    static_assert(offsetof(CharacterDesc, m_NameStrId) == 0x10);
    static_assert(offsetof(CharacterDesc, m_FilePath) == 0x14);
    static_assert(offsetof(CharacterDesc, m_characterAttributes) == 0x20);
    static_assert(offsetof(CharacterDesc, m_fScale) == 0x24);

    
    struct MagicItemDesc
    {
        Ltl2String  m_Name; // +0x00
        uint32_t    m_Id;   // +0x0C CRC32 magic item identifier
        uint32_t    m_Type; // +0x10 lux::MagicItemType enum value
    };
    static_assert(sizeof(MagicItemDesc) == 0x14);
    static_assert(offsetof(MagicItemDesc, m_Name) == 0x00);
    static_assert(offsetof(MagicItemDesc, m_Id) == 0x0C);
    static_assert(offsetof(MagicItemDesc, m_Type) == 0x10);


    struct GamePackage
    {
        List<LevelDesc> m_Levels; // +0x00
        List<CharacterDesc> m_Characters; // +0x0C
        List<MagicItemDesc> m_MagicItems; // +0x18
    };
    static_assert(sizeof(GamePackage) == 36);
    static_assert(offsetof(GamePackage, m_Levels) == 0x00);
    static_assert(offsetof(GamePackage, m_Characters) == 0x0C);
    static_assert(offsetof(GamePackage, m_MagicItems) == 0x18);


    struct GameSpeeds
    {
        char _pad[0x0C];
        float currRatio; // +0x0C game speed multiplier (1.0 = normal)
    };
    static_assert(sizeof(GameSpeeds) == 16);
    static_assert(offsetof(GameSpeeds, currRatio) == 0x0C);


    struct Game
    {
        float           m_fDeathHeight;             // +0x000 Y below which characters respawn
        float           m_fPlayerIdleTime;          // +0x004
        uint32_t        m_PrevLevel;                // +0x008 CRC32 of the previously loaded level
        uint32_t        m_CurrLevel;                // +0x00C CRC32 of the current level
        uint32_t        m_NextLevel;                // +0x010 CRC32 of the queued next level (0 when none)
        uint32_t        m_CurrCategory;             // +0x014 CRC32 of category ("PvP_Level" = 0xBD608A50)
        uint8_t         m_bPrepareNextLevel;        // +0x018 true during level transition
        char            _pad0[0x03];                // +0x019
        GamePackage     m_GamePackage;              // +0x01C level / character / magic-item descriptors
        List<uint32_t>  m_LoadedCharacters;         // +0x040 CRC32s of loaded character IDs
        uint8_t         m_bFrameCorrection;         // +0x04C
        char            _pad1[0x23];                // +0x04D
        uint8_t         m_ControlsLockedCountdown;  // +0x070
        uint8_t         m_bGameFrozen;              // +0x071
        char            _pad2[0x52];                // +0x072
        int32_t         requiredControllers;        // +0x0A4
        int32_t         activeControllers;          // +0x0A8
        uint8_t         m_bLoadingMission;          // +0x0AC
        char            _pad3[0x03];                // +0x0AD
        volatile int32_t m_nLoadState;              // +0x0B0
        void*           m_pLoadScreen;              // +0x0B4
        void*           m_globalMF;                 // +0x0B8
        int32_t         m_nCoins;                   // +0x0BC
        uint8_t         m_forceResetToken;          // +0x0C0
        char            _pad4[0x03];                // +0x0C1
        GameSpeeds      gameSpeeds;                 // +0x0C4

        static Game* instance()
        {
            return reinterpret_cast<Game*>(GetAddress(GAME));
        }

        [[nodiscard]] bool isTransitioningLevel() const { return m_bPrepareNextLevel != 0; }
        [[nodiscard]] bool isInPvPCategory() const { return m_CurrCategory == 0xBD608A50u; }
        [[nodiscard]] bool isFrozen() const { return m_bGameFrozen != 0; }

        // Returns the LevelDesc for the currently loaded level
        [[nodiscard]] const LevelDesc* getCurrLevelDesc() const
        {
            for (const auto& desc : m_GamePackage.m_Levels)
                if (desc.m_Id == m_CurrLevel) return &desc;
            return nullptr;
        }

        // Returns the CharacterDesc for a given CRC32 character ID
        [[nodiscard]] const CharacterDesc* getCharacterDesc(uint32_t characterCrc) const
        {
            for (const auto& desc : m_GamePackage.m_Characters)
                if (desc.m_Id == characterCrc) return &desc;
            return nullptr;
        }
    };
    static_assert(offsetof(Game, m_PrevLevel) == 0x008);
    static_assert(offsetof(Game, m_CurrLevel) == 0x00C);
    static_assert(offsetof(Game, m_NextLevel) == 0x010);
    static_assert(offsetof(Game, m_CurrCategory) == 0x014);
    static_assert(offsetof(Game, m_bPrepareNextLevel) == 0x018);
    static_assert(offsetof(Game, m_GamePackage) == 0x01C);
    static_assert(offsetof(Game, m_LoadedCharacters) == 0x040);
    static_assert(offsetof(Game, m_bFrameCorrection) == 0x04C);
    static_assert(offsetof(Game, m_ControlsLockedCountdown) == 0x070);
    static_assert(offsetof(Game, m_bGameFrozen) == 0x071);
    static_assert(offsetof(Game, requiredControllers) == 0x0C4);
    static_assert(offsetof(Game, activeControllers) == 0x0C8);
    static_assert(offsetof(Game, m_bLoadingMission) == 0x0CC);
    static_assert(offsetof(Game, m_nLoadState) == 0x0D0);
    static_assert(offsetof(Game, m_pLoadScreen) == 0x0D4);
    static_assert(offsetof(Game, m_globalMF) == 0x0D8);
    static_assert(offsetof(Game, m_nCoins) == 0x0DC);
    static_assert(offsetof(Game, m_forceResetToken) == 0x0E0);
    static_assert(offsetof(Game, gameSpeeds) == 0x0E4);
} // namespace ssa
#pragma pack(pop)

namespace ssa::Game::Data
{
    struct LevelDisplayInfo
    {
        uint32_t crc;
        const char* internalName; // matches LevelDesc::m_LevelFile basename
        const char* displayName;
    };

    inline const LevelDisplayInfo* GetLevelDisplayInfo(uint32_t crc)
    {
        static constexpr LevelDisplayInfo k_levels[] =
        {
            {0x837D57EDu, "Level_000", "Molekin Mine"},
            {0xF47A677Bu, "Level_001", "Crystal Eye Castle"},
            {0x6D7336C1u, "Level_002", "Cadaverous Crypt"},
            {0x1A740657u, "Level_003", "Empire of Ice"},
            {0x6A1EF2D8u, "Level_006", "Leviathan Lagoon"},
            {0x8DA6DFDFu, "Level_008", "Pirate Seas"},
            {0xFAA1EF49u, "Level_009", "Troll Warehouse"},
            {0x9A6666ACu, "Level_010", "Kaos's Lair"},
            {0xAE44665Cu, "Level_010b", "The Final Fight"},
            {0x9D0BA2B5u, "Level_014", "Oilspill Island"},
            {0x0402F30Fu, "Level_017", "Falling Forest"},
            {0x94BDEE9Eu, "Level_018", "Sky Schooner Docks"},
            {0xE3BADE08u, "Level_019", "Stonetown"},
            {0xC64C05F9u, "Level_021", "Perilous Pastures"},
            {0x284264D5u, "Level_023", "Crawling Catacombs"},
            {0xB626F176u, "Level_024", "Lava Lakes Railway"},
            {0xC121C1E0u, "Level_025", "Arkeyan Armory"},
            {0x5828905Au, "Level_026", "Stormy Stronghold"},
            {0x2F2FA0CCu, "Level_027", "Shattered Island"},
            {0x465E6502u, "Level_032", "Creepy Citadel"},
            {0xAF3DC037u, "Level_034", "Goo Factory"},
            {0x4133A11Bu, "Level_036", "Darklight Crypt"},
            {0x3634918Du, "Level_037", "Dark Water Cove"},
            {0xA68B8C1Cu, "Level_038", "Treetop Terrace"},
            {0xD18CBC8Au, "Level_039", "Quicksilver Vault"},
            {0xE71192E9u, "Level_040", "Battleground"},
            {0x0E7237DCu, "Level_046", "Dragon's Peak"},
        };

        for (const auto& l : k_levels)
            if (l.crc == crc) return &l;
        return nullptr;
    }
} // namespace ssa
