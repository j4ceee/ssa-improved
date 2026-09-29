#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
namespace ssa::Game
{
    struct RigidBody
    {
        char    _pad0[0x18];    // +0x000
        Vec3    gravityDir;     // +0x018
        char    _pad1[0x04];    // +0x024
        float   gravityMag;     // +0x028
        char    _pad2[0x74];    // +0x02C
        Vec3    linearVelocity; // +0x0A0
    };
    static_assert(offsetof(RigidBody, gravityDir) == 0x018);
    static_assert(offsetof(RigidBody, gravityMag) == 0x028);
    static_assert(offsetof(RigidBody, linearVelocity) == 0x0A0);


    struct PhysBody
    {
        RigidBody*  rigidBody;  // +0x000
        char        _pad0[0x28];// +0x004
        Vec3        tractionA;  // +0x02C zeroed by SetTraction
        Vec3        tractionB;  // +0x038
    };
    static_assert(offsetof(PhysBody, tractionA) == 0x02C);
    static_assert(offsetof(PhysBody, tractionB) == 0x038);

    static constexpr uint16_t kColLayerPlayerBlocker = 0x0040; // player-only invisible walls

    static constexpr uint32_t kTractionBitEnable = 0x01;
    static constexpr uint32_t kTractionClearMask = 0x12;
    static constexpr uint32_t kTractionBitLock = 0x20; // if set, 0x40 gets cleared instead of set
    static constexpr uint32_t kTractionBit40 = 0x40;

    struct PhysicsObject
    {
        char        _pad0[0x9C];            // +0x000
        uint16_t    maskSend;               // +0x09C outbound collision filter
        uint16_t    maskReceive;            // +0x09E inbound collision filter
        uint16_t    maskSendOriginal;       // +0x0A0 restored from here on reconfigure
        uint16_t    maskReceiveOriginal;    // +0x0A2 restored from here on reconfigure
        PhysBody*   body;                   // +0x0A4
        char        _pad1[0x12C];           // +0x0A8
        uint32_t    tractionFlags;          // +0x1D4
        char        _pad2[0x008];           // +0x1D8
        uint32_t    group;                  // +0x1E0

        [[nodiscard]] RigidBody* rigidBody() const { return body ? body->rigidBody : nullptr; }

        // mirrors SetTraction(true, 0, 0) as used by jump pads / AirMotion unstuck hop
        void releaseTraction()
        {
            if (!body) return;
            body->tractionA = {};
            body->tractionB = {};
            uint32_t f = (tractionFlags | kTractionBitEnable) & ~kTractionClearMask;
            f = (f & kTractionBitLock) ? (f & ~kTractionBit40) : (f | kTractionBit40);
            tractionFlags = f;
        }
    };
    static_assert(offsetof(PhysicsObject, maskSend) == 0x09C);
    static_assert(offsetof(PhysicsObject, maskReceive) == 0x09E);
    static_assert(offsetof(PhysicsObject, maskSendOriginal) == 0x0A0);
    static_assert(offsetof(PhysicsObject, maskReceiveOriginal) == 0x0A2);
    static_assert(offsetof(PhysicsObject, group) == 0x1E0);
} // namespace ssa::Game
#pragma pack(pop)