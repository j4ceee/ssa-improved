#pragma once

#include <cstdint>
#include "addresses.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    //  mask constants, found in Character::Init
    static constexpr uint32_t kTargetMaskEnemy = 0x20000000;
    static constexpr uint32_t kTargetMaskSkylander = 0xc0000000;
    static constexpr uint32_t kTargetMaskPvP = 0xa0000000;

    struct TargetEntry
    {
        void*    mPtr;    // +0x00  weak_ptr object pointer (lux::Object*)
        void*    mCB;     // +0x04  weak_ptr control block
        uint32_t mask;    // +0x08
        float    radius;  // +0x0C
    };

    struct TargetingList
    {
        static List<TargetEntry>* instance()
        {
            return reinterpret_cast<List<TargetEntry>*>(GetAddress(TARGETING_LIST));
        }
    };
} // namespace ssa::Game
#pragma pack(pop)
