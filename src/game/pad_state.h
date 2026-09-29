#pragma once

#include <cstddef>
#include <cstdint>

#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct PadState
    {
        char        _pad0[0x4];         // +0x000
        int         index;              // +0x004 joystick id
        uint32_t    flags;              // +0x008
        char        _pad1[0x84];        // +0x00C
        uint32_t    logicalButtons;     // +0x090 held (rebuilt every poll, 0 while joyGetPosEx fails)
        uint32_t    hitButtons;         // +0x094 pressed this frame
        uint32_t    releasedButtons;    // +0x098 released this frame
        char        _pad2[0x70];        // +0x09C
        uint32_t    rawButtons;         // +0x10C
        char        _pad3[0x34];        // +0x110
        uint32_t    buttonMap[32];      // +0x144 raw bit -> logical mask (0 = unused by game)
        int         axisMap[4];         // +0x1C4

        static PadState* array() { return static_cast<PadState*>(GetAddress(PAD_STATE_ARRAY)); }
    };
    static_assert(offsetof(PadState, index) == 0x004);
    static_assert(offsetof(PadState, logicalButtons) == 0x090);
    static_assert(offsetof(PadState, rawButtons) == 0x10C);
    static_assert(offsetof(PadState, buttonMap) == 0x144);
    static_assert(offsetof(PadState, axisMap) == 0x1C4);
    static_assert(sizeof(PadState) == 0x1D4);
    static constexpr int kPadCount = 8;
}
#pragma pack(pop)