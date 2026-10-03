#pragma once

#include <cstdint>

namespace ssa::Game::Data
{
    namespace detail
    {
        constexpr uint32_t Crc32(const char* s)
        {
            uint32_t crc = 0xFFFFFFFFu;
            for (; *s; ++s)
            {
                crc ^= static_cast<uint8_t>(*s);
                for (int i = 0; i < 8; ++i)
                    crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
            }
            return ~crc;
        }
    }

    struct LevelDisplayInfo
    {
        const char* internalName;
        const char* displayName; // in-game name, nullptr if the level has none
        bool loadable; // offered in the level selector (false: menu, PvP arenas, test level)
        uint32_t crc;

        constexpr LevelDisplayInfo(const char* internalName, const char* displayName, bool loadable) : internalName(internalName), displayName(displayName), loadable(loadable), crc(detail::Crc32(internalName))
        {
        }

        [[nodiscard]] constexpr const char* label() const { return displayName ? displayName : internalName; }
    };

    namespace Levels
    {
        // menu
        inline constexpr LevelDisplayInfo FrontEnd{"FrontEnd", nullptr, false};
        // story levels
        inline constexpr LevelDisplayInfo Level_000{"Level_000", "Molekin Mine", true};
        inline constexpr LevelDisplayInfo Level_001{"Level_001", "Crystal Eye Castle", true};
        inline constexpr LevelDisplayInfo Level_002{"Level_002", "Cadaverous Crypt", true};
        inline constexpr LevelDisplayInfo Level_003{"Level_003", "Empire of Ice", true};
        inline constexpr LevelDisplayInfo Level_006{"Level_006", "Leviathan Lagoon", true};
        inline constexpr LevelDisplayInfo Level_008{"Level_008", "Pirate Seas", true};
        inline constexpr LevelDisplayInfo Level_009{"Level_009", "Troll Warehouse", true};
        inline constexpr LevelDisplayInfo Level_010{"Level_010", "Kaos's Lair", true};
        inline constexpr LevelDisplayInfo Level_014{"Level_014", "Oilspill Island", true};
        inline constexpr LevelDisplayInfo Level_017{"Level_017", "Falling Forest", true};
        inline constexpr LevelDisplayInfo Level_018{"Level_018", "Sky Schooner Docks", true};
        inline constexpr LevelDisplayInfo Level_019{"Level_019", "Stonetown", true};
        inline constexpr LevelDisplayInfo Level_021{"Level_021", "Perilous Pastures", true};
        inline constexpr LevelDisplayInfo Level_023{"Level_023", "Crawling Catacombs", true};
        inline constexpr LevelDisplayInfo Level_024{"Level_024", "Lava Lakes Railway", true};
        inline constexpr LevelDisplayInfo Level_025{"Level_025", "Arkeyan Armory", true};
        inline constexpr LevelDisplayInfo Level_026{"Level_026", "Stormy Stronghold", true};
        inline constexpr LevelDisplayInfo Level_027{"Level_027", "Shattered Island", true};
        inline constexpr LevelDisplayInfo Level_032{"Level_032", "Creepy Citadel", true};
        inline constexpr LevelDisplayInfo Level_034{"Level_034", "Goo Factory", true};
        inline constexpr LevelDisplayInfo Level_036{"Level_036", "Darklight Crypt", true};
        inline constexpr LevelDisplayInfo Level_037{"Level_037", "Dark Water Cove", true};
        inline constexpr LevelDisplayInfo Level_038{"Level_038", "Treetop Terrace", true};
        inline constexpr LevelDisplayInfo Level_039{"Level_039", "Quicksilver Vault", true};
        inline constexpr LevelDisplayInfo Level_040{"Level_040", "Battleground", true};
        inline constexpr LevelDisplayInfo Level_046{"Level_046", "Dragon's Peak", true};
        inline constexpr LevelDisplayInfo Level_010b{"Level_010b", "The Final Fight", true};
        // hub (ruins)
        inline constexpr LevelDisplayInfo Level_Hub{"Level_Hub", nullptr, true};
        // heroic challenges
        inline constexpr LevelDisplayInfo Challenge_Level_000{"Challenge_Level_000", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_001{"Challenge_Level_001", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_002{"Challenge_Level_002", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_003{"Challenge_Level_003", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_004{"Challenge_Level_004", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_005{"Challenge_Level_005", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_006{"Challenge_Level_006", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_007{"Challenge_Level_007", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_008{"Challenge_Level_008", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_009{"Challenge_Level_009", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_010{"Challenge_Level_010", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_011{"Challenge_Level_011", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_012{"Challenge_Level_012", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_013{"Challenge_Level_013", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_014{"Challenge_Level_014", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_015{"Challenge_Level_015", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_016{"Challenge_Level_016", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_017{"Challenge_Level_017", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_018{"Challenge_Level_018", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_019{"Challenge_Level_019", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_020{"Challenge_Level_020", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_021{"Challenge_Level_021", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_022{"Challenge_Level_022", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_023{"Challenge_Level_023", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_024{"Challenge_Level_024", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_025{"Challenge_Level_025", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_026{"Challenge_Level_026", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_027{"Challenge_Level_027", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_028{"Challenge_Level_028", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_029{"Challenge_Level_029", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_030{"Challenge_Level_030", nullptr, true};
        inline constexpr LevelDisplayInfo Challenge_Level_031{"Challenge_Level_031", nullptr, true};
        // pvp arenas
        inline constexpr LevelDisplayInfo PvP_Level_001{"PvP_Level_001", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_002{"PvP_Level_002", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_003{"PvP_Level_003", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_004{"PvP_Level_004", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_006{"PvP_Level_006", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_007{"PvP_Level_007", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_008{"PvP_Level_008", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_009{"PvP_Level_009", nullptr, false};
        inline constexpr LevelDisplayInfo PvP_Level_010{"PvP_Level_010", nullptr, false};
        // leftover dev level
        inline constexpr LevelDisplayInfo MyTest{"MyTest", nullptr, false};
    }

    static_assert(Levels::FrontEnd.crc == 0x0D2A3A2Au);
    static_assert(Levels::Level_Hub.crc == 0xD1F4E59Du);
    static_assert(Levels::Level_000.crc == 0x837D57EDu);

    inline constexpr const LevelDisplayInfo* kLevels[] =
    {
        &Levels::FrontEnd,
        &Levels::Level_000, &Levels::Level_001, &Levels::Level_002, &Levels::Level_003,
        &Levels::Level_006, &Levels::Level_008, &Levels::Level_009, &Levels::Level_010,
        &Levels::Level_014, &Levels::Level_017, &Levels::Level_018, &Levels::Level_019,
        &Levels::Level_021, &Levels::Level_023, &Levels::Level_024, &Levels::Level_025,
        &Levels::Level_026, &Levels::Level_027, &Levels::Level_032, &Levels::Level_034,
        &Levels::Level_036, &Levels::Level_037, &Levels::Level_038, &Levels::Level_039,
        &Levels::Level_040, &Levels::Level_046, &Levels::Level_010b,
        &Levels::Level_Hub,
        &Levels::Challenge_Level_000, &Levels::Challenge_Level_001, &Levels::Challenge_Level_002,
        &Levels::Challenge_Level_003, &Levels::Challenge_Level_004, &Levels::Challenge_Level_005,
        &Levels::Challenge_Level_006, &Levels::Challenge_Level_007, &Levels::Challenge_Level_008,
        &Levels::Challenge_Level_009, &Levels::Challenge_Level_010, &Levels::Challenge_Level_011,
        &Levels::Challenge_Level_012, &Levels::Challenge_Level_013, &Levels::Challenge_Level_014,
        &Levels::Challenge_Level_015, &Levels::Challenge_Level_016, &Levels::Challenge_Level_017,
        &Levels::Challenge_Level_018, &Levels::Challenge_Level_019, &Levels::Challenge_Level_020,
        &Levels::Challenge_Level_021, &Levels::Challenge_Level_022, &Levels::Challenge_Level_023,
        &Levels::Challenge_Level_024, &Levels::Challenge_Level_025, &Levels::Challenge_Level_026,
        &Levels::Challenge_Level_027, &Levels::Challenge_Level_028, &Levels::Challenge_Level_029,
        &Levels::Challenge_Level_030, &Levels::Challenge_Level_031,
        &Levels::PvP_Level_001, &Levels::PvP_Level_002, &Levels::PvP_Level_003,
        &Levels::PvP_Level_004, &Levels::PvP_Level_006, &Levels::PvP_Level_007,
        &Levels::PvP_Level_008, &Levels::PvP_Level_009, &Levels::PvP_Level_010,
        &Levels::MyTest,
    };

    // any level in the game, nullptr for levels this table doesn't know
    inline const LevelDisplayInfo* FindLevel(uint32_t crc)
    {
        for (const auto* l : kLevels)
            if (l->crc == crc) return l;
        return nullptr;
    }

    // legacy lookup: only levels with an in-game name (the story levels), nullptr otherwise
    inline const LevelDisplayInfo* GetLevelDisplayInfo(uint32_t crc)
    {
        const auto* l = FindLevel(crc);
        return (l && l->displayName) ? l : nullptr;
    }
}
