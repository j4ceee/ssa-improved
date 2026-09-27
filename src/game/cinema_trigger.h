#pragma once

#include <cstddef>
#include <cstdint>

#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct CinemaTrigger
    {
        char    _pad0[0x1F0];   // +0x000
        uint8_t bSeenBefore;    // +0x1F0 - runtime: "already watched"
        uint8_t bNeverSkip;     // +0x1F1 - level data: dev lock, never skippable
        uint8_t bAlwaysSkip;    // +0x1F2 - level data: always skippable

        // mirrors the skip condition in CinemaTrigger::Update / ReallyGo / UnTrigger
        [[nodiscard]] bool canSkip() const { return bAlwaysSkip || (bSeenBefore && !bNeverSkip); }

        static List<CinemaTrigger*>* instanceList()
        {
            return static_cast<List<CinemaTrigger*>*>(GetAddress(CINEMA_TRIGGER_LIST));
        }
    };
    static_assert(offsetof(CinemaTrigger, bSeenBefore) == 0x1F0);
    static_assert(offsetof(CinemaTrigger, bNeverSkip) == 0x1F1);
    static_assert(offsetof(CinemaTrigger, bAlwaysSkip) == 0x1F2);
} // namespace ssa::Game
#pragma pack(pop)