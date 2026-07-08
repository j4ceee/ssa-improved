#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
namespace ssa::Game
{
    struct PhysicsObject
    {
        char        _pad0[0x9C];            // +0x000
        uint16_t    maskSend;               // +0x09C outbound collision filter
        uint16_t    maskReceive;            // +0x09E inbound collision filter
        uint16_t    maskSendOriginal;       // +0x0A0 restored from here on reconfigure
        uint16_t    maskReceiveOriginal;    // +0x0A2 restored from here on reconfigure
        char        _pad1[0x13C];           // +0x0A4
        uint32_t    group;                  // +0x1E0
    };
    static_assert(offsetof(PhysicsObject, maskSend) == 0x09C);
    static_assert(offsetof(PhysicsObject, maskReceive) == 0x09E);
    static_assert(offsetof(PhysicsObject, maskSendOriginal) == 0x0A0);
    static_assert(offsetof(PhysicsObject, maskReceiveOriginal) == 0x0A2);
    static_assert(offsetof(PhysicsObject, group) == 0x1E0);
} // namespace ssa::Game
#pragma pack(pop)