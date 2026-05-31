// Portions of this file are derived from Cemu (https://github.com/cemu-project/Cemu)
// Licensed under Mozilla Public License 2.0 - see THIRD_PARTY_NOTICES.md
#pragma once
#include <cstdint>

namespace ssa::Skylanders
{
    enum class Element
    {
        AIR = 0,
        EARTH = 1,
        FIRE = 2,
        LIFE = 3,
        MAGIC = 4,
        TECH = 5,
        UNDEAD = 6,
        WATER = 7,

        SIDEKICK = 8,
        ITEM = 9,
    };

    struct SkylanderInfo
    {
        uint16_t figureId;
        uint16_t variantId;
        const char* displayName;
        const char* fileAppend; // used for auto-generating filenames (cleanName_fileAppend_xxx.sky)
    };

    struct SkylanderCategory
    {
        const char* displayName;
        const char* cleanName;
        const Element element;
        const SkylanderInfo* figures;
        size_t numFigures;
    };

    // AIR
    constexpr SkylanderInfo g_lightningRod[] = {
        {3, 0x0000, "Lightning Rod"},
        {3, 0x1801, "Series 2 Lightning Rod", "s2"}
    };
    constexpr SkylanderInfo g_sonicBoom[] = {
        {1, 0x0000, "Sonic Boom"},
        {1, 0x1801, "Series 2 Sonic Boom", "s2"},
    };
    constexpr SkylanderInfo g_warnado[] = {
        {2, 0x0000, "Warnado"},
        {2, 0x2206, "LightCore Warnado", "lightcore"},
    };
    constexpr SkylanderInfo g_whirlwind[] = {
        {0, 0x0000, "Whirlwind"},
        {0, 0x1801, "Series 2 Whirlwind", "s2"},
        {0, 0x1C02, "Polar Whirlwind", "polar"},
        {0, 0x2805, "Horn Blast Whirlwind", "hb"},
        {0, 0x3810, "Eon's Elite Whirlwind", "ee"},
    };
    // EARTH
    constexpr SkylanderInfo g_bash[] = {
        {4, 0x0000, "Bash"},
        {4, 0x1801, "Series 2 Bash", "s2"},
        {404, 0x0000, "Legendary Bash", "legendary"},
    };
    constexpr SkylanderInfo g_dinoRang[] = {
        {6, 0x0000, "Dino Rang"},
        {6, 0x4810, "Eon's Elite Dino Rang", "ee"},
    };
    constexpr SkylanderInfo g_prismBreak[] = {
        {7, 0x0000, "Prism Break"},
        {7, 0x1801, "Series 2 Prism Break", "s2"},
        {7, 0x2805, "Hyper Beam Prism Break", "hb"},
        {7, 0x1206, "LightCore Prism Break", "lightcore"},
    };
    constexpr SkylanderInfo g_terrafin[] = {
        {5, 0x0000, "Terrafin"},
        {5, 0x1801, "Series 2 Terrafin", "s2"},
        {5, 0x2805, "Knockout Terrafin", "knockout"},
        {5, 0x3810, "Eon's Elite Terrafin", "ee"},
    };
    // FIRE
    constexpr SkylanderInfo g_eruptor[] = {
        {9, 0x0000, "Eruptor"},
        {9, 0x1801, "Series 2 Eruptor", "s2"},
        {9, 0x2C02, "Volcanic Eruptor", "volcanic"},
        {9, 0x2805, "Lava Barf Eruptor", "lava"},
        {9, 0x1206, "LightCore Eruptor", "lightcore"},
        {9, 0x3810, "Eon's Elite Eruptor", "ee"},
    };
    constexpr SkylanderInfo g_flameslinger[] = {
        {11, 0x0000, "Flameslinger"},
        {11, 0x1801, "Series 2 Flameslinger", "s2"},
    };
    constexpr SkylanderInfo g_ignitor[] = {
        {10, 0x0000, "Ignitor"},
        {10, 0x1801, "Series 2 Ignitor", "s2"},
        {10, 0x1C03, "Legendary Ignitor", "legendary"},
    };
    constexpr SkylanderInfo g_sunburn[] = {
        {8, 0x0000, "Sunburn"},
    };
    // LIFE
    constexpr SkylanderInfo g_camo[] = {
        {24, 0x0000, "Camo"},
        {24, 0x2805, "Thorn Horn Camo", "th"},
    };
    constexpr SkylanderInfo g_stealthElf[] = {
        {26, 0x0000, "Stealth Elf"},
        {26, 0x1801, "Series 2 Stealth Elf", "s2"},
        {26, 0x2C02, "Dark Stealth Elf", "dark"},
        {26, 0x1C03, "Legendary Stealth Elf", "legendary"},
        {26, 0x2805, "Ninja Stealth Elf", "ninja"},
        {26, 0x3810, "Eon's Elite Stealth Elf", "ee"},
    };
    constexpr SkylanderInfo g_stumpSmash[] = {
        {27, 0x0000, "Stump Smash"},
        {27, 0x1801, "Series 2 Stump Smash", "s2"},
    };
    constexpr SkylanderInfo g_zook[] = {
        {25, 0x0000, "Zook"},
        {25, 0x1801, "Series 2 Zook", "s2"},
        {25, 0x4810, "Eon's Elite Zook", "ee"},
    };
    // MAGIC
    constexpr SkylanderInfo g_doubleTrouble[] = {
        {18, 0x0000, "Double Trouble"},
        {18, 0x1801, "Series 2 Double Trouble", "s2"},
        {18, 0x1C02, "Royal Double Trouble", "royal"},
    };
    constexpr SkylanderInfo g_spyro[] = {
        {16, 0x0000, "Spyro"},
        {16, 0x1801, "Series 2 Spyro", "s2"},
        {16, 0x2C02, "Dark Mega Ram Spyro", "dmr"},
        {16, 0x2805, "Mega Ram Spyro", "mr"},
        {16, 0x3810, "Eon's Elite Spyro", "ee"},
        {28, 0x0000, "Dark Spyro", "dark"},
        {416, 0x0000, "Legendary Spyro", "legendary"},
    };
    constexpr SkylanderInfo g_voodood[] = {
        {17, 0x0000, "Voodood"},
        {17, 0x4810, "Eon's Elite Voodood", "ee"},
    };
    constexpr SkylanderInfo g_wreckingBall[] = {
        {23, 0x0000, "Wrecking Ball"},
        {23, 0x1801, "Series 2 Wrecking Ball", "s2"},
    };
    // TECH
    constexpr SkylanderInfo g_boomer[] = {
        {22, 0x0000, "Boomer"},
        {22, 0x4810, "Eon's Elite Boomer", "ee"},
    };
    constexpr SkylanderInfo g_drillSeargeant[] = {
        {21, 0x0000, "Drill Seargeant"},
        {21, 0x1801, "Series 2 Drill Seargeant", "s2"},
    };
    constexpr SkylanderInfo g_drobot[] = {
        {20, 0x0000, "Drobot"},
        {20, 0x1801, "Series 2 Drobot", "s2"},
        {20, 0x1206, "LightCore Drobot", "lightcore"},
    };
    constexpr SkylanderInfo g_triggerHappy[] = {
        {19, 0x0000, "Trigger Happy"},
        {19, 0x1801, "Series 2 Trigger Happy", "s2"},
        {19, 0x2C02, "Springtime Trigger Happy", "springtime"},
        {19, 0x2805, "Big Bang Trigger Happy", "bigbang"},
        {19, 0x3810, "Eon's Elite Trigger Happy", "ee"},
        {419, 0x0000, "Legendary Trigger Happy", "legendary"},
    };
    // UNDEAD
    constexpr SkylanderInfo g_chopChop[] = {
        {30, 0x0000, "Chop Chop"},
        {30, 0x1801, "Series 2 Chop Chop", "s2"},
        {30, 0x2805, "Twin Blade Chop Chop", "twinblade"},
        {30, 0x3810, "Eon's Elite Chop Chop", "ee"},
        {430, 0x0000, "Legendary Chop Chop", "legendary"},
    };
    constexpr SkylanderInfo g_cynder[] = {
        {32, 0x0000, "Cynder"},
        {32, 0x1801, "Series 2 Cynder", "s2"},
        {32, 0x2805, "Phantom Cynder", "phantom"},
    };
    constexpr SkylanderInfo g_ghostRoaster[] = {
        {31, 0x0000, "Ghost Roaster"},
        {31, 0x4810, "Eon's Elite Ghost Roaster", "ee"},
    };
    constexpr SkylanderInfo g_hex[] = {
        {29, 0x0000, "Hex"},
        {29, 0x1801, "Series 2 Hex", "s2"},
        {29, 0x1206, "LightCore Hex", "lightcore"},
    };
    // WATER
    constexpr SkylanderInfo g_gillGrunt[] = {
        {14, 0x0000, "Gill Grunt"},
        {14, 0x1801, "Series 2 Gill Grunt", "s2"},
        {14, 0x2805, "Anchors Away Gill Grunt", "aa"},
        {14, 0x3805, "Tidal Wave Gill Grunt", "tidalwave"},
        {14, 0x3810, "Eon's Elite Gill Grunt", "ee"},
    };
    constexpr SkylanderInfo g_slamBam[] = {
        {15, 0x0000, "Slam Bam"},
        {15, 0x1801, "Series 2 Slam Bam", "s2"},
        {15, 0x1C03, "Legendary Slam Bam", "legendary"},
        {15, 0x4810, "Eon's Elite Slam Bam", "ee"},
    };
    constexpr SkylanderInfo g_whamShell[] = {
        {13, 0x0000, "Wham Shell"},
        {13, 0x2206, "LightCore Wham Shell", "lightcore"},
    };
    constexpr SkylanderInfo g_zap[] = {
        {12, 0x0000, "Zap"},
        {12, 0x1801, "Series 2 Zap", "s2"},
    };

    constexpr SkylanderCategory g_skylanderDb[] = {
        // AIR
        {"Lightning Rod", "lightning_rod", Element::AIR, g_lightningRod, 2},
        {"Sonic Boom", "sonic_boom", Element::AIR, g_sonicBoom, 2},
        {"Warnado", "warnado", Element::AIR, g_warnado, 2},
        {"Whirlwind", "whirlwind", Element::AIR, g_whirlwind, 5},
        // EARTH
        {"Bash", "bash", Element::EARTH, g_bash, 3},
        {"Dino Rang", "dino_rang", Element::EARTH, g_dinoRang, 2},
        {"Prism Break", "prism_break", Element::EARTH, g_prismBreak, 4},
        {"Terrafin", "terrafin", Element::EARTH, g_terrafin, 4},
        // FIRE
        {"Eruptor", "eruptor", Element::FIRE, g_eruptor, 6},
        {"Flameslinger", "flameslinger", Element::FIRE, g_flameslinger, 2},
        {"Ignitor", "ignitor", Element::FIRE, g_ignitor, 3},
        {"Sunburn", "sunburn", Element::FIRE, g_sunburn, 1},
        // LIFE
        {"Camo", "camo", Element::LIFE, g_camo, 2},
        {"Stealth Elf", "stealth_elf", Element::LIFE, g_stealthElf, 6},
        {"Stump Smash", "stump_smash", Element::LIFE, g_stumpSmash, 2},
        {"Zook", "zook", Element::LIFE, g_zook, 3},
        // MAGIC
        {"Double Trouble", "double_trouble", Element::MAGIC, g_doubleTrouble, 3},
        {"Spyro", "spyro", Element::MAGIC, g_spyro, 7},
        {"Voodood", "voodood", Element::MAGIC, g_voodood, 2},
        {"Wrecking Ball", "wrecking_ball", Element::MAGIC, g_wreckingBall, 2},
        // TECH
        {"Boomer", "boomer", Element::TECH, g_boomer, 2},
        {"Drill Seargeant", "drill_seargeant", Element::TECH, g_drillSeargeant, 2},
        {"Drobot", "drobot", Element::TECH, g_drobot, 3},
        {"Trigger Happy", "trigger_happy", Element::TECH, g_triggerHappy, 6},
        // UNDEAD
        {"Chop Chop", "chop_chop", Element::UNDEAD, g_chopChop, 5},
        {"Cynder", "cynder", Element::UNDEAD, g_cynder, 3},
        {"Ghost Roaster", "ghost_roaster", Element::UNDEAD, g_ghostRoaster, 2},
        {"Hex", "hex", Element::UNDEAD, g_hex, 3},
        // WATER
        {"Gill Grunt", "gill_grunt", Element::WATER, g_gillGrunt, 5},
        {"Slam Bam", "slam_bam", Element::WATER, g_slamBam, 4},
        {"Wham Shell", "wham_shell", Element::WATER, g_whamShell, 2},
        {"Zap", "zap", Element::WATER, g_zap, 2},
    };
}
