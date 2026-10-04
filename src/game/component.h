#pragma once

#include <cstddef>
#include <cstdint>
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    struct EntityObject;

    // lux::Component: base of Light, TriggerListener & every other component an entity object holds
    struct Component
    {
        void*                   vtable;     // +0x00
        SharedPtr<Component>    self;       // +0x04 component's own shared_ptr, ptr is cleared once the engine deleted it
        SharedPtr<EntityObject> owner;      // +0x0C
        int16_t                 matrixIndex; // +0x14 attachment matrix of the owner the component follows
        int16_t                 flags;      // +0x16
        char                    _pad0[0x10]; // +0x18
    };

    static_assert(offsetof(Component, self) == 0x04);
    static_assert(offsetof(Component, owner) == 0x0C);
    static_assert(offsetof(Component, matrixIndex) == 0x14);
    static_assert(sizeof(Component) == 0x28);
} // namespace ssa::Game
#pragma pack(pop)
