#pragma once

#include <cstddef>
#include <cstdint>
#include "addresses.h"
#include "component.h"
#include "data_types.h"

#pragma pack(push, 1)
namespace ssa::Game
{
    // - lives in the World's component lists (see World::componentList)
    // - owner is the entity object the projectile's effects hang off
    struct Projectile
    {
        Component   base;               // +0x000
        char        _pad0[0x14];        // +0x028
        uint32_t    triggerScriptName;  // +0x03C 0 if the projectile has no trigger script; NOT an attack ID: several attacks can share it
        char        _pad1[0x1C];        // +0x040
        WeakPtr<EntityObject> attacker; // +0x05C
        char        _pad2[0x31];        // +0x064
        uint8_t     enabled;            // +0x095
        uint8_t     fadeAndDisappear;   // +0x096
        char        _pad3[0x3D];        // +0x097
        uint8_t     lifeTimeUp;         // +0x0D4

        // the component as a projectile, null if it is something else
        [[nodiscard]] static Projectile* fromComponent(Component* component)
        {
            if (!component || component->vtable != GetAddress(PROJECTILE_VTABLE)) return nullptr;
            return reinterpret_cast<Projectile*>(component);
        }

        // still flying: not disabled, not fading out and its lifetime has not run out
        [[nodiscard]] bool isActive() const { return enabled && !fadeAndDisappear && !lifeTimeUp; }
    };

    static_assert(offsetof(Projectile, triggerScriptName) == 0x03C);
    static_assert(offsetof(Projectile, attacker) == 0x05C);
    static_assert(offsetof(Projectile, enabled) == 0x095);
    static_assert(offsetof(Projectile, fadeAndDisappear) == 0x096);
    static_assert(offsetof(Projectile, lifeTimeUp) == 0x0D4);
} // namespace ssa::Game
#pragma pack(pop)
