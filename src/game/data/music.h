#pragma once

#include <cstdint>

namespace ssa::Game::Data
{
    enum class MusicCategory : uint8_t { Menu, Hub, PvP };

    struct MusicTrack
    {
        uint32_t eventId; // Wwise event (global.bnk)
        const char* name;
        MusicCategory category;
    };

    // all single-track playlist events: they play without a music state
    namespace Music
    {
        inline constexpr MusicTrack MainMenu{0x46E7191Du, "Main Menu (original)", MusicCategory::Menu};
        // hub (ruins): 5 events for 6 GetSpyroRuinsState values
        inline constexpr MusicTrack Hub_1{0x4278CE76u, "Hub 1", MusicCategory::Hub}; // level_level_hub_1
        inline constexpr MusicTrack Hub_2{0x4278CE75u, "Hub 2", MusicCategory::Hub}; // level_level_hub_2
        inline constexpr MusicTrack Hub_3{0x4278CE74u, "Hub 3", MusicCategory::Hub}; // level_level_hub_3
        inline constexpr MusicTrack Hub_4{0x4278CE73u, "Hub 4", MusicCategory::Hub}; // level_level_hub_4
        inline constexpr MusicTrack Hub_5{0x4278CE72u, "Hub 5", MusicCategory::Hub}; // level_level_hub_5
        // pvp music
        inline constexpr MusicTrack PvP_1{0x1ADE9F22u, "PvP 1", MusicCategory::PvP}; // level_pvp_001
        inline constexpr MusicTrack PvP_2{0x1ADE9F21u, "PvP 2", MusicCategory::PvP}; // level_pvp_002
        inline constexpr MusicTrack PvP_3{0x1ADE9F20u, "PvP 3", MusicCategory::PvP}; // level_pvp_003
        inline constexpr MusicTrack PvP_4{0x1ADE9F27u, "PvP 4", MusicCategory::PvP}; // level_pvp_004
        inline constexpr MusicTrack PvP_5{0x1ADE9F26u, "PvP 5", MusicCategory::PvP}; // level_pvp_005
        inline constexpr MusicTrack PvP_6{0x1ADE9F25u, "PvP 6", MusicCategory::PvP}; // level_pvp_006
        inline constexpr MusicTrack PvP_7{0x1ADE9F24u, "PvP 7", MusicCategory::PvP}; // level_pvp_007
        inline constexpr MusicTrack PvP_8{0x1ADE9F2Bu, "PvP 8", MusicCategory::PvP}; // level_pvp_008
        inline constexpr MusicTrack PvP_9{0x1ADE9F2Au, "PvP 9", MusicCategory::PvP}; // level_pvp_009
        inline constexpr MusicTrack PvP_10{0x19DE9DB0u, "PvP 10", MusicCategory::PvP}; // level_pvp_010
    }

    inline constexpr const MusicTrack* kMusicTracks[] =
    {
        &Music::MainMenu,
        &Music::Hub_1, &Music::Hub_2, &Music::Hub_3, &Music::Hub_4, &Music::Hub_5,
        &Music::PvP_1, &Music::PvP_2, &Music::PvP_3, &Music::PvP_4, &Music::PvP_5,
        &Music::PvP_6, &Music::PvP_7, &Music::PvP_8, &Music::PvP_9, &Music::PvP_10,
    };

    inline const MusicTrack* FindMusicTrack(uint32_t eventId)
    {
        for (const auto* t : kMusicTracks)
            if (t->eventId == eventId) return t;
        return nullptr;
    }
}
