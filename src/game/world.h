#pragma once

#include <cstddef>
#include "addresses.h"
#include "camera.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct World
    {
        char    _pad0[0x00C];           // +0x000
        float   time;                   // +0x00C
        char    _pad1[0x0A4];           // +0x010
        float   updateSpd;              // +0x0C8
        float   playbackRatio;          // +0x0CC (write playbackRatioNew instead)
        float   playbackRatioNew;       // +0x0D0
        float   frameCorrector;         // +0x0C0
        float   physicsFastAreaRadius;  // +0x0C4
        char    _pad2[0x37C];           // +0x0C8
        CameraSet cameraSet;            // +0x444

        static World* instance()
        {
            return reinterpret_cast<World*>(GetAddress(WORLD));
        }
    };
    static_assert(offsetof(World, time) == 0xC);
    static_assert(offsetof(World, updateSpd) == 0xB4);
    static_assert(offsetof(World, cameraSet) == 0x444);

} // namespace ssa::Game
#pragma pack(pop)