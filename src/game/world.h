#pragma once

#include <cstddef>
#include "addresses.h"
#include "camera.h"
#include "component.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    using ComponentList = List<WeakPtr<Component>>;

    struct World
    {
        char    _pad0[0x00C];           // +0x000
        float   time;                   // +0x00C
        ComponentList componentList;        // +0x010 all live components
        ComponentList componentRemoveList;  // +0x01C
        ComponentList compInitList;         // +0x028 components waiting for their init (a component can be in more than one list)
        ComponentList compPostInitList;     // +0x034
        char    _pad1[0x074];           // +0x040
        float   updateSpd;              // +0x0B4
        float   playbackRatio;          // +0x0B8 (write playbackRatioNew instead)
        float   playbackRatioNew;       // +0x0BC
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
    static_assert(offsetof(World, componentList) == 0x10);
    static_assert(offsetof(World, compInitList) == 0x28);
    static_assert(offsetof(World, compPostInitList) == 0x34);
    static_assert(offsetof(World, updateSpd) == 0xB4);
    static_assert(offsetof(World, frameCorrector) == 0xC0);
    static_assert(offsetof(World, cameraSet) == 0x444);

} // namespace ssa::Game
#pragma pack(pop)